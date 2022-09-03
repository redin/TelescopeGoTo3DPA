#include <AccelStepper.h>

// Define pin connections
const int dirPinAZ = 2;
const int dirPinALT = 4;
const int stepPinAZ = 3;
const int stepPinALT = 5;

// Creates instances
AccelStepper stepperAZ(AccelStepper::DRIVER, stepPinAZ, dirPinALT);
AccelStepper stepperALT(AccelStepper::DRIVER, stepPinAZ, dirPinALT);

//1344000
const int stepsPerRevolution = 3200;
const double AZRange = 360.00000;
const double ALTRange = 90.00000;
const double stepsPerDegreeAZ = stepsPerRevolution/AZRange;
const double stepsPerDegreeALT = stepsPerRevolution/ALTRange;
const double degreesPerStepAZ = AZRange/stepsPerRevolution;
const double degreesPerStepALT = ALTRange/stepsPerRevolution;
double targetAZ = 0.0;
double targetALT = 0.0;
double currentAZ = 0.0;
double currentALT = 0.0;

unsigned long currentMilis=0;
unsigned long previousMilis=0;


boolean parked = true;
boolean aligned = false;
String inputString = "";
bool stringComplete = false;

int toSteps(double value, boolean alt){
  if(alt){
    return value * stepsPerDegreeALT;
  }else{
    return value * stepsPerDegreeAZ;
  }
}

double fromSteps(int value, boolean alt){
  if(alt){
    return value * degreesPerStepALT;
  }else{
    return value * degreesPerStepAZ;
  }
}

void moveMount(){
  if(!parked){
    if( stepperAZ.distanceToGo() == 0 ){
      currentAZ = targetAZ;
      stepperAZ.moveTo(toSteps(targetAZ, false));
    }else {
      currentAZ = fromSteps(stepperAZ.currentPosition(), false);
    }
    if( stepperALT.distanceToGo() == 0 ){
      currentALT = targetALT;
      stepperALT.moveTo(toSteps(targetALT, true));
    }else {
      currentALT = fromSteps(stepperALT.currentPosition(), true);
    } 
  }
}

void serialEvent() {
  while (Serial.available() && !stringComplete) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void setup(){
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(8, INPUT_PULLUP);
  Serial.begin (115200);
  inputString.reserve(200);
  stringComplete = false;
  stepperAZ.setMaxSpeed(200);
  stepperAZ.setAcceleration(30);
  stepperALT.setMaxSpeed(200);
  stepperALT.setAcceleration(30);
}

void setAligned(){
  boolean ualigned = digitalRead(8);
  if(!ualigned){
    currentAZ = targetAZ;
    currentALT = targetALT;
    aligned = true;
    parked = false;
    //Serial.println("aligned");
  }
}

void align(){
  int potAZ = map(analogRead(A0),0,1024,-512,512);
  int potALT = map(analogRead(A1),0,1024,-512,512);
  if( stepperAZ.distanceToGo() == 0 ){
    if(potAZ < -150){
      stepperAZ.move(-1);
    }else if(potAZ > 150){
      stepperAZ.move(1);
    }
  }
  if( stepperALT.distanceToGo() == 0 ){
    if(potALT < -150){
      stepperALT.move(-1);
    }else if(potALT > 150){
      stepperALT.move(1);
    }
  }
}

void reportCurrent(){
  if(fabs(targetAZ - currentAZ) > 0.001){
    Serial.print("AZ");
    Serial.println(currentAZ, 6);
  }
  if(fabs(targetALT - currentALT) > 0.001){
    Serial.print("AL");
    Serial.println(currentALT, 6);
  }
}

void loop(){
  stepperAZ.run();
  stepperALT.run();
  currentMilis=millis();
  if(currentMilis > (previousMilis + 100)){
    previousMilis = currentMilis;
    moveMount();
    align();
    setAligned();
    reportCurrent();
  }
  if (stringComplete) {
    String cmd = inputString;
    inputString = "";
    stringComplete = false;
    //Serial.print(cmd);
    if (cmd.startsWith("AZ")){
      String sval = cmd.substring(2);
      //Serial.print(sval);
      targetAZ = sval.toDouble();
    } else if (cmd.startsWith("AL")){
      String sval = cmd.substring(2);
      //Serial.print(sval);
      targetALT = sval.toDouble();
    } else if (cmd.startsWith("P")){
      parked = true;
    } else if (cmd.startsWith("UP")){
      parked = false;
    }
  }
}
