#include <math.h>
#include <Stepper.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char *ssid     = "network";
const char *password = "password?";
const int epoch2jd = 946684800;

WiFiUDP ntpUDP;
int port = 10001;
WiFiServer server(port);

NTPClient timeClient(ntpUDP);
int timeOffset = 0;

const int stepsPerRevolution = 32;
const int stepsOne = 2048;
const double stepsPerDegreeAZ = stepsOne/360.0;
const double stepsPerDegreeALT = stepsOne/90.0;
const double degreesPerStepAZ = 360.0/stepsOne;
const double degreesPerStepALT = 90.0/stepsOne;
int deltaAZsteps = 0;
int deltaALTsteps = 0;
int maxSpeed = 400;
boolean parked = true;

//Stepper stepperAZ(stepsPerRevolution, 8,10,9,11);            
//Stepper stepperALT(stepsPerRevolution, 4,6,5,7);

unsigned int ra = 0;
int dec = 0;
long currentRA=0;
long currentDEC=0;
long targetRA=0;
long targetDEC=0;
long h=0;
long m=0;
long s=0;
long latitude=0;
float latitudeDEC=-30.042140;
long longitude=0;
float longitudeDEC=0;
//LST = 100.46 + 0.985647 * d + long + 15*UT
double lst;
double targetALT=0;
double targetAZ=0;
double currentALT=0;
double currentAZ=0;
double ut=0;
unsigned long currentMilis=0;
unsigned long previousMilis=0;
double decimalTime;

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {

  Serial.begin (115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.setTimeOffset(timeOffset);
  timeClient.begin();
  //-30.042140
  latitude = -30L*3600L+02L*60L+31L;
  //-51.210638
  longitude = -51L*3600L+(-12L*60L)+(-38L);

  decimalTime =(double)timeClient.getHours()+((double)timeClient.getMinutes()/60.0000)+((double)timeClient.getSeconds()/3600.000000);
  
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

void calculateALT_AZ(){
  double lh = targetRA/3600.0000;
  double lm = (targetRA - (lh*3600.0000))/60.0000;
  double ls = targetRA - (lh*3600.0000) -(lm*60.0000);
  double raDeg = (double)lh + ((double)lm/60.0000)+ ((double)ls/3600.0000);;
  double hourAngle = lst - raDeg;
  if(hourAngle < 0){
    hourAngle = hourAngle + 360.0F;
  }
  double sinAlt = sin(targetDEC)*sin(latitudeDEC)+cos(targetDEC)*cos(latitudeDEC)*cos(hourAngle);
  targetALT = asin(sinAlt);
  double cosA = (sin(targetDEC) - sin(targetALT)*sin(latitudeDEC))/(cos(targetALT)*cos(latitudeDEC));
  if(sin(hourAngle) < 0){
    targetAZ = acos(cosA);
  }else{
    targetAZ = 360.0F - acos(cosA);
  }
}

void moveMount(){
  if(!parked){
    deltaAZsteps = toSteps(targetAZ - currentAZ, false);
    deltaALTsteps = toSteps(targetALT - currentALT, true);
    if(deltaAZsteps > 0){
      //stepperAZ.setSpeed(min(deltaAZsteps, maxSpeed));
      //stepperAZ.step(1);
      currentAZ+= degreesPerStepAZ;
    }else if(deltaAZsteps < 0){
      //stepperAZ.setSpeed(min(abs(deltaAZsteps), maxSpeed));
      //stepperAZ.step(-1);
      currentAZ-= degreesPerStepAZ;
    }
    if(deltaALTsteps > 0){
      //stepperALT.setSpeed(min(deltaALTsteps, maxSpeed));
      //stepperALT.step(1);
      currentALT+= degreesPerStepALT;
    }else if(deltaALTsteps < 0){
      //stepperALT.setSpeed(min(abs(deltaALTsteps), maxSpeed));
      //stepperALT.step(-1);
      currentALT-= degreesPerStepALT;
    }  
  }
}

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
  double curRA1 = lst - curHA;
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

void calculateLST(){
  currentALTAZ2RADEC();
  double lh = longitude/3600.0000;
  double lm = (longitude - (lh*3600.0000))/60.0000;
  double ls = longitude - (lh*3600.0000) -(lm*60.0000);
  double longDEC = lh + (lm/60.0000);
//LST = 100.46 + 0.985647 * d + long + 15*UT
  double daysJ2000 = calcDaysSinceJ2000(); 
  lst = (0.985647 * daysJ2000) + (15.0000 * decimalTime) + longDEC + 100.460000;
  while(lst >360.0000){
    lst-= 360.00000;
  }
  Serial.print("LST = ");
  Serial.println(lst,10);
}

WiFiClient cl;

void reportCurremtRADEC(){
  
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
      Serial.print("Size = ");
      Serial.println(s);
      Serial.print("Type = ");
      Serial.println(tp);
      Serial.print("Time = ");
      Serial.println(tm);
      Serial.print("RA = "); 
      Serial.println(ra, DEC);
      double t0 = (double)ra;
      double t1 = (double) 4294967296;
      Serial.println((t0*24.00d/t1),10);
      Serial.print("DEC = ");
      Serial.println(dec,DEC);
      Serial.println(mapDouble(dec,-1073741824,1073741824,-90,90),10);
    }
  }
}

void loop() {
  MDNS.update();
  
  if(cl == NULL){
    cl = server.available();
  }else{
    readTargetRADEC();
  }
  currentMilis=millis();
  if(currentMilis > (previousMilis + 1000)){
    previousMilis = currentMilis;
    reportCurremtRADEC();
    timeClient.update();
    decimalTime =(double)timeClient.getHours()+(double)(timeClient.getMinutes()/60.0000)+(double)(timeClient.getSeconds()/3600.000000);
    calculateLST();
    Serial.println(timeClient.getFormattedTime());
  }
  calculateALT_AZ();
  moveMount();
}
