
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
float lst;
float targetALT=0;
float targetAZ=0;
float currentALT=0;
float currentAZ=0;
float ut=0;

void setup() {
  Serial.begin(9600);
  currentRA=23L*3600L+59L*60L+58L;
  currentDEC=-23L*3600L+59L*60L;
  //-30.042140
  latitude = -30L*3600L+02L*60L+31L;
  //-51.210638
  longitude = -51L*3600L+12L*60L+38L;
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



void calculateALT_AZ(){
  //calculateLST();
  float raDeg = targetRA;
  float hourAngle = lst - raDeg;
  if(hourAngle < 0){
    hourAngle = hourAngle + 360.0F;
  }
  float sinAlt = sin(targetDEC)*sin(latitudeDEC)+cos(targetDEC)*cos(latitudeDEC)*cos(hourAngle);
  targetALT = asin(sinAlt);
  float cosA = (sin(targetDEC) - sin(targetALT)*sin(latitudeDEC))/(cos(targetALT)*cos(latitudeDEC));
  if(sin(hourAngle) < 0){
    targetAZ = acos(cosA);
  }else{
    targetAZ = 360.0F - acos(cosA);
  }
}

void loop() {
  calculateALT_AZ();
  if(Serial.available()>0){
    delay(60);
    while(Serial.available() > 0){
      incomingChar = Serial.read();
      readCmd += incomingChar;
    }
    //Serial.println("/Read");
  }else{
      if(readCmd.equals("#:GR#")){
        answerCurrentRA();
        Serial.flush();
        readCmd="";
      }else if(readCmd.equals("#:GD#")){
        answerCurrentDEC();
        Serial.flush();
        readCmd="";
      } else if(readCmd.startsWith("#:Q#:Sr ") && readCmd.length() == 17) {
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
        //Serial.print(readCmd);
        readCmd="";
      }
  }
    
}
