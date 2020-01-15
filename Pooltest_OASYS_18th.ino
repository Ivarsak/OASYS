
#include <SPI.h> 
#include <Servo.h>
#include <SD.h> 
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "MS5837.h"
#include "TSYS01.h"
#include "BNO055_support.h"    

MS5837 sensor;
TSYS01 sensor1;

Servo ESC, Valve; 
WiFiClient espClient;
PubSubClient client(espClient);
 
void Bladder_to_tank();
void Tank_to_Bladder();
void Setup_Loop();
void Depth_Feedback();
void IMU_Update();

int T2B = 1900, B2T = 1100, StopPump = 1500;            // Pump Control
int Valve_open = 40, Valve_closed = 100;                // valve Control
int DelaY_UP = 10000, Delay_DOWN = 10000;               // based on the time the glider will take from surface to bottom and vice versa
int DelaY_PUMP_T2B = 27000, DelaY_PUMP_B2T = 4000;     // defining pumping time 
int SlowStart, SlowStop, dly = 10;                      // define smooth start and stop for motor
int EscPin = 2, ValvePin = 0;
int k=0;
float Max_Depth, Depth, Temp, Max_Depth_Callback = 0;

struct bno055_t myBNO;
struct bno055_euler myEulerData; //Structure to hold the Euler data

const char* ssid = "OASYS";
const char* password = "Oasyspooltest";
const char* mqtt_server = "broker.hivemq.com";

const int chipSelect = 15;
const unsigned long period = 10000; 
unsigned long startMillis, LastTime=0;  

File myFile;

long lastMsg = 0;
char msg[50], msg1[50],  msg2[50],  msg3[50], msg4[50], msg5[50], msg6[50], msg7[50];
int value, value1 = 0;



// --------------------- Setup ---------------------------------------------------------------------------

void setup() {
  
 Setup_Loop(); // This function initializes WIFI modem and Micro-SD card

  ESC.attach(2);                     // Defining the ESC PWM-pin
  Valve.attach(0);                 // Defining the servo PWM-pin              
  Valve.write(Valve_closed);              // initialize valve condition
  ESC.writeMicroseconds(StopPump);        // Arming the brushless motor
  delay(6500);                            // delay 7 seconds for the ESC
 
 //Initialization of the BNO055
  BNO_Init(&myBNO); //Assigning the structure to hold information about the device
  
  //Configuration to NDoF mode
  bno055_set_operation_mode(OPERATION_MODE_NDOF);

  delay(1);
}


// --------------- Main loop ------------------------------------------------------------------

void loop() {


  if (!client.connected()) {
    reconnect();
  }

  
  client.loop(); // Initializes callback function

  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "Surface Detected #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("Oasys/Subscribe", msg);

  if (value1 < 3)

  {

    ++value1;
    snprintf (msg7, 50, "Max Depth vaule = %lf2.2", Max_Depth_Callback);
    Serial.print("Publish message: ");
    Serial.println(msg7);
    client.publish("Oasys/Subscribe", msg7);   
  }
    
  }      
 LogData();
 IMU_Update();
    
  }


// --------------------- Functions ----------------------------------------------------------------------------



  void reconnect() {

  unsigned long RECT, REST; // Timer variables
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("Oasys/Subscribe", "Surface Detected");
      // ... and resubscribe
      client.subscribe("Oasys/Publish");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

 RECT = millis();
  REST = RECT;
      
  while(RECT - REST < 5000)

  {
  
    RECT = millis();
    delay(1);
    LogData();
  }
     
    }
  }
  
}


void callback(char* topic, byte* payload, unsigned int length) {

  char buff[]= "";
  
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

// Storing the incomming message in strings, one string for each character of payload[i]

  String var1 = (String)payload[0]; // delay up/down or Max depth limit 
  String var2 = (String)payload[1]; // Max Depth limit
  String var3 = (String)payload[2]; // Max Depth limit
  String var4 = (String)payload[3]; //DelaY_PUMP_B2T
  String var5 = (String)payload[4]; //DelaY_PUMP_B2T
  String var6 = (String)payload[5]; //DelaY_PUMP_T2B
  String var7 = (String)payload[6]; //DelaY_PUMP_T2B
  
// Converting the incomming message from string to int. Need to subtract 48 (zero in character) from the value as the orginial message is in decimal from the ASCII table
// See chart https://www.arduino.cc/en/Reference/ASCIIchart

  int v1 = var1.toInt()-48;       
  int v2 = var2.toInt()-48;
  int v3 = var3.toInt()-48;
  int v4 = var4.toInt()-48;
  int v5 = var5.toInt()-48;
  int v6 = var6.toInt()-48;
  int v7 = var7.toInt()-48;

 // Adding strings together to form a double digit number and 3 digit number including "." for the Max_Depth value 

  String d1 = String(v4) + String (v5);  // Delay to pump oil from external bladder to internal tank  (Glider dives) 
  String d2 = String(v6) + String (v7);  // Delay to pump oil from interlan tank to external bladder  (Glider rises)
  String d3 = String(v1) + String (v2);
  
  DelaY_PUMP_B2T = d1.toInt();  // String to int
  DelaY_PUMP_T2B = d2.toInt();  // String to int

  if (var4 == "0")        // if the the first delay will be a single digit number (0-9) 
  {
  DelaY_PUMP_B2T = v5;
  }

  if (var2 != "46")
  {
    if (var3 == 0)

    {
         DelaY_UP = v1*10000;
         Delay_DOWN = v1*10000;     
    }

    else
    {
         DelaY_UP = d3.toInt()*1000;
         Delay_DOWN = d3.toInt()*1000;
    }
  }


 Serial.println("Payload");
 Serial.print("diving delay [sec]"); Serial.print(":"); Serial.println(v1*10);
 Serial.print("pump interval in [millisec]"); Serial.print(":"); Serial.println( DelaY_PUMP_B2T*1000);
 Serial.print("pump interval out [millisec]"); Serial.print(":"); Serial.println(DelaY_PUMP_T2B*1000);




if( var2 != "46" && v3 == 0)   // This will be true if we send a timer command to control the glider

    {

      // Confirming the delay values by publishing back on MQTT topic

     snprintf (msg1, 50, "Diving delay  = %ld sec", Delay_DOWN);
 Serial.print("Publish message: ");
 Serial.println(msg1);
 client.publish("Oasys/Subscribe", msg1);

 delay(100);

 snprintf (msg2, 50, "Pump interval IN = %ld millisec" ,  DelaY_PUMP_B2T*1000);
 Serial.print("Publish message: ");
 Serial.println(msg2);
 client.publish("Oasys/Subscribe", msg2);

 delay(100);

 snprintf (msg3, 50, "Pump interval OUT = %ld millisec", DelaY_PUMP_T2B*1000);
 Serial.print("Publish message: ");
 Serial.println(msg3);
 client.publish("Oasys/Subscribe", msg3);

 delay(10);
 
    }

 // Scaling delay values from seconds to milliseconds
 
 DelaY_PUMP_B2T = DelaY_PUMP_B2T*1000;
 DelaY_PUMP_T2B = DelaY_PUMP_T2B*1000;
 
   if( var2 != "46" && v3 == 0)   // This will be true if we use a timer to control the glider

    {
      Tank_to_Bladder();
      Bladder_to_tank();
      
    }


    if( var2 == "46")     // 46 is dot symbol in ASCII table. 46 ---> ".", this is used to denote the fractional and integer part

    {
      sprintf(buff, "%d.%d", v1,v3);  // Using the sprintf function to add together the desired value for the Max Depth limit, stored in a char buffer
      Max_Depth = atof(buff);         // Converting char to float

      snprintf (msg3, 50, "Max Depth Limit = %lf Meters", Max_Depth);
      Serial.print("Publish message: ");
      Serial.println(msg3);
      client.publish("Oasys/Subscribe", msg3);
      delay(10);

      
      DelaY_UP = 0;
      Delay_DOWN = 0;
      
     Tank_to_Bladder(); // Pumps oil from external bladder to internal bladder (Glider Dives)
     Depth_Feedback(); // Checks if the max depth limit is reached
      
  
    }
   
 value1 = 0;
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void LogData()

{
  unsigned long CT =0;

  if ((millis() - CT) >= 300)

  {
    Serial.println("LogData()");
       
 CT = millis();
 
 
 sensor.read();
 sensor1.read();
   
 Depth = sensor.depth();
 Temp = sensor1.temperature();

 if (Depth > Max_Depth_Callback)

 {
   Max_Depth_Callback = Depth;
   Serial.println("ny max_depth_callback");

 }
 
 delay(100);
  
  myFile = SD.open("Depth.txt", FILE_WRITE);
  if (myFile) {

    myFile.print(CT);
    myFile.print(',');
    myFile.print(Depth);
    myFile.println("");
  }
  myFile.close();
  delay(100);
 
  myFile = SD.open("Temp.txt", FILE_WRITE);
  if (myFile) {

    myFile.print(CT);
    myFile.print(',');   
    myFile.print(Temp);
    myFile.println("");
  }
  myFile.close();


 } 

 }

 void Setup_Loop ()


{
   
  Serial.begin(115200);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  Wire.begin();

  startMillis = millis();  //initial start time

 
  // setup for the SD card
  Serial.print("Initializing SD card...");

  if(!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
    
  //open file
  myFile=SD.open("Depth.txt", FILE_WRITE);

  // if the file opened ok, write to it:
  if (myFile) {
    Serial.println("File opened ok");
    // print the headings for our data
    myFile.println("Timestamp,Depth");
  }
  myFile.close();


   while (!sensor.init()) {
    Serial.println("Init failed!");
    Serial.println("Are SDA/SCL connected correctly?");
    Serial.println("Blue Robotics Bar30: White=SDA, Green=SCL");
    Serial.println("\n\n\n");
    delay(500);
  }
  
  sensor.setModel(MS5837::MS5837_30BA);
  sensor.setFluidDensity(997); // kg/m^3 (freshwater, 1029 for seawater)

   sensor1.init();

  
}


 void Tank_to_Bladder()
   {
unsigned long SM, SM1, SM2, CM, CM1, CM2;  // SM = StartMillis() and CM = Current millis
 
    
  // Start pumping from tank to bladder
 if (T2B>B2T) {
  
 SlowStart = StopPump;
 SlowStop = T2B;
 Valve.write(Valve_open);                 //      Open Valve
 delay(100);                              //      Wait 0.1 second before turning on the Motor
 for (SlowStart = StopPump; SlowStart < T2B; SlowStart++) {
    ESC.writeMicroseconds(SlowStart);
    delay(dly);
  }      
    
 //      Motor = on --> Tank to Ballast
 
  CM = millis();
  SM = CM;

  while(CM - SM < DelaY_PUMP_T2B)  // By using millis() instead of delay(), we do not block the code, thus logging data is possible

  {
    Serial.println("T2B");
    LogData();

    CM = millis();
    delay(1);         // This delay avoids Watchdog timer (WDT) reset on the ESP8266
  } 

 Valve.write(Valve_closed);               //      Close valve
 delay(100);                              //      Wait 0.1 second before turning off the Motor
 for (SlowStop = T2B; SlowStop > StopPump; SlowStop--) {
    ESC.writeMicroseconds(SlowStop);
    delay(dly);
  }                                       //      Motor = off
  }
  
  CM1 = millis();
  SM1 = CM1;

  while(CM1 - SM1 < Delay_DOWN)

  {
    Serial.println("Delay down");

    CM1 = millis();
    delay(1);
    LogData();
  }


}

void Bladder_to_tank()
 
 {
  unsigned long CMM, CMM1, CMM2, SMM, SMM1, SMM2;   // SMM = StartMillis() and CMM = CurrentMillis()

 // Start pumping from bladder to tank

 Valve.write(Valve_open);                 //Open Valve
 
 CMM = millis();
 SMM = CMM;

  while(CMM - SMM < 500)

  {

    CMM = millis();
    Serial.println("before B2T");
    delay(1);
    LogData();
  }


  if (B2T<T2B) {
  SlowStart = StopPump;
  SlowStop = B2T;
  for (SlowStart = StopPump; SlowStart > B2T; SlowStart--) {
    ESC.writeMicroseconds(SlowStart);
    delay(dly);
  }   
  
  //      Motor = on --> Ballast to Tank

  CMM1 = millis();
  SMM1 = CMM1;

  while(CMM1 - SMM1 < DelaY_PUMP_B2T)
  {
    
    Serial.println("B2T");
    delay(1);
    CMM1 = millis();
    LogData();
   
  }

 Valve.write(Valve_closed);               //      Close valve
 delay(100);                              //      Wait 0.5 second before turning off the Motor
 for (SlowStop = B2T; SlowStop < StopPump; SlowStop++) {
    
    ESC.writeMicroseconds(SlowStop);
    delay(dly);
  }                                       //      Motor = off
  }

  CMM2 = millis();
  SMM2 = CMM2;

  while(CMM2 - SMM2 < DelaY_UP)
  {
    CMM2 = millis();
    Serial.println("Delay UP");
    delay(1);
    LogData(); 
  }
  }
  
  
 void Depth_Feedback()

 {
   sensor.read();
   while (Max_Depth > sensor.depth())
   
   {
     LogData();
     Serial.print("Depth ="); Serial.println(sensor.depth());
   }
   
    Bladder_to_tank();
 }


void IMU_Update()

{

  
if ((millis() - LastTime) >= 100) //To stream at 10Hz without using additional timers
  {
    LastTime = millis();

    bno055_read_euler_hrp(&myEulerData);      //Update Euler data into the structure

    Serial.print("Time Stamp: ");       //To read out the Time Stamp
    Serial.println(LastTime);

    Serial.print("Heading(Yaw): ");       //To read out the Heading (Yaw)
    Serial.println(float(myEulerData.h) / 16.00);   //Convert to degrees

    Serial.print("Roll: ");         //To read out the Roll
    Serial.println(float(myEulerData.r) / 16.00);   //Convert to degrees

    Serial.print("Pitch: ");        //To read out the Pitch
    Serial.println(float(myEulerData.p) / 16.00);   //Convert to degrees

    Serial.println();         //Extra line to differentiate between packets
  }

  }













  



 
 
