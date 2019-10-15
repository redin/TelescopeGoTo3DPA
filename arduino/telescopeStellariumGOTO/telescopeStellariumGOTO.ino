#include <math.h>
#include <Stepper.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

const char *ssid     = "network";
const char *password = "password?";
const int epoch2jd = 946684800;

WiFiUDP ntpUDP;
const int port = 10001;
WiFiServer server(port);

NTPClient timeClient(ntpUDP);
const int timeOffset = 0;

const int stepsPerRevolution = 32;
const int stepsOne = 2048;
const double stepsPerDegreeAZ = stepsOne/360.0;
const double stepsPerDegreeALT = stepsOne/90.0;
const double degreesPerStepAZ = 360.0/stepsOne;
const double degreesPerStepALT = 90.0/stepsOne;
const int maxSpeed = 400;

Stepper stepperAZ(stepsPerRevolution, D1,D2,D3,D4);            
Stepper stepperALT(stepsPerRevolution, D5,D6,D7,D8);
WiFiClient cl;

double latitudeDEC=-30.042140;
double longitudeDEC=-51.210638;
boolean parked = true;
long deltaAZsteps = 0;
long deltaALTsteps = 0;
unsigned int ra = 0;
int dec = 0;
long h=0;
long m=0;
long s=0;
double lst;
double targetRA=0;
double targetDEC=0;
double currentRA=0;
double currentDEC=0;
double targetALT=0;
double targetAZ=0;
double currentALT=0;
double currentAZ=0;

unsigned long currentMilis=0;
unsigned long previousMilis=0;
double decimalTime;

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double stellariumRA2Double(unsigned int intRA){
  return mapDouble(intRA, 0x80000000, 0x100000000, 12.0, 24.0);
}

double stellariumDEC2Double(int intDEC){
  return mapDouble(intDEC, -0x40000000, 0x40000000, -90.0, 90.0);
}

void setup() {

  Serial.begin (115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  timeClient.setTimeOffset(timeOffset);
  timeClient.begin();
  
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.begin();
  Serial.println("TCP server started");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

int toSteps(double value, boolean alt){
  if(alt){
    return value * stepsPerDegreeALT;
  }else{
    return value * stepsPerDegreeAZ;
  }
}

double calcDaysSinceJ2000(){
  double epoch =  timeClient.getEpochTime();
  double seconds = (epoch - epoch2jd);
  return seconds / 86400;
}

void calculateLST(){
  //LST = 100.46 + 0.985647 * d + long + 15*UT
  double daysJ2000 = calcDaysSinceJ2000(); 
  lst = (0.985647 * daysJ2000) + (15.0000 * decimalTime) + longitudeDEC + 100.460000;
  while(lst >360.0000){
    lst-= 360.00000;
  }
  Serial.print("LST = ");
  Serial.println(mapDouble(lst, 0.0, 360.0, 0.0, 24.0),10);
}

void moveMount(){
  if(!parked){
    deltaAZsteps = toSteps(targetAZ - currentAZ, false);
    deltaALTsteps = toSteps(targetALT - currentALT, true);
    if(deltaAZsteps > 0){
      //stepperAZ.setSpeed(min(deltaAZsteps, maxSpeed));
      //stepperAZ.step(1);
      //currentAZ+= degreesPerStepAZ;
    }else if(deltaAZsteps < 0){
      //stepperAZ.setSpeed(min(abs(deltaAZsteps), maxSpeed));
      //stepperAZ.step(-1);
      //currentAZ-= degreesPerStepAZ;
    }
    if(deltaALTsteps > 0){
      //stepperALT.setSpeed(min(deltaALTsteps, maxSpeed));
      //stepperALT.step(1);
      //currentALT+= degreesPerStepALT;
    }else if(deltaALTsteps < 0){
      //stepperALT.setSpeed(min(abs(deltaALTsteps), maxSpeed));
      //stepperALT.step(-1);
      //currentALT-= degreesPerStepALT;
    }  
  }
}

//to convert the current telescope position to RADEC to pass to stellarium
void currentALTAZ2RADEC(){
  //Practical Astronomy with your Calculator or Spreadsheet
  double radLat = radians(latitudeDEC);
  double sinCurDec = sin(currentALT)*sin(radLat)+cos(currentALT)*cos(radLat)*cos(currentAZ);
  double radcurDec = asin(sinCurDec);
  double curDec = degrees(radcurDec);
  double cosHA = (sin(currentALT) - sin(radLat)* sin(sinCurDec))/ (cos(radLat)* cos(radcurDec));
  double curHA1 = acos(cosHA);
  double sinAZ = sin(currentAZ);
  double curHA = 0;
  if(sinAZ < 0){
    curHA = curHA1;
  }else{
    curHA = 360 - curHA1;
  }
  double curRA1 = mapDouble(lst, 0.0, 360.0, 0.0, 24.0) - curHA;
  double curRA = 0;
  if(curRA1 < 0){
    curRA = curRA1 + 24;
  }else{
    curRA = curRA1;
  }
  Serial.print("currentALT = ");
  Serial.println(currentALT,10);
  Serial.print("currentAZ = ");
  Serial.println(currentAZ,10);
  Serial.print("radLat = ");
  Serial.println(radLat,10);
  Serial.print("DEC = ");
  Serial.println(curDec,10);
  Serial.print("RA = ");
  Serial.println(curRA,10);
}

//to convert target position sent by stellarium to a telescope position
void targetRADEC2ALTAZ(){}


void reportcurrentRADEC(){
  if(cl.connected()){
    byte zero = 0x0;
    byte s = 0x18;
    short tp = 0;
    int st = 0;
    //Size
    cl.write(s);
    cl.write(zero);
    //Type
    cl.write(zero);
    cl.write(zero);
    //Time
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    //RA
    byte l0 = ra;
    byte l1 = (ra>>8);
    byte h0 = (ra>>16);
    byte h1 = (ra>>24);
    cl.write(l0);
    cl.write(l1);
    cl.write(h0);
    cl.write(h1);
    //DEC
    l0 = dec;
    l1 = (dec>>8);
    h0 = (dec>>16);
    h1 = (dec>>24);
    cl.write(l0);
    cl.write(l1);
    cl.write(h0);
    cl.write(h1);
    //STATUS
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
    cl.write(zero);
  }
}

void readTargetRADEC(){
  if(cl.connected()){
    while(cl.available()>0){
      short s = 0;
      short tp = 0;
      long tm = 0;
      byte l = cl.read();
      byte h =  cl.read();
      s = (h<<8)+l;
      l = cl.read();
      h =  cl.read();
      tp = (h<<8)+l;
      byte l0 = cl.read();
      byte l1 =  cl.read();
      byte l2 = cl.read();
      byte l3 =  cl.read();
      byte h0 = cl.read();
      byte h1 =  cl.read();
      byte h2 = cl.read();
      byte h3 =  cl.read();
      tm = (h3<<56)+(h2<<48)+(h1<<40)+(h0<<32)+(l3<<24)+(l2<<16)+(l1<<8)+l0;
      l0 = cl.read();
      l1 =  cl.read();
      l2 = cl.read();
      l3 =  cl.read();
      ra = (l3<<24)+(l2<<16)+(l1<<8)+l0;
      l0 = cl.read();
      l1 =  cl.read();
      l2 = cl.read();
      l3 =  cl.read();
      dec = (l3<<24)+(l2<<16)+(l1<<8)+l0;
      targetRA = stellariumRA2Double(ra);
      targetDEC = stellariumDEC2Double(dec);
    }
  }
}

void loop() {
  currentMilis=millis();
  ArduinoOTA.handle();
  MDNS.update();
  if(cl == NULL){
    cl = server.available();
  }else{
    readTargetRADEC();
  }
  if(currentMilis > (previousMilis + 1000)){
    previousMilis = currentMilis;
    timeClient.update();
    decimalTime =(double)timeClient.getHours()+(double)(timeClient.getMinutes()/60.0000)+(double)(timeClient.getSeconds()/3600.000000);
    calculateLST();
    targetRADEC2ALTAZ();
    currentALTAZ2RADEC();
    reportcurrentRADEC();
    Serial.println(timeClient.getFormattedTime());
  }
  moveMount();
}
