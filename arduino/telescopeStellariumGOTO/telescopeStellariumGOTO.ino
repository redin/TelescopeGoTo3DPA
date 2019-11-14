#include <math.h>

#include <ESP8266WiFi.h>
//#include <WiFiUdp.h>
//#include <ESP8266HTTPClient.h>
//#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <ESP8266mDNS.h>

const int epoch2jd = 946684800;

const int stepsPerRevolution = 40960;
const double AZRange = 360.00000;
const double ALTRange = 90.00000;
const double stepsPerDegreeAZ = stepsPerRevolution/AZRange;
const double stepsPerDegreeALT = stepsPerRevolution/ALTRange;
const double degreesPerStepAZ = AZRange/stepsPerRevolution;
const double degreesPerStepALT = ALTRange/stepsPerRevolution;

WiFiUDP ntpUDP;
const int port = 10001;
WiFiServer server(port);

NTPClient timeClient(ntpUDP);
const int timeOffset = 0;

WiFiClient cl;

double latitudeDEC=-30.042140;
double longitudeDEC=-51.210638;
int ledState = LOW;
unsigned int ra = 0;
int dec = 0;
double lst;
double targetRA=0;
double targetDEC=0;
double currentRA=0;
double currentDEC=0;
double targetALT=0;
double targetAZ=0;
double currentALT=0;
double currentAZ=0;
boolean parked = true;
boolean aligned = false;
double deltaAZ = 0.00000;
double deltaALT = 0.0000;

unsigned long currentMilis=0;
unsigned long previousMilis=0;
double decimalTime;

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double stellariumRA2Double(unsigned int intRA){
  return mapDouble(intRA, 0x0, 0x100000000, 0.00000, 24.00000);
}

double stellariumDEC2Double(int intDEC){
  return mapDouble(intDEC, -0x40000000, 0x40000000, -90.00000, 90.00000);
}

unsigned int RADouble2stellarium(double raDouble){
  return (unsigned int) mapDouble(raDouble, 0.00000, 24.00000, 0x0, 0x100000000);
}

signed int DECDouble2stellarium(double DECDouble){
  
  return (signed int) mapDouble(DECDouble, -90.00000, 90.00000, -0x40000000, 0x40000000);
}

void calcDeltas(){
  deltaAZ = targetAZ - currentAZ;
  deltaALT = targetALT - currentALT;
//  if((currentAZ - targetAZ) > (targetAZ - currentAZ)){
//    deltaAZ = targetAZ - currentAZ;
//  }else{
//    deltaAZ = currentAZ - targetAZ;
//  }
//  if((currentALT - targetALT) > (targetALT - currentALT)){
//    deltaALT = targetALT - currentALT;
//  }else{
//    deltaALT = currentALT - targetALT;
//  }
}

int toSteps(double value, boolean alt){
  if(alt){
    return value * stepsPerDegreeALT;
  }else{
    return value * stepsPerDegreeAZ;
  }
}

void sendDeltaSteps(){
  int stepsAZ = toSteps(deltaAZ,false);
  int stepsALT = toSteps(deltaALT,true);
  if(aligned && parked){
    Serial.println("UP");
    parked = false;
  }
  if(stepsAZ != 0 && !parked){
    Serial.print("AZ");
    Serial.println(stepsAZ);
    currentAZ = targetAZ;
  }
  if(stepsALT != 0 && !parked){
    Serial.print("AL");
    Serial.println(stepsALT);
    currentALT = targetALT;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  pinMode(D1, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin (115200);
//  WiFi.begin(ssid, password);
//
//  while ( WiFi.status() != WL_CONNECTED ) {
//    delay ( 500 );
//    Serial.print ( "." );
//  }

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if(!wifiManager.autoConnect("telescope","GOTO3DPA")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  timeClient.setTimeOffset(timeOffset);
  timeClient.begin();

  if (!MDNS.begin("telescope")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("telescope", "tcp", 10001);

  server.begin();
  Serial.println("TCP server started");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}

double calcDaysSinceJ2000(){
  double epoch =  timeClient.getEpochTime();
  double seconds = (epoch - epoch2jd);
  return seconds / 86400;
}

void calculateLST(){
  //LST = 100.46 + 0.985647 * d + long + 15*UT
  double daysJ2000 = calcDaysSinceJ2000(); 
  lst = (0.985647 * daysJ2000) + (15.00000 * decimalTime) + longitudeDEC + 100.460000;
  while(lst >360.00000){
    lst-= 360.00000;
  }
  while(lst < 0.0000){
    lst+= 360.00000;
  }
//  Serial.print("LST = ");
//  Serial.println(mapDouble(lst, 0.00000, 360.00000, 0.00000, 24.00000),10);
}


//to convert the current telescope position to RADEC to pass to stellarium
void currentALTAZ2RADEC(){
  //Practical Astronomy with your Calculator or Spreadsheet
  double radALT = radians(currentALT);
  double radAZ = radians(currentAZ);
  double radLat = radians(latitudeDEC);
  double sinCurDec = sin(radALT)*sin(radLat)+cos(radALT)*cos(radLat)*cos(radAZ);
  double radcurDec = asin(sinCurDec);
  double curDec = degrees(radcurDec);

  double sinAlt = sin(radALT);
  double sinLat = sin(radLat);
  double cosH = (sinAlt - (sinLat*sinCurDec))/(cos(radLat)*cos(radcurDec));
  double h = degrees(acos(cosH));
  double sinA = sin(radAZ);
  if(sinA > 0.000000){
    h = 360.00000 - h;
  }
  double curRA = mapDouble(lst, 0.0000, 360.0000, 0.0000, 24.0000) - (h/15.0);
  if(curRA < 0.00000){
    curRA = curRA + 24.00000;
  }
  currentDEC = curDec;
  currentRA = curRA;
}

//to convert target position sent by stellarium to a telescope position
void targetRADEC2ALTAZ(){
  double h =  mapDouble(lst, 0.0000, 360.0000, 0.0000, 24.0000) - targetRA;
  double degH = h*15.0000;
  double radH = radians(degH);
  double radDEC = radians(targetDEC);
  double radLat = radians(latitudeDEC);
  double sinALT = (sin(radDEC)*sin(radLat))+(cos(radDEC)*cos(radLat)*cos(radH));
  double radALT = asin(sinALT);
  double radCosALT = cos(radALT);
  double alt = degrees(radALT);
  double cosAZ = (sin(radDEC) - (sin(radLat) * sinALT))/ (cos(radLat)* radCosALT);
  double az = degrees(acos(cosAZ));
  double sinH = sin(radH);
  if(sinH > 0.00000){
    az = 360.00000 - az;
  }
  
  targetALT = alt;
  targetAZ = az;
  
}

void reportcurrentRADEC(){
  if(cl.connected()){
    double epoch =  timeClient.getEpochTime();
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
    unsigned int cra = RADouble2stellarium(currentRA);
    byte l0 = cra;
    byte l1 = (cra>>8);
    byte h0 = (cra>>16);
    byte h1 = (cra>>24);
    cl.write(l0);
    cl.write(l1);
    cl.write(h0);
    cl.write(h1);
    //DEC
    int cdec = DECDouble2stellarium(currentDEC);
    l0 = cdec;
    l1 = (cdec>>8);
    h0 = (cdec>>16);
    h1 = (cdec>>24);
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
      targetRA = stellariumRA2Double(ra);
      targetDEC = stellariumDEC2Double(dec);
    }
  }
}

void align(){
  boolean ualigned = digitalRead(D1);
//  Serial.print("ualigned ");
//  Serial.println(ualigned);
  if(ualigned){
    currentAZ = targetAZ;
    currentALT = targetALT;
    deltaAZ = 0.00000;
    deltaALT = 0.0000;
    aligned = true;
    Serial.println("UP");
    parked = false;
  }else{
    currentAZ = targetAZ;
    currentALT = targetALT;
    deltaAZ = 0.00000;
    deltaALT = 0.0000;
  }
}

void loop() {
  currentMilis=millis();
  if(cl == NULL){
    cl = server.available();
  }else{
    readTargetRADEC();
  }
  if(currentMilis > (previousMilis + 250)){
    previousMilis = currentMilis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED_BUILTIN, ledState);
    timeClient.update();
    MDNS.update();
    decimalTime =(double)timeClient.getHours()+(double)(timeClient.getMinutes()/60.0000)+(double)(timeClient.getSeconds()/3600.000000);
    calculateLST();
    targetRADEC2ALTAZ();
    currentALTAZ2RADEC();
    reportcurrentRADEC();
    //Serial.println(timeClient.getFormattedTime());
//    Serial.print("targetDEC = ");
//    Serial.println(targetDEC,10);
//    Serial.print("targetRA = ");
//    Serial.println(targetRA,10);
//    Serial.print("targetALT = ");
//    Serial.println(targetALT,10);
//    Serial.print("targetAZ = ");
//    Serial.println(targetAZ,10);
//    Serial.print("currentDEC = ");
//    Serial.println(currentDEC,10);
//    Serial.print("currentRA = ");
//    Serial.println(currentRA,10);
//    Serial.print("currentALT = ");
//    Serial.println(currentALT,10);
//    Serial.print("currentAZ = ");
//    Serial.println(currentAZ,10);
  }
  //Send message to move mount
  calcDeltas();
  if(!aligned){
    align();
  }
  if(!parked && (deltaAZ > 0.0000 || deltaAZ < 0.0000 || deltaALT > 0.0000 || deltaALT < 0.0000)){
    sendDeltaSteps();
  }
}
