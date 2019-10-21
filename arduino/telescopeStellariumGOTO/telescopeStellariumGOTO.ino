#include <math.h>
#include <Stepper.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
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

unsigned int mapUint(double x, double in_min, double in_max, unsigned int out_min, unsigned int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double stellariumRA2Double(unsigned int intRA){
  return mapDouble(intRA, 0x0, 0x100000000, 0.0, 24.0);
}

double stellariumDEC2Double(int intDEC){
  return mapDouble(intDEC, -0x40000000, 0x40000000, -90.0, 90.0);
}

unsigned int RADouble2stellarium(double raDouble){
  return mapUint(raDouble, 0.0, 24.0, 0x0, 0x100000000);
}

int DECDouble2stellarium(double DECDouble){
  return map(DECDouble, -90.0, 90.0, -0x40000000, 0x40000000);
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
  
//  if (MDNS.begin("esp8266")) {
//    Serial.println("MDNS responder started");
//  }

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
  while(lst < 0.0000){
    lst+= 360.00000;
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
  double radALT = radians(currentALT);
  double radAZ = radians(currentAZ);
  double radLat = radians(latitudeDEC);
  double sinCurDec = sin(radALT)*sin(radLat)+cos(radALT)*cos(radLat)*cos(radAZ);
  double radcurDec = asin(sinCurDec);
  double curDec = degrees(radcurDec);

  double y = -cos(radALT)*cos(radLat)*sin(radAZ);
  double x = sin(radALT)-sin(radLat)*sinCurDec;
  double a = atan2(x,y);
  double b = degrees(a);
  double c = 0;
  if(x < 0.0000){
    c = b + 180.0000;
  }else if(x > 0.0000 & y < 0.0000){
    c = b + 360.0000;
  }
  
  double curRA1 = mapDouble(lst, 0.0000, 360.0000, 0.0000, 24.0000) - c;
  double curRA = 0;
  if(curRA1 < 0.0000){
    curRA = curRA1 + 24.0000;
  }else{
    curRA = curRA1;
  }
  //Serial.print("DEC = ");
  //Serial.println(curDec,10);
  currentDEC = curDec;
  //Serial.print("RA = ");
  //Serial.println(curRA,10);
  currentRA = curRA;
}

//to convert target position sent by stellarium to a telescope position
void targetRADEC2ALTAZ(){
  double h1 =  mapDouble(lst, 0.0000, 360.0000, 0.0000, 24.0000) - targetRA;
  double h = 0;
  if(h1 < 0.0000){
    h = h1 + 24.0000;
  }else{
    h = h1;
  }
  double degH = h*15.0000;
  double radDEC = radians(targetDEC);
  double radLat = radians(latitudeDEC);
  double sinALT = sin(radDEC)*sin(radLat)+cos(radDEC)*cos(radLat)*cos(degH);
  double radALT = asin(sinALT);
  double alt = degrees(radALT);

  double y = -cos(radDEC)*cos(radLat)*sin(degH);
  double x = sin(radDEC)-sin(radLat)*sinALT;
  double a = atan2(x,y);
  double b = degrees(a);
  double c = 0;
  if(x < 0.0000){
    c = b + 180.0000;
  }else if(x > 0.0000 & y < 0.0000){
    c = b + 360.0000;
  }
  
  double az = c;

  if(c < 0.0000){
    az = c + 360.0000;
  }else{
    az = c;
  }
  targetALT = alt;
  targetAZ = az;
  
}

void reportcurrentRADEC(){
  if(cl.connected()){
    double epoch =  timeClient.getEpochTime();
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
    unsigned int cra = RADouble2stellarium(currentRA);
    byte l0 = cra;
    byte l1 = (cra>>8);
    byte h0 = (cra>>16);
    byte h1 = (cra>>24);
    cl.write(l0);
    cl.write(l1);
    cl.write(h0);
    cl.write(h1);
    //DEC
    int cdec = DECDouble2stellarium(currentDEC);
    l0 = cdec;
    l1 = (cdec>>8);
    h0 = (cdec>>16);
    h1 = (cdec>>24);
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
  //MDNS.update();
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
    Serial.print("targetDEC = ");
    Serial.println(targetDEC,10);
    Serial.print("targetRA = ");
    Serial.println(targetRA,10);
    Serial.print("targetALT = ");
    Serial.println(targetALT,10);
    Serial.print("targetAZ = ");
    Serial.println(targetAZ,10);
    Serial.print("currentDEC = ");
    Serial.println(currentDEC,10);
    Serial.print("currentRA = ");
    Serial.println(currentRA,10);
    Serial.print("currentALT = ");
    Serial.println(currentALT,10);
    Serial.print("currentAZ = ");
    Serial.println(currentAZ,10);
  }
  moveMount();
}
