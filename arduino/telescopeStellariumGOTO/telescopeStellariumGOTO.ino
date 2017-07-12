
char incomingChar;
String readCmd;
long currentRA=0;
long currentDEC=0;
long h=0;
long m=0;
long s=0;


void setup() {
  Serial.begin(9600);
  currentRA=23L*3600L+59L*60L+58L;
}
void answerCurrentRA() {
  h = currentRA/3600L;
  m = (currentRA - (h*3600))/60L;
  s = currentRA - (h*3600) -(m*60);
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
  //Serial.print("10:00:00#");
}

void loop() {
  if(Serial.available()>0){
    delay(50);
    while(Serial.available() > 0){
      incomingChar = Serial.read();
      readCmd += incomingChar;
      //Serial.print(incomingChar);
    }
    //Serial.println("/Read");
  }else{
      if(readCmd.equals("#:GR#")){
        answerCurrentRA();
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
        Serial.print("1#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":Sd ")) {
        //Serial.print(readCmd);
        Serial.print("1#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.startsWith(":MS#")) {
        //Serial.print(readCmd);
        Serial.print("0#");
        Serial.flush();
        readCmd="";
      }else if(readCmd.length() > 20){
        //Serial.print(readCmd);
        readCmd="";
      }
  }
    
}
