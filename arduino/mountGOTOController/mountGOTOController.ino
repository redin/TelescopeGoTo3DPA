#include <AccelStepper.h>

// Define pin connections

// Creates instances
AccelStepper stepperAZ(AccelStepper::DRIVER, 3, 2);
AccelStepper stepperALT(AccelStepper::DRIVER, 5, 4);

//1344000
const int stepsPerRevolution = 3200;
const double AZRange = 360.00000;
const double ALTRange = 90.00000;
const double stepsPerDegreeAZ = ((double)stepsPerRevolution)/AZRange;
const double stepsPerDegreeALT = ((double)stepsPerRevolution)/ALTRange;
const double degreesPerStepAZ = AZRange/((double)stepsPerRevolution);
const double degreesPerStepALT = ALTRange/((double)stepsPerRevolution);
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
    return (value * stepsPerDegreeALT);
  }else{
    return (value * stepsPerDegreeAZ);
  }
}

double fromSteps(int value, boolean alt){
  if(alt){
    return ((double)value) * degreesPerStepALT;
  }else{
    return ((double)value ) * degreesPerStepAZ;
  }
}

void moveMount(){
  if(!parked){
    if( stepperAZ.distanceToGo() == 0 ){
      currentAZ = targetAZ;
      
    }else {
      currentAZ = fromSteps(stepperAZ.currentPosition(), false);
    }
    stepperAZ.moveTo(toSteps(targetAZ, false));
    
    if( stepperALT.distanceToGo() == 0 ){
      currentALT = targetALT;
      
    }else {
      currentALT = fromSteps(stepperALT.currentPosition(), true);
    }
    stepperALT.moveTo(toSteps(targetALT, true)); 
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
  pinMode(8, INPUT);
  digitalWrite(8,HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin (115200);
  inputString.reserve(200);
  stringComplete = false;
  stepperAZ.setMaxSpeed(1000);
  stepperAZ.setAcceleration(300);
  stepperALT.setMaxSpeed(1000);
  stepperALT.setAcceleration(300);
}

void setAligned(){
  boolean ualigned = digitalRead(8);
  if(!ualigned && !aligned){
    currentAZ = targetAZ;
    currentALT = targetALT;
    aligned = true;
    parked = false;
    //Serial.println("aligned");
    digitalWrite(LED_BUILTIN, HIGH);
  }else if(!ualigned && aligned) {
    aligned = false;
    parked = true;
    //Serial.println("parked");
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void align(){
  int potAZ = map(analogRead(A0),0,1024,-512,512);
  int potALT = map(analogRead(A1),0,1024,-512,512);
  if( stepperAZ.distanceToGo() == 0 ){
    if(potAZ < -150){
      stepperAZ.setCurrentPosition(stepperAZ.currentPosition()-11);
    }else if(potAZ > 150){
      stepperAZ.setCurrentPosition(stepperAZ.currentPosition()+11);
      
    }
    stepperAZ.moveTo(toSteps(targetAZ, false));
  }
  if( stepperALT.distanceToGo() == 0 ){
    if(potALT < -150){
      stepperALT.setCurrentPosition(stepperALT.currentPosition()-11);
    }else if(potALT > 150){
      stepperALT.setCurrentPosition(stepperALT.currentPosition()+11);
    }
    stepperALT.moveTo(toSteps(targetALT, true)); 
  }
}

void reportCurrent(){
  if(fabs(targetAZ - currentAZ) > 0.00001){
    Serial.print("AZ");
    Serial.println(currentAZ, 6);
  }
  if(fabs(targetALT - currentALT) > 0.00001){
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
