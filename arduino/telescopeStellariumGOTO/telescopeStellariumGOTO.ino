#include <math.h>
#include <Stepper.h>
#include <Wire.h>
#include "RTClib.h"

const int days2MonthN[]= {0,31,59,90,120,151,181,212,243,273,304,334};
const int days2MonthL[]= {0,31,60,91,121,152,182,213,244,274,305,335};

const double days2YearN[]={6208.5,6573.5,6938.5,7303.5,7669.5};

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

Stepper stepperAZ(stepsPerRevolution, 8,10,9,11);            
Stepper stepperALT(stepsPerRevolution, 4,6,5,7);
RTC_DS1307 rtc;

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
DateTime time;
double decimalTime;


void setup() {
  currentRA=23L*3600L+59L*60L+58L;
  currentDEC=-23L*3600L+59L*60L;
  targetRA=23L*3600L+59L*60L+58L;
  targetDEC=-23L*3600L+59L*60L;
  //-30.042140
  latitude = -30L*3600L+02L*60L+31L;
  //-51.210638
  longitude = -51L*3600L+(-12L*60L)+(-38L);
  Serial.begin(9600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  time = rtc.now();
  decimalTime =(double)time.hour()+((double)time.minute()/60.0000)+((double)time.second()/3600.000000);
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

double calcDaysSinceJ2000(int y, int m, int d, int hours, int minutes, int seconds){
  double decFractionOfDay = decimalTime/24.000000;
  //TODO: take into account leap years
  int days2Month = days2MonthN[m-1];
  double days2Year = days2YearN[y-2017];
  return decFractionOfDay + days2Month + d + days2Year;
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
      stepperAZ.setSpeed(min(deltaAZsteps, maxSpeed));
      stepperAZ.step(1);
      currentAZ+= degreesPerStepAZ;
    }else if(deltaAZsteps < 0){
      stepperAZ.setSpeed(min(abs(deltaAZsteps), maxSpeed));
      stepperAZ.step(-1);
      currentAZ-= degreesPerStepAZ;
    }
    if(deltaALTsteps > 0){
      stepperALT.setSpeed(min(deltaALTsteps, maxSpeed));
      stepperALT.step(1);
      currentALT+= degreesPerStepALT;
    }else if(deltaALTsteps < 0){
      stepperALT.setSpeed(min(abs(deltaALTsteps), maxSpeed));
      stepperALT.step(-1);
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
  double daysJ2000 = calcDaysSinceJ2000(time.year(),time.month(),time.day(),time.hour(),time.minute(),time.second()); 
  lst = (0.985647 * daysJ2000) + (15.0000 * decimalTime) + longDEC + 100.460000;
  while(lst >360.0000){
    lst-= 360.00000;
  }
}

void loop() {
  currentMilis=millis();
  if(currentMilis > (previousMilis + 1000)){
    previousMilis = currentMilis;
    time = rtc.now();
    decimalTime =(double)time.hour()+((double)time.minute()/60.0000)+((double)time.second()/3600.000000);
    calculateLST();
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
        if(time.hour() < 10){
          Serial.print("0");
        }
        Serial.print(time.hour(),DEC);
        Serial.print(":");
        if(time.minute() < 10){
          Serial.print("0");
        }
        Serial.print(time.minute(),DEC);
        Serial.print(":");
        if(time.second() < 10){
          Serial.print("0");
        }
        Serial.print(time.second(),DEC);
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
        rtc.adjust(DateTime(time.year(), time.month(), time.day(), ltH, ltM, ltS));
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith("#:SC ") && readCmd.length() == 14){
        //Change Handbox Date to MM/DD/YY
        int month = readCmd.substring(5,7).toInt();
        int day = readCmd.substring(8,10).toInt();
        int yr = readCmd.substring(11,13).toInt();
        rtc.adjust(DateTime(yr, month, day, time.hour(), time.minute(), time.second()));
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
