
char incomingChar;
String readCmd;

void setup() {
  // open the serial port:
  Serial.begin(9600);
  //acquireUTC();
  //ensureParked();
  //Serial.end();
  //connectStellarium();
}

void loop() {
  if(Serial.available()>0){
    delay(1000);
    while(Serial.available() > 0){
      incomingChar = Serial.read();
      readCmd += incomingChar;
      //Serial.print(incomingChar);
    }
    //Serial.println("/Read");
  }else{
      if(readCmd.equals("#:GR#")){
        Serial.print("10:00:00#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.equals("#:GD#")){
        Serial.write("+");
        Serial.write("10");
        Serial.write((char) 223);
        Serial.write("00#");
        Serial.flush();
        readCmd="";
      } else if(readCmd.startsWith("#:Q#:Sr ")) {
        Serial.print("1");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":Sd ")) {
        //Serial.print(readCmd);
        Serial.print("1");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":MS#")) {
        //Serial.print(readCmd);
        Serial.print("0");
        Serial.flush();
        readCmd="";
      }else if(readCmd.length() > 20){
        //Serial.print(readCmd);
        readCmd="";
      }
  }
    
}
