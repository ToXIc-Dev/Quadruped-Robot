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

//Arduroid4 (Top View)
//FLONT
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
  
//===== Globals ============================================================================ 
/* define port */
//WiFiClient client;
ESP8266WebServer server(80);

String Hostname = "Quad"; // The network name
char* Hostname2 = &Hostname[0];

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
int spd = 3;// Speed of walking motion, larger the number, the slower the speed
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
<button onclick="SendCommand('lay')">Lay</button> | <button onclick="SendCommand('s2s')">Side to Side</button> 
<script>
function SendCommand(Command) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
  };
  xhttp.open("GET", "/" + Command + "", true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";

void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

//===== Setup ============================================================================== 
void setup() {
  Serial.begin (9600);
  WiFi.hostname(Hostname2);
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.autoConnect(Hostname2);

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
  server.on("/forward", forward );
  server.on("/backward", back );
  server.on("/left", turn_left );
  server.on("/right", turn_right );
  server.on("/stop", center_servos );
  server.on("/minimal", minimal );
  server.on("/leanl", lean_left );
  server.on("/leanr", lean_right );
  server.on("/stepl", step_left );
  server.on("/stepr", step_right );
  server.on("/lay", lay );
  server.on("/s2s", side2side );
  
  server.begin();                                       //Start the server
  Serial.println("Server listening");   
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
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
  server.send(200, "text/plain", "Recieved");
}

//== side to side ======================================================================== 
void side2side() {
  lean_left();
  lean_right();
  lean_left();
  lean_right();
  server.send(200, "text/plain", "Recieved");
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


