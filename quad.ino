//==========================================================================================
// nakanoshima robot:○ kawa　2018.5.5
// mePed_nakarobo_COM.ino 
//
// The mePed is an open source quadruped robot designed by Scott Pierce of 
// Spierce Technologies (www.meped.io & www.spiercetech.com) 
// 
// This program is based on code written by Alexey Butov (www.alexeybutov.wix.com/roboset) 
// 
//========================================================================================== 
#include <ESP8266WiFi.h> 
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <Servo.h> 
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "RemoteDebug.h"

// RUN
// Open serial monitor 9600bps and Input numeric.
//
// 0 minimal
// 1 lean_left
// 2 forward
// 3 lean_right
// 4 turn_right
// 5 center_servos
// 6 turn_left
// 7 step_left
// 8 back
// 9 step_right

//Quad (Top View)
//FRONT
//  -----               -----
// |  2  |             |  8  |
// | P03 |             | P09 |
//  ----- -----   ----- -----
//       |  1  | |  7  |
//       | P02 | | P08 |
//        -----   -----
//       |  3  | |  5  |
//       | P04 | | P06 |
//  ----- -----   ----- -----
// |  4  |             |  6  |
// | P05 |             | P07 |
//  -----               -----  
//BACK

//Arudiroid4 (Top View)
//FLONT
// sp1: Speed 1 sp2: Speed 2 sp3: Speed 3 sp4: Speed 4 
//
//  -----       -----
// | SP1 |      | SP4 |
//  -----        -----
//        ----- 
//       | Nano|
//       |     |
//        ----- 
//  -----        -----
// | SP2 |      | SP3 |
//  -----        -----  
//BACK

// a,90,90,90,90,90,90,90,90 [Enter]
// s,1,90 [Enter]
  
//===== Globals ============================================================================ 
/* define port */
//WiFiClient client;
ESP8266WebServer server(80);
RemoteDebug Debug;

char* Hostname = "Quad"; // The network name
char* otaPass = "quadbot"; // The network name

#define MAX_SERVO_NUM 8
Servo servo[MAX_SERVO_NUM+1];

/* data received from application */
String  data =""; 
// calibration
int da = -12; // Left Front Pivot 
int db =  18; // Left Back Pivot
int dc = -18; // Right Back Pivot
int dd =  12; // Right Front Pivot 
// servo initial positions + calibration 
int a90 = (90 + da), a120 = (120 + da), a150 = (150 + da), a180 = (180 + da);
int b0 = (0 + db), b30 = (30 + db), b60 = (60 + db), b90 = (90 + db);
int c90 = (90 + dc), c120 = (120 + dc), c150 = (150 + dc), c180 = (180 + dc);
int d0 = (0 + dd), d30 = (30 + dd), d60 = (60 + dd), d90 = (90 + dd);
// start points for servo 
int s11 = 90; // Front Left Pivot Servo 
int s12 = 90; // Front Left Lift Servo 
int s21 = 90; // Back Left Pivot Servo 
int s22 = 90; // Back Left Lift Servo 
int s31 = 90; // Back Right Pivot Servo 
int se32 = 90; // Back Right Lift Servo 
int s41 = 90; // Front Right Pivot Servo
int s42 = 90; // Front Right Lift Servo
int f = 0;
int b = 0;
int l = 0;
int r = 0;
int spd = 50;// Speed of walking motion, larger the number, the slower the speed
int high = 0;// How high the robot is standing

// Define 8 Servos 
//Servo servo[1]; // Front Left Pivot S  ervo 
//Servo servo[2]; // Front Left Lift Servo 
//Servo servo[3]; // Back Left Pivot Servo 
//Servo servo[4]; // Back Left Lift Servo 
//Servo servo[5]; // Back Right Pivot Servo 
//Servo servo[6]; // Back Right Lift Servo 
//Servo servo[7]; // Front Right Pivot Servo 
//Servo servo[8]; // Front Right Lift Servo 

const char MAIN_page[] PROGMEM = R"=====(
  <!DOCTYPE html>
<html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
  background: #2c3e50ff;
  color: red;
  text-align: center; 
  font-family: "Trebuchet MS", Arial; 
  margin-left:auto; 
  margin-right:auto;
}
button:focus {
    outline: 0;
}
a:visited {
    color: green;
}

a:link {
    color: red;
}
</style>
<title>Quad Control</title>
<body>
</head><body>
<h1>Quad Control</h1>
<button onclick="SendCommand('stop')">Home</button> | <button onclick="SendCommand('minimal')">Minimal</button><br><br>

<button onclick="SendCommand('forward')">Forward</button><br>
<button onclick="SendCommand('left')">Left</button><button onclick="SendCommand('right')">Right</button><br>
<button onclick="SendCommand('backward')">Back</button><br><br>

<button onclick="SendCommand('stepl')">Step Left</button> | <button onclick="SendCommand('stepr')">Step Right</button><br><br>
<button onclick="SendCommand('leanl')">Lean Left</button> | <button onclick="SendCommand('leanr')">Lean Right</button><br><br>
<button onclick="SendCommand('lay')">Lay</button> | <button onclick="SendCommand('s2s')">Side to Side</button><br><br>
<button onclick="SendCommand('fightst')">Fighting Stance</button>  | <button onclick="SendCommand('bow')">bow</button><br><br>
<script>
function SendCommand(Command) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
  };
  xhttp.open("GET", "/api?command=" + Command + "", true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";

const char DOC_page[] PROGMEM = R"=====(
<HTML>
  <HEAD>
      <TITLE>Quad Control API</TITLE>
  </HEAD>
<BODY>
  <CENTER>
  <h2>Received</h2> <br><br>
      <h2>Servo Layout</h2>
<img src='data:image/png;base64,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' alt=''></BODY>
</HTML>
)=====";

void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void handleSpecificArg() { 

String message = "Invalid or No command given";

if (server.arg("command")== ""){    //Parameter not found

}else{     //Parameter found

if (server.arg("command") == "forward"){
  forward();
  message = "Received";
}
if (server.arg("command") == "backward"){
  back();
  message = "Received";
}
if (server.arg("command") == "left"){
  turn_left();
  message = "Received";
}
if (server.arg("command") == "right"){
  turn_right();
  message = "Received";
}
if (server.arg("command") == "stop"){
  center_servos();
  message = "Received";
}
if (server.arg("command") == "minimal"){
  minimal();
  message = "Received";
}
if (server.arg("command") == "leanl"){
  lean_left();
  message = "Received";
}
if (server.arg("command") == "leanr"){
  lean_right();
  message = "Received";
}
if (server.arg("command") == "stepl"){
  step_left();
  message = "Received";
}
if (server.arg("command") == "stepr"){
  step_right();
  message = "Received";
}
if (server.arg("command") == "lay"){
  lay();
  message = "Received";
}
if (server.arg("command") == "s2s"){
  side2side();
  message = "Received";
}
if (server.arg("command") == "fightst"){
  fightst();
  message = "Received";
}
if (server.arg("command") == "bow"){
  bow();
  message = "Received";
}

String parseArg = getValue(server.arg("command"), ',', 0); // if  a,4,D,r  would return D  
if (parseArg == "s" || parseArg == "S"){
  int servoN = getValue(server.arg("command"), ',', 1).toInt(); // if  a,4,D,r  would return D 
  int sAngle = getValue(server.arg("command"), ',', 2).toInt(); // if  a,4,D,r  would return D
  if ( 0 <= servoN &&  servoN < MAX_SERVO_NUM+1 )
      {
        // Single Servo Move
        servo[servoN].write(sAngle);

        // Wait Servo Move
        //delay(300); // 180/60(°/100msec）=300(msec)
      }
  message = "Received";
  Debug.println("Servo moved");
}

if (parseArg == "a" || parseArg == "A"){
  int angle[MAX_SERVO_NUM];
  angle[0] = getValue(server.arg("command"), ',', 1).toInt(); // if  a,4,D,r  would return D 
  angle[1] = getValue(server.arg("command"), ',', 2).toInt(); // if  a,4,D,r  would return D 
  angle[2] = getValue(server.arg("command"), ',', 3).toInt(); // if  a,4,D,r  would return D 
  angle[3] = getValue(server.arg("command"), ',', 4).toInt(); // if  a,4,D,r  would return D 
  angle[4] = getValue(server.arg("command"), ',', 5).toInt(); // if  a,4,D,r  would return D
  angle[5] = getValue(server.arg("command"), ',', 6).toInt(); // if  a,4,D,r  would return D
  angle[6] = getValue(server.arg("command"), ',', 7).toInt(); // if  a,4,D,r  would return D
  angle[7] = getValue(server.arg("command"), ',', 8).toInt(); // if  a,4,D,r  would return D
  
  set8Pos( angle[0], angle[1], angle[2], angle[3], angle[4], angle[5], angle[6], angle[7] );
  message = "Received";
  Debug.println("Servos moved");
}



}
// String  var = getValue( StringVar, ',', 2); // if  a,4,D,r  would return D  
//server.send(200, "text/plain", message);          //Returns the HTTP response
String s = DOC_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page

}

//===== Setup ============================================================================== 
void setup() {
  Serial.begin (9600);
  WiFi.hostname(Hostname);
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.autoConnect(Hostname);

// Initialize the server (telnet or web socket) of RemoteDebug
Debug.begin(Hostname);
// OR
//Debug.begin(HOST_NAME, startingDebugLevel);
// Options
Debug.setResetCmdEnabled(true); // Enable the reset command
// Debug.showProfiler(true); // To show profiler - time between messages of Debug

  String IPString = WiFi.localIP().toString();
  Serial.println(IPString);
  // Start position
  attachServo();
  delay(300);
//  minimal();
  center_servos();
  delay(300);
//  detachServo();
  delay(300);
  
  server.on("/", handleRoot);    
  server.on("/api", handleSpecificArg);   //Associate the handler function to the path
  
  server.begin();                                       //Start the server
  Serial.println("Server listening");   
  
  ArduinoOTA.setPassword((const char *)otaPass);
    ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

//setup 
//========================================================================================== 

//== Loop ================================================================================== 
void loop() {
  unsigned long value;
  int lastValue;

  // Center all servos 
  high = 15;
  // Set hight to 15 
  spd = 3;
  // Set speed to 3 

//attachServo();
server.handleClient(); 
  ArduinoOTA.handle();
  Debug.handle();
 if (Serial.available() > 0) {

    char command = Serial.read();
    
    // SERVO Set command
    if ( command == 'S' || command == 's' )  // 'S' is Servo Command "S,Servo_no,Angle"
    {
      Serial.print(command);
      Serial.print(',');
      int servoNo = Serial.parseInt();
      Serial.print(servoNo);
      Serial.print(',');
      int servoAngle = Serial.parseInt();
      Serial.print(servoAngle);
      Serial.println();

      if ( 0 <= servoNo &&  servoNo < MAX_SERVO_NUM+1 )
      {
        // Single Servo Move
        servo[servoNo].write(servoAngle);

        // Wait Servo Move
        //delay(300); // 180/60(°/100msec）=300(msec)
      }
    }  // SERVO Set END

    // SERVO All command
    if ( command == 'A' || command == 'a' )  // 'A' is Servo All Command "A,Angle0, Angle1...Angle7"
    {
      Serial.print(command);

      // read Angle from Serial
      int angle[MAX_SERVO_NUM];
      for (int i = 0; i < MAX_SERVO_NUM; i++)
      {
        angle[i] = Serial.parseInt();
        Serial.print(',');
        Serial.print(angle[i]);
      }
      Serial.println();

      // servo Move
      set8Pos( angle[0], angle[1], angle[2], angle[3], angle[4], angle[5], angle[6], angle[7] );

    }

 //   detachServo(); 
 }
    
}//loop

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length();

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}  // END

// Set 8 ServoMotor angles
void set8Pos(int a, int b, int c, int d, int e, int f, int g, int h){
  servo[1].write(a);
  servo[2].write(b);
  servo[3].write(c);
  servo[4].write(d);
  servo[5].write(e);
  servo[6].write(f);
  servo[7].write(g);
  servo[8].write(h);
}

void attachServo()
{
  servo[1].attach(D1);
  servo[2].attach(D2);
  servo[3].attach(D3);
  servo[4].attach(D4);
  servo[5].attach(D5);
  servo[6].attach(D6);
  servo[7].attach(D7);
  servo[8].attach(D8);
}

void detachServo()
{
  servo[1].detach();
  servo[2].detach();
  servo[3].detach();
  servo[4].detach();
  servo[5].detach();
  servo[6].detach();
  servo[7].detach();
  servo[8].detach();
}

//== Lean_Left ============================================================================= 
void lean_left() {
  servo[2].write(40);
  servo[4].write(140);
  servo[6].write(140);
  servo[8].write(40);
  delay(700);
}

//== Lean_Right ============================================================================ 
void lean_right() {
  servo[2].write(140);
  servo[4].write(40);
  servo[6].write(40);
  servo[8].write(140);
  delay(700);
}

// trim
const int trimStep = 1;
//== trim_left ============================================================================= 
void trim_left() {
  da-=trimStep;
  // Left Front Pivot
  db-=trimStep;
  // Left Back Pivot
  dc-=trimStep;
  // Right Back Pivot
  dd-=trimStep;
  // Right Front Pivot
}

//== trim_Right ============================================================================ 
void trim_right() {
  da+=trimStep;
  // Left Front Pivot 
  db+=trimStep;
  // Left Back Pivot
  dc+=trimStep;
  // Right Back Pivot
  dd+=trimStep;
  // Right Front Pivot
}

//== Forward ===============================================================================
void forward() {
  // calculation of points 
  // Left Front Pivot 
  a90 = (90 + da), a120 = (120 + da), a150 = (150 + da), a180 = (180 + da);
  // Left Back Pivot
  b0 = (0 + db), b30 = (30 + db), b60 = (60 + db), b90 = (90 + db);
  // Right Back Pivot
  c90 = (90 + dc), c120 = (120 + dc), c150 = (150 + dc), c180 = (180 + dc);
  // Right Front Pivot
  d0 = (0 + dd), d30 = (30 + dd), d60 = (60 + dd), d90 = (90 + dd);
  // set servo positions and speeds needed to walk forward one step 
  // (LFP, LBP, RBP, RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4) 
  srv(a180, b0 , c120, d60, 42, 33, 33, 42, 1, 3, 1, 1);
  srv( a90, b30, c90, d30, 6, 33, 33, 42, 3, 1, 1, 1);
  srv( a90, b30, c90, d30, 42, 33, 33, 42, 3, 1, 1, 1);
  srv(a120, b60, c180, d0, 42, 33, 6, 42, 1, 1, 3, 1);
  srv(a120, b60, c180, d0, 42, 33, 33, 42, 1, 1, 3, 1);
  srv(a150, b90, c150, d90, 42, 33, 33, 6, 1, 1, 1, 3);
  srv(a150, b90, c150, d90, 42, 33, 33, 42, 1, 1, 1, 3);
  srv(a180, b0, c120, d60, 42, 6, 33, 42, 1, 3, 1, 1);
  //
  srv(a180, b0, c120, d60, 42, 15, 33, 42, 1, 3, 1, 1);
//  Serial.println("Forward");
  
}

//== Back ================================================================================== 
void back () {
  // set servo positions and speeds needed to walk backward one step 
  // (LFP, LBP, RBP, RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4) 
  srv(180, 0, 120, 60, 42, 33, 33, 42, 3, 1, 1, 1);
  srv(150, 90, 150, 90, 42, 18, 33, 42, 1, 3, 1, 1);
  srv(150, 90, 150, 90, 42, 33, 33, 42, 1, 3, 1, 1);
  srv(120, 60, 180, 0, 42, 33, 33, 6, 1, 1, 1, 3);
  srv(120, 60, 180, 0, 42, 33, 33, 42, 1, 1, 1, 3);
  srv(90, 30, 90, 30, 42, 33, 18, 42, 1, 1, 3, 1);
  srv(90, 30, 90, 30, 42, 33, 33, 42, 1, 1, 3, 1);
  //
//  srv(180, 0, 120, 60, 6, 33, 33, 42, 3, 1, 1, 1);
  srv(160,20, 120, 60, 18, 33, 33, 42, 3, 1, 1, 1);
  
}

//== Left =================================================================================
void turn_left () {
  // set servo positions and speeds needed to turn left one step 
  // (LFP, LBP, RBP, RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4) 
  srv(150, 90, 90, 30, 42, 6, 33, 42, 1, 3, 1, 1);
  srv(150, 90, 90, 30, 42, 33, 33, 42, 1, 3, 1, 1); 
  srv(120, 60, 180, 0, 42, 33, 6, 42, 1, 1, 3, 1);
  srv(120, 60, 180, 0, 42, 33, 33, 24, 1, 1, 3, 1);
  srv(90, 30, 150, 90, 42, 33, 33, 6, 1, 1, 1, 3);
  srv(90, 30, 150, 90, 42, 33, 33, 42, 1, 1, 1, 3);
  srv(180, 0, 120, 60, 6, 33, 33, 42, 3, 1, 1, 1);
  srv(180, 0, 120, 60, 42, 33, 33, 33, 3, 1, 1, 1);
  
}

//== Right ================================================================================
void turn_right () { 
  // set servo positions and speeds needed to turn right one step 
  // (LFP, LBP, RBP, RFP, LFL, LBL, RBL, RFL, S1, S2, S3, S4) 
  srv( 90, 30, 150, 90, 6, 33, 33, 42, 3, 1, 1, 1);
  srv( 90, 30, 150, 90, 42, 33, 33, 42, 3, 1, 1, 1);
  srv(120, 60, 180, 0, 42, 33, 33, 6, 1, 1, 1, 3);
  srv(120, 60, 180, 0, 42, 33, 33, 42, 1, 1, 1, 3);
  srv(150, 90, 90, 30, 42, 33, 6, 42, 1, 1, 3, 1);
  srv(150, 90, 90, 30, 42, 33, 33, 42, 1, 1, 3, 1);
  srv(180, 0, 120, 60, 42, 6, 33, 42, 1, 3, 1, 1);
  srv(180, 0, 120, 60, 42, 33, 33, 42, 1, 3, 1, 1);
  
}

//== Right Step================================================================================
void step_right () {
  servo[1].write(130);
  servo[3].write(50);
  servo[5].write(130);
  servo[7].write(50);
  delay(500);
  servo[2].write(130);
  servo[4].write(50);
  servo[6].write(40);
  servo[8].write(140);
  delay(300);
  servo[2].write(90);
  servo[4].write(90);
  servo[6].write(140);
  servo[8].write(40);
  delay(150);
  servo[2].write(130);
  servo[4].write(50);
  servo[6].write(40);
  servo[8].write(140);
  delay(700);
  
}
//== Left Step ================================================================================
void step_left () { 
  servo[1].write(130);
  servo[3].write(50);
  servo[5].write(130);
  servo[7].write(50);
  delay(500);
  servo[2].write(40);
  servo[4].write(140);
  servo[6].write(130);
  servo[8].write(50);
  delay(300);
  servo[2].write(140);
  servo[4].write(40);
  servo[6].write(90);
  servo[8].write(90);
  delay(150);
  servo[2].write(40);
  servo[4].write(140);
  servo[6].write(130);
  servo[8].write(50);
  delay(700);
  
}

//== Center Servos ======================================================================== 
void center_servos() {
  servo[1].write(90);
  servo[3].write(90);
  servo[5].write(90);
  servo[7].write(90);
  delay(500);
  servo[2].write(90);
  servo[4].write(90);
  servo[6].write(90);
  servo[8].write(90);
  delay(500);
  
}

//== minimal Servos ======================================================================== 
void minimal() {
  servo[2].write(140);
  servo[4].write(40);
  servo[6].write(140);
  servo[8].write(40);
  delay(300);
  servo[1].write(140);
  servo[3].write(40);
  servo[5].write(140);
  servo[7].write(40);
  delay(700);
  
}

//== Fighting Stance ======================================================================== 
void fightst() {
  servo[1].write(140);
  servo[3].write(90);
  servo[5].write(90);
  servo[7].write(40);
  servo[2].write(90);
  servo[4].write(90);
  servo[6].write(90);
  servo[8].write(90);
  delay(500);
  
}

//== lay ======================================================================== 
void lay() {
  servo[2].write(0);
  servo[4].write(180);
  servo[6].write(0);
  servo[8].write(180);
  delay(300);
  servo[1].write(140);
  servo[3].write(40);
  servo[5].write(140);
  servo[7].write(40);
  delay(700);
  
}

//== bow ======================================================================== 
void bow() {
  servo[1].write(140);
  servo[3].write(90);
  servo[5].write(90);
  servo[7].write(40);
  servo[2].write(90);
  servo[4].write(90);
  servo[6].write(90);
  servo[8].write(90);
  delay(300);
  servo[2].write(50);
  servo[8].write(130);
  delay(300);
  servo[2].write(90);
  servo[8].write(90);
  delay(300);
  servo[2].write(50);
  servo[8].write(130);
  delay(300);
  servo[2].write(90);
  servo[8].write(90);
  delay(700);
  
}

//== side to side ======================================================================== 
void side2side() {
  lean_left();
  lean_right();
  lean_left();
  lean_right();
  
}
//== Increase Speed ========================================================================
void increase_speed() {
  if (spd > 3) spd--;
}
//== Decrease Speed ======================================================================== 
void decrease_speed() {
  if (spd < 50) spd++;
}
//== Srv ===================================================================================
void srv( int p11, int p21, int p31, int p41, int p12, int p22, int p32, int p42, int sp1, int sp2, int sp3, int sp4) {
  // p11: Front Left Pivot Servo 
  // p21: Back Left Pivot Servo 
  // p31: Back Right Pivot Servo 
  // p41: Front Right Pivot Servo 
  // p12: Front Left Lift Servo 
  // p22: Back Left Lift Servo 
  // p32: Back Right Lift Servo 
  // p42: Front Right Lift Servo 
  // sp1: Speed 1 
  // sp2: Speed 2 
  // sp3: Speed 3 
  // sp4: Speed 4 
  // Multiply lift servo positions by manual height adjustment 
  p12 = p12 + high * 3;
  p22 = p22 + high * 3;
  p32 = p32 + high * 3;
  p42 = p42 + high * 3;
  
  while ((s11 != p11) || (s21 != p21) || (s31 != p31) || (s41 != p41) || (s12 != p12) || (s22 != p22) || (se32 != p32) || (s42 != p42)) {
    // Front Left Pivot Servo
    if (s11 < p11) // if servo position is less than programmed position 
    {
      if ((s11 + sp1) <= p11) s11 = s11 + sp1; // set servo position equal to servo position plus speed constant 
      else s11 = p11;
    }
    if (s11 > p11) // if servo position is greater than programmed position 
    {
      if ((s11 - sp1) >= p11) s11 = s11 - sp1; // set servo position equal to servo position minus speed constant 
      else s11 = p11;
    }
    // Back Left Pivot Servo 
    if (s21 < p21) {
      if ((s21 + sp2) <= p21) s21 = s21 + sp2 ;
      else s21 = p21;
    }
    if (s21 > p21) {
      if ((s21 - sp2) >= p21) s21 = s21 - sp2; 
      else s21 = p21;
    }
    // Back Right Pivot Servo 
    if (s31 < p31) {
      if ((s31 + sp3) <= p31) s31 = s31 + sp3;
      else s31 = p31;
    }
    if (s31 > p31) {
      if ((s31 - sp3) >= p31) s31 = s31 - sp3; 
      else s31 = p31;
    }
    // Front Right Pivot Servo 
    if (s41 < p41) {
      if ((s41 + sp4) <= p41) s41 = s41 + sp4;
      else s41 = p41;
    }
    if (s41 > p41) {
      if ((s41 - sp4) >= p41) s41 = s41 - sp4;
      else s41 = p41;
    }
    // Front Left Lift Servo 
    if (s12 < p12) {
      if ((s12 + sp1) <= p12) s12 = s12 + sp1;
      else s12 = p12;
    }
    if (s12 > p12) {
      if ((s12 - sp1) >= p12) s12 = s12 - sp1;
      else s12 = p12;
    }
    // Back Left Lift Servo 
    if (s22 < p22) { 
      if ((s22 + sp2) <= p22) s22 = s22 + sp2;
      else s22 = p22;
    }
    if (s22 > p22) {
      if ((s22 - sp2) >= p22) s22 = s22 - sp2;
      else s22 = p22;
    }
    // Back Right Lift Servo 
    if (se32 < p32) {
      if ((se32 + sp3) <= p32) se32 = se32 + sp3;
      else se32 = p32;
    }
    if (se32 > p32) {
      if ((se32 - sp3) >= p32) se32 = se32 - sp3;
      else se32 = p32;
    }
    // Front Right Lift Servo 
    if (s42 < p42) {
      if ((s42 + sp4) <= p42) s42 = s42 + sp4;
      else s42 = p42;
    }
    if (s42 > p42) {
      if ((s42 - sp4) >= p42) s42 = s42 - sp4;
      else s42 = p42;
    }
    // Write Pivot Servo Values 
    servo[1].write(s11 + da);
    servo[3].write(s21 + db);
    servo[5].write(s31 + dc);
    servo[7].write(s41 + dd);
    // Write Lift Servos Values 
    servo[2].write(s12);
    servo[4].write(s22);
    servo[6].write(se32);
    servo[8].write(s42);
    delay(spd);
    // Delay before next movement
  }// while
}//srv


