#include <Unistep2.h>

const int stepsPerRevolution = 4096;
boolean parked = true;
int deltaAZsteps = 0;
int deltaALTsteps = 0;
String inputString = "";
bool stringComplete = false;

Unistep2 stepperAZ(3,4,5,6, stepsPerRevolution, 2000);            
Unistep2 stepperALT(8,9,10,11, stepsPerRevolution, 2000);

void moveMount(){
  if(!parked){
    if( stepperAZ.stepsToGo() == 0 ){
      if(deltaAZsteps > 0){
        stepperAZ.move(1);
        deltaAZsteps--;
        Serial.println(deltaAZsteps);
      }else if(deltaAZsteps < 0){
        stepperAZ.move(-1);
        deltaAZsteps++;
        Serial.println(deltaAZsteps);
      }
    }
    if( stepperALT.stepsToGo() == 0 ){
      if(deltaALTsteps > 0){
        stepperALT.move(1);
        deltaALTsteps--;
        Serial.println(deltaALTsteps);
      }else if(deltaALTsteps < 0){
        stepperALT.move(-1);
        deltaALTsteps++;
        Serial.println(deltaALTsteps);
      }
    } 
  }
}

void serialEvent() {
  while (Serial.available()) {
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
  Serial.begin (115200);
  inputString.reserve(200);
}

void align(){
  int potAZ = map(analogRead(A0),0,1024,-512,512);
  int potALT = map(analogRead(A1),0,1024,-512,512);
//  Serial.print("potAZ ");
//  Serial.println(potAZ);
//  Serial.print("potALT ");
//  Serial.println(potALT);
  if( stepperAZ.stepsToGo() == 0 ){
    if(potAZ < -150){
      stepperAZ.move(-1);
      Serial.println("AZ-");
    }else if(potAZ > 150){
      stepperAZ.move(1);
      Serial.println("AZ+");
    }
  }
  if( stepperALT.stepsToGo() == 0 ){
    if(potALT < -150){
      stepperALT.move(-1);
      Serial.println("ALT-");
    }else if(potALT > 150){
      stepperALT.move(1);
      Serial.println("ALT+");
    }
  }
}

void loop(){
  stepperAZ.run();
  stepperALT.run();
  moveMount();
  align();
  if (stringComplete) {
    String cmd = inputString;
    inputString = "";
    stringComplete = false;
    Serial.print(cmd);
    if (cmd.startsWith("AZ")){
      String sval = cmd.substring(2);
      Serial.print(sval);
      deltaAZsteps += sval.toInt();
    } else if (cmd.startsWith("AL")){
      String sval = cmd.substring(2);
      Serial.print(sval);
      deltaALTsteps += sval.toInt();
    } else if (cmd.startsWith("P")){
      parked = true;
    } else if (cmd.startsWith("UP")){
      parked = false;
    }
  }
}
