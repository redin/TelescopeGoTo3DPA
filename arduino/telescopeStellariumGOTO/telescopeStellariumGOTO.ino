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

const int days2MonthN[]= {0,31,59,90,120,151,181,212,243,273,304,334};
const int days2MonthL[]= {0,31,60,91,121,152,182,213,244,274,305,335};
const double days2YearN[]={6208.5,6573.5,6938.5,7303.5,7669.5};

WiFiUDP ntpUDP;
int port = 10001;
WiFiServer server(port);

NTPClient timeClient(ntpUDP);
int timeOffset = -10800;

const int stepsPerRevolution = 32;
const int stepsOne = 2048;
const double stepsPerDegreeAZ = stepsOne/360.0;
const double stepsPerDegreeALT = stepsOne/90.0;
const double degreesPerStepAZ = 360.0/stepsOne;
const double degreesPerStepALT = 90.0/stepsOne;
int deltaAZsteps = 0;
int deltaALTsteps = 0;
int maxSpeed = 400;
boolean parked = false;

//Stepper stepperAZ(stepsPerRevolution, 8,10,9,11);            
//Stepper stepperALT(stepsPerRevolution, 4,6,5,7);

char incomingChar;
String readCmd;
long currentRA=0;
long currentDEC=0;
long targetRA=0;
long targetDEC=0;
long h=0;
long m=0;
long s=0;
long latitude=0;
float latitudeDEC=0;
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

int count=0;

void setup() {

  Serial.begin (115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.setTimeOffset(timeOffset);
  timeClient.begin();
  currentRA=23L*3600L+59L*60L+58L;
  currentDEC=-23L*3600L+59L*60L;
  targetRA=23L*3600L+59L*60L+58L;
  targetDEC=-23L*3600L+59L*60L;
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

void answerCurrentRA() {
  h = currentRA/3600L;
  m = (currentRA - (h*3600L))/60L;
  s = currentRA - (h*3600L) -(m*60L);
  if(h <10){
    Serial.write('0');
  }
  Serial.print(h);
  Serial.print(':');
  if(m <10){
    Serial.print('0');
  }
  Serial.print(String(m,DEC));
  Serial.print(':');
 
  if(s <10){
    Serial.print('0');
  }
  Serial.print(String(s,DEC));
  Serial.print('#');
}

void answerCurrentDEC() {
  h = currentDEC/3600L;
  
  m = (currentDEC - (h*3600L))/60L;
  if(h < 0){
    Serial.write('-');
  }else{
    Serial.write('+');
  }
  if(abs(h) <10){
    Serial.write('0');
  }
  Serial.print(abs(h));
  Serial.print((char) 223);
  if(abs(m) <10){
    Serial.print('0');
  }
  
  Serial.print(String(abs(m),DEC));
  Serial.print('#');
}

void parseTargetRA(String target){
  long tH = target.toInt();
  long tM = target.substring(3,5).toInt();
  long tS = target.substring(6,8).toInt();
  targetRA = tH*3600L + tM*60L + tS;
}

void parseTargetDEC(String targetD){
  long tH = targetD.substring(1,3).toInt();
  long tM = targetD.substring(4,6).toInt();
  long tS = targetD.substring(7,9).toInt();
  //Serial.print(String(tS,DEC));
  targetDEC = tH*3600L + tM*60L + tS;
  if(targetD[0] == '-'){
    targetDEC = targetDEC*-1L;
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

void calculateLST(){
 
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
}

WiFiClient cl;

void loop() {
  MDNS.update();
  
  if(cl == NULL){
    cl = server.available();
  }else{
    if(cl.connected()){
      while(cl.available()>0){
        short s = 0;
        short tp = 0;
        long tm = 0;
        unsigned int ra = 0;
        int dec = 0;
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
        Serial.println(ra, BIN); 
        Serial.println(ra, DEC);
        double t0 = (double)ra;
        double t1 = (double) 4294967296;
        Serial.println((t0*24.00d/t1),10);
        Serial.print("DEC = ");
        Serial.println(dec,BIN); //1073741824
        Serial.println(dec,DEC);
        Serial.println(map(dec,-1073741824,1073741824,-90,90),10);
      }
    }          
  }
  currentMilis=millis();
  if(currentMilis > (previousMilis + 1000)){
    previousMilis = currentMilis;
    decimalTime =(double)timeClient.getHours()+(double)(timeClient.getMinutes()/60.0000)+(double)(timeClient.getSeconds()/3600.000000);
    timeClient.update();
    calculateLST();
    Serial.println(timeClient.getFormattedTime());
  }
  //Serial.print("ALT=");Serial.print(currentALT); Serial.print(" "); Serial.print(targetALT);  Serial.print(" "); Serial.print(deltaALTsteps); Serial.println(" "); 
  //Serial.print("AZ=");Serial.print(currentAZ); Serial.print(" "); Serial.print(targetAZ);  Serial.print(" "); Serial.print(deltaAZsteps); Serial.println(" "); 
  calculateALT_AZ();
  moveMount();
}
