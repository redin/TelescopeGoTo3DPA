#include <math.h>
#include <Stepper.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Arduino_JSON.h>

const char *ssid     = "network";
const char *password = "passwd";

WiFiUDP ntpUDP;

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

//double calcDaysSinceJ2000(int y, int m, int d, int hours, int minutes, int seconds){
//  // lets use https://api.usno.navy.mil/jdconverter?date=today&time=now
//  double decFractionOfDay = decimalTime/24.000000;
//  //TODO: take into account leap years
//  int days2Month = days2MonthN[m-1];
//  double days2Year = days2YearN[y-2017];
//  return decFractionOfDay + days2Month + d + days2Year;
//}

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
  //lets use https://api.usno.navy.mil/sidtime?date=today&coords=30.042140S,51.210638W&reps=1&intv_mag=1&intv_unit=seconds&time=now

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  if (http.begin(*client, "https://api.usno.navy.mil/sidtime?date=today&coords=30.042140S,51.210638W&reps=1&intv_mag=1&intv_unit=seconds&time=now")) {
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
        JSONVar myObject = JSON.parse(payload);
        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
        }
        if (myObject.hasOwnProperty("properties")) {
          JSONVar keys = myObject.keys();
          JSONVar value = myObject["properties"];
          JSONVar data = value["data"];
          JSONVar data0 = data[0];
          JSONVar lmst = data0["lmst"];
          Serial.println(lmst);
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
  
  double lh = longitude/3600.0000;
  double lm = (longitude - (lh*3600.0000))/60.0000;
  double ls = longitude - (lh*3600.0000) -(lm*60.0000);
  double longDEC = lh + (lm/60.0000);
//LST = 100.46 + 0.985647 * d + long + 15*UT
  //double daysJ2000 = calcDaysSinceJ2000(rtime.Year(),rtime.Month(),rtime.Day(),rtime.Hour(),rtime.Minute(),rtime.Second()); 
  // lst = (0.985647 * daysJ2000) + (15.0000 * decimalTime) + longDEC + 100.460000;
  while(lst >360.0000){
    lst-= 360.00000;
  }
}

void loop() {

  
  currentMilis=millis();
  if(currentMilis > (previousMilis + 5000)){
    previousMilis = currentMilis;
    decimalTime =(double)timeClient.getHours()+(double)(timeClient.getMinutes()/60.0000)+(double)(timeClient.getSeconds()/3600.000000);
    calculateLST();
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
  }
  //Serial.print("ALT=");Serial.print(currentALT); Serial.print(" "); Serial.print(targetALT);  Serial.print(" "); Serial.print(deltaALTsteps); Serial.println(" "); 
  //Serial.print("AZ=");Serial.print(currentAZ); Serial.print(" "); Serial.print(targetAZ);  Serial.print(" "); Serial.print(deltaAZsteps); Serial.println(" "); 
  calculateALT_AZ();
  moveMount();
  if(Serial.available()>0){
    while(Serial.available() > 0){
      incomingChar = Serial.read();
      readCmd += incomingChar;
    }
  }else{
      if(readCmd.equals("#:GR#")){
        answerCurrentRA();
        Serial.flush();
        readCmd="";
      }else if(readCmd.equals("#:GD#")){
        answerCurrentDEC();
        Serial.flush();
        readCmd="";
      }else if(readCmd.equals("#:GL#")){
        if(timeClient.getHours() < 10){
          Serial.print("0");
        }
        Serial.print(timeClient.getHours(),DEC);
        Serial.print(":");
        if(timeClient.getMinutes() < 10){
          Serial.print("0");
        }
        Serial.print(timeClient.getMinutes(),DEC);
        Serial.print(":");
        if(timeClient.getSeconds() < 10){
          Serial.print("0");
        }
        Serial.print(timeClient.getSeconds(),DEC);
        Serial.print("#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.equals("#:GS#")){
        Serial.print(lst);
        Serial.print("#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith("#:SL ") && readCmd.length() == 14){
        //Set the local Time
        int ltH = readCmd.substring(5,7).toInt();
        int ltM = readCmd.substring(8,10).toInt();
        int ltS = readCmd.substring(11,13).toInt();
        //rtc.adjust(DateTime(time.year(), time.month(), time.day(), ltH, ltM, ltS));
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith("#:SC ") && readCmd.length() == 14){
        //Change Handbox Date to MM/DD/YY
        int month = readCmd.substring(5,7).toInt();
        int day = readCmd.substring(8,10).toInt();
        int yr = readCmd.substring(11,13).toInt();
        //rtc.adjust(DateTime(yr, month, day, rtime.Hour(), rtime.Minute(), rtime.Second()));
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith("#:Q#:Sr ") && readCmd.length() == 17) {
        parseTargetRA(readCmd.substring(8));
        Serial.print("1#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":Sd ") && readCmd.length() == 14) {
        parseTargetDEC(readCmd.substring(4));
        Serial.print("1#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":MS#")) {
        currentRA = targetRA;
        currentDEC= targetDEC;
        Serial.print("0#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.length() > 20){
        //Serial.println(readCmd);
        readCmd="";
      }
  }
    
}
