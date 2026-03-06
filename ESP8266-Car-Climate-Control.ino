#if __has_include("private.h")
    #include "private.h"
#endif


static inline int8_t sign(int val) {
  if (val < 0) return -1;
  if (val==0) return 0;
  return 1;
}





///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////PINS

#define POT_PIN A0
#define SERVO_PIN D8
#define THERMO_PIN D6
#define LED 2
#define FLASH_BUTTON_PIN 0

#define IN1 D7   //"Engine On" Signal
#define OUT1 D5  //headlights






////////////////////////////////////////////////////////////////////////////////
///////////////////////////////CONFIG

#define CONFIG_ARRAY_SIZE 11

#define SERVO_POS_MIN_CARRAY_INDEX 0
#define SERVO_POS_MAX_CARRAY_INDEX 1

#define SERVO_VOLTAGE_MIN_CARRAY_INDEX 2
#define SERVO_VOLTAGE_MAX_CARRAY_INDEX 3

#define POT_POS_MIN_CARRAY_INDEX 4
#define POT_POS_MAX_CARRAY_INDEX 5

//p for parameter
#define P_TOLERANCE_CARRAY_INDEX 6
#define P_MAX_STEPSIZE_CARRAY_INDEX 7
#define P_STEP_DELAY_CARRAY_INDEX 8
#define P_OUTDOORS_T_DEPENDANCY_CARRAY_INDEX 9
#define P_DIRECT_CONTROL_CARRAY_INDEX 10

int config_array[CONFIG_ARRAY_SIZE] = {0};

void reset_config(){
  config_array[SERVO_POS_MIN_CARRAY_INDEX]=0;
  config_array[SERVO_POS_MAX_CARRAY_INDEX]=2500;

  config_array[SERVO_VOLTAGE_MIN_CARRAY_INDEX]=33;
  config_array[SERVO_VOLTAGE_MAX_CARRAY_INDEX]=1700;

  config_array[POT_POS_MIN_CARRAY_INDEX]=0;
  config_array[POT_POS_MAX_CARRAY_INDEX]=1024;

  config_array[P_TOLERANCE_CARRAY_INDEX]=3;
  config_array[P_MAX_STEPSIZE_CARRAY_INDEX]=5;
  config_array[P_STEP_DELAY_CARRAY_INDEX]=2000;
  config_array[P_OUTDOORS_T_DEPENDANCY_CARRAY_INDEX]=0;
  config_array[P_DIRECT_CONTROL_CARRAY_INDEX]=0;
}



///////////////////////////////////////////////////////////////////////////////
/////DATA READINGS
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1015 ads;    // задаём тип АЦП


#define DATA_ARRAY_SIZE 13

#define OUTDOOR_T_DARRAY_INDEX 0
#define STREAM_T_DARRAY_INDEX 1
#define INTERIOR_T_DARRAY_INDEX 2
#define DESIRED_STREAM_T_DARRAY_INDEX 3
#define DESIRED_INTERIOR_T_DARRAY_INDEX 4

#define POT_VALUE_DARRAY_INDEX 5
#define POT_PERCENTAGE_DARRAY_INDEX 6

#define SERVO_VOLTAGE_DARRAY_INDEX 7
#define SERVO_TARGET_POSITION_DARRAY_INDEX 8
#define SERVO_REAL_POSITION_DARRAY_INDEX 9
#define SERVO_TARGET_PERCENTAGE_DARRAY_INDEX 10 
#define SERVO_REAL_PERCENTAGE_DARRAY_INDEX 11

#define LIGHT_LEVEL_DARRAY_INDEX 12


#define POT_ARRAY_SIZE 5 //pot readings smoothing array size


int data_array[DATA_ARRAY_SIZE] = {0};

void GetReadings() {
  float pot_value = readPot(analogRead(POT_PIN));
  data_array[POT_VALUE_DARRAY_INDEX] = pot_value;
  data_array[POT_PERCENTAGE_DARRAY_INDEX] 
  = 100 * (pot_value - config_array[POT_POS_MIN_CARRAY_INDEX]) 
  / (config_array[POT_POS_MAX_CARRAY_INDEX] - config_array[POT_POS_MIN_CARRAY_INDEX]);

  float servo_voltage=ads.readADC_SingleEnded(0);

  data_array[SERVO_VOLTAGE_DARRAY_INDEX]=servo_voltage;
}

float readPot(int new_value) {
  static int smoothing_array[POT_ARRAY_SIZE] = { 0 };
  static int cursor = 0;
  float smooth_value = 0;
  smoothing_array[cursor] = new_value;
  cursor = (cursor + 1) % POT_ARRAY_SIZE;  //увеличение на 1 и округление до размеров массива
  for (int i = 0; i < POT_ARRAY_SIZE; i++) {
    smooth_value += smoothing_array[i];
  }
  return smooth_value / POT_ARRAY_SIZE;
}

void printData(){
  for(int Index=0; Index<DATA_ARRAY_SIZE; ++Index){
    Serial.print(data_array[Index]);
    Serial.print("\t");
  }
  Serial.println();
}






/////////////////////////////////////////////////////////////////
////////SENSORS

#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(THERMO_PIN);
DallasTemperature sensors(&oneWire);

uint8_t sensor_outdoors[8] = { 0x28, 0xA6, 0xC1, 0xD5, 0x0E, 0x00, 0x00, 0x6B };  //OUTDOORS TEMPERATURE
uint8_t sensor_airstream[8] = { 0x28, 0xFF, 0x7D, 0x11, 0xC4, 0xA1, 0x95, 0xB7 };  //AIR STREAM TEMPERATURE
uint8_t sensor_interior[8] = { 0x28, 0xFF, 0x03, 0xC3, 0x80, 0x16, 0x05, 0xAC };  //INTERIOR TEMPERATURE


void findSensors() {
  int thermo_devices_count = 0;
  DallasTemperature sensors(&oneWire);
  DeviceAddress Thermometer;
  sensors.begin();
  Serial.print("Locating devices... \n Found ");
  thermo_devices_count = sensors.getDeviceCount();
  Serial.print(thermo_devices_count, DEC);
  Serial.println(" devices.");
  Serial.println("Printing addresses...");
  for (int i = 0; i < thermo_devices_count; i++) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);
    for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (Thermometer[i] < 0x10) Serial.print("0");
    Serial.print(Thermometer[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
  }
}







///////////////////////////////////////////////////////////////////////////////
////Wifi Server

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "webpage.h"
#ifndef APSSID
  #define APSSID "ESP8266-Climate-Control"
  #define APPSK "password"
#endif


const char *ssid = APSSID;
const char *password = APPSK;

IPAddress my_ip = 0;
ESP8266WebServer server(80);

void setupWifiServer(){
  Serial.println();
  Serial.println();
  Serial.println("Configuring access point...");
  Serial.println();
  WiFi.softAP(ssid, password);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASSWORD: ");
  Serial.println(password);

  my_ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(my_ip);

  server.on("/", handleRoot);
  server.begin();

  Serial.println();
  Serial.println("HTTP server started");
}

void handleRoot() {
  server.send(200, "text/html", root_webpage);
}









////////////////////////////////////////////////////////////////////////////////
////////DISPLAY, currently not implemented but used to flush I2C when dealing with servo

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET LED_BUILTIN  //4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Hello World!");
  display.display();
}










/////////////////////////////////////////////////////////////////////////////////
//////Servo

#include <Servo.h>

bool is_servo_attached=false;
Servo my_servo;  // create servo object to control a servo

#define MAX_STEP 8            //maximum step size when moving servo slowly, used to control level of noise (bigger steps can lead to screeching)
#define EPSILON_SERVO_POS 10   //maximum deviance between target position and real position (not in percent, in PWM frequency probably)

//slowly move servo to data_array[SERVO_TARGET_PERCENTAGE_DARRAY_INDEX] position
void moveServo()  {

  

  if(
    abs(data_array[SERVO_TARGET_POSITION_DARRAY_INDEX]-data_array[SERVO_REAL_POSITION_DARRAY_INDEX])>EPSILON_SERVO_POS //if real servo pos differs from needed position
    )
    {
      int new_servo_real_position=
        data_array[SERVO_REAL_POSITION_DARRAY_INDEX]
        +sign(
          data_array[SERVO_TARGET_POSITION_DARRAY_INDEX]-data_array[SERVO_REAL_POSITION_DARRAY_INDEX]
          )
        *constrain(
          abs(
            data_array[SERVO_TARGET_POSITION_DARRAY_INDEX]-data_array[SERVO_REAL_POSITION_DARRAY_INDEX]
            )
            ,1,MAX_STEP
          );


    my_servo.writeMicroseconds(new_servo_real_position);
    display.display();

    data_array[SERVO_REAL_POSITION_DARRAY_INDEX]=new_servo_real_position;
    data_array[SERVO_REAL_PERCENTAGE_DARRAY_INDEX]
      = 100*(new_servo_real_position-config_array[SERVO_POS_MIN_CARRAY_INDEX])
      /(config_array[SERVO_POS_MAX_CARRAY_INDEX]-config_array[SERVO_POS_MIN_CARRAY_INDEX]);

    if (is_servo_attached == false) {
      my_servo.attach(SERVO_PIN, 0, 2500);
      is_servo_attached = true;
      Serial.println(" servo attached ");
    }
  }
  else{
    if (is_servo_attached == true) {
      display.display();
      delay(300);
      my_servo.detach();
      display.display();
      is_servo_attached = false;
      Serial.println(" servo detached ");
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////
////////////// CLIMATE CONTROL

void ChooseTargetServoPosition(){
  //DIRECT CONTROL
    if(config_array[P_DIRECT_CONTROL_CARRAY_INDEX]==1)
    {
      data_array[SERVO_TARGET_PERCENTAGE_DARRAY_INDEX]=data_array[POT_PERCENTAGE_DARRAY_INDEX];
    }else{
      StepSearch();

      
    }
    data_array[SERVO_TARGET_POSITION_DARRAY_INDEX] 
    = config_array[SERVO_POS_MIN_CARRAY_INDEX] 
    + data_array[SERVO_TARGET_PERCENTAGE_DARRAY_INDEX] * (config_array[SERVO_POS_MAX_CARRAY_INDEX] - config_array[SERVO_POS_MIN_CARRAY_INDEX] )/100;
}


void StepSearch(){

}





///////////////////////////////////////////////////////////////////////////////
////////////CALIBRATION MODE

#define SERVO_MIDDLE_POSITION 2000


bool isCalibrationModeOn = false;

//CM for calibration mode
int CM_servo_pos_min=0;
int CM_servo_pos_max=0;

int CM_servo_voltage_min=0;
int CM_servo_voltage_max=0;

int CM_pot_pos_min=0;
int CM_pot_pos_max=0;

void StartCalibration(){
  isCalibrationModeOn = true;
  CM_servo_pos_min=SERVO_MIDDLE_POSITION;
  CM_servo_pos_max=SERVO_MIDDLE_POSITION;

  CM_servo_voltage_min=data_array[SERVO_VOLTAGE_DARRAY_INDEX];
  CM_servo_voltage_max=data_array[SERVO_VOLTAGE_DARRAY_INDEX];

  CM_pot_pos_min=data_array[POT_VALUE_DARRAY_INDEX];
  CM_pot_pos_max=data_array[POT_VALUE_DARRAY_INDEX];

  Serial.println("");
  Serial.println("Starting Calibration...");
  Serial.println("");
}


void CalibrationLoop(){
  
  
  CM_pot_pos_min=min(CM_pot_pos_min,data_array[POT_VALUE_DARRAY_INDEX]);
  CM_pot_pos_max=max(CM_pot_pos_max,data_array[POT_VALUE_DARRAY_INDEX]);

  CM_servo_voltage_min=min(CM_servo_voltage_min,data_array[SERVO_VOLTAGE_DARRAY_INDEX]);
  CM_servo_voltage_max=max(CM_servo_voltage_max,data_array[SERVO_VOLTAGE_DARRAY_INDEX]);

  
  Serial.println("");
  Serial.print("Servo pos:");
  Serial.print(CM_servo_pos_min);
  Serial.print("\t-\t");
  Serial.print(CM_servo_pos_max);

  Serial.print("\t\tServo voltage:");
  Serial.print(CM_servo_voltage_min);
  Serial.print("\t-\t");
  Serial.print(CM_servo_voltage_max);

  Serial.print("\t\tPot position:");
  Serial.print(CM_pot_pos_min);
  Serial.print("\t-\t");
  Serial.println(CM_pot_pos_max);

  if(digitalRead(FLASH_BUTTON_PIN)==LOW){

    EndCalibration();
  }
}

#define VOLTAGE_EPSILON 5
#define VOLTAGE_BORDERS 30
#define STEP 5
#define T 25000

void EndCalibration(){

  CM_servo_voltage_min=max(CM_servo_voltage_min,35);
  CM_servo_voltage_max=min(CM_servo_voltage_max,1600);
  

  Serial.println("");
  Serial.println("Ending Calibration...");
  Serial.println("");


  //moveServo to middle position
  my_servo.attach(SERVO_PIN, 0, 2500);
  is_servo_attached = true;
  Serial.println(" servo attached ");
  delay(100);
  my_servo.writeMicroseconds(CM_servo_pos_min);
  display.display();
  delay(500);


  static int timer=millis();
  while(millis()-timer<T){
    


    Serial.println("");
    Serial.print(millis()-timer);
    Serial.print("/");
    Serial.print(T);
    
    Serial.print("Servo pos:");
    Serial.print(CM_servo_pos_min);
    Serial.print(" - ");
    Serial.print(CM_servo_pos_max);

    Serial.print("\t\tServo voltage:\t");

    Serial.print(data_array[SERVO_VOLTAGE_DARRAY_INDEX]);
    Serial.print("<< | ");
    Serial.print(CM_servo_voltage_min);
    Serial.print(" - ");
    Serial.println(CM_servo_voltage_max);

    data_array[SERVO_TARGET_POSITION_DARRAY_INDEX]=CM_servo_pos_min;
    CM_servo_pos_min-=STEP;
    my_servo.writeMicroseconds(CM_servo_pos_min);
    display.display();
    GetReadings();

    if(data_array[SERVO_VOLTAGE_DARRAY_INDEX]-CM_servo_voltage_min<VOLTAGE_EPSILON){

      Serial.println("Found minimum...");
      break;
    }
  }
  CM_servo_pos_max=CM_servo_pos_min;
  CM_servo_voltage_min=data_array[SERVO_VOLTAGE_DARRAY_INDEX];

  timer=millis();
  while(millis()-timer<T){
    


    Serial.println("");
    Serial.print(millis()-timer);
    Serial.print("/");
    Serial.print(T);
    
    Serial.print("Servo pos:");
    Serial.print(CM_servo_pos_min);
    Serial.print(" - ");
    Serial.print(CM_servo_pos_max);

    Serial.print("\t\tServo voltage:\t");

    Serial.print(data_array[SERVO_VOLTAGE_DARRAY_INDEX]);
    Serial.print(">> | ");
    Serial.print(CM_servo_voltage_min);
    Serial.print(" - ");
    Serial.println(CM_servo_voltage_max);

    data_array[SERVO_TARGET_POSITION_DARRAY_INDEX]=CM_servo_pos_max;
    CM_servo_pos_max+=STEP;
    my_servo.writeMicroseconds(CM_servo_pos_max);
    display.display();
    GetReadings();

    if(CM_servo_voltage_max - data_array[SERVO_VOLTAGE_DARRAY_INDEX] < VOLTAGE_EPSILON){

      Serial.println("Found maximum...");
      break;
    }
  }
  CM_servo_voltage_max=data_array[SERVO_VOLTAGE_DARRAY_INDEX];


  Serial.println("End");
  my_servo.detach();
  is_servo_attached = false;


  data_array[SERVO_REAL_POSITION_DARRAY_INDEX]=CM_servo_pos_min;



  Serial.println("");
  Serial.print("Servo pos:");
  Serial.print(CM_servo_pos_min);
  Serial.print("-");
  Serial.print(CM_servo_pos_max);

  Serial.print("\t\tServo voltage:");
  Serial.print(CM_servo_voltage_min);
  Serial.print("-");
  Serial.print(CM_servo_voltage_max);

  Serial.print("\t\tPot position:");
  Serial.print(CM_pot_pos_min);
  Serial.print("-");
  Serial.println(CM_pot_pos_max);


  isCalibrationModeOn = false;
  
}









///////////////////////////////////////////////////////////////////////////////
//////MAIN BLOCK

void setup() {
  Serial.begin(9600);
  delay(500);
  pinMode(FLASH_BUTTON_PIN, INPUT_PULLUP);

  setupWifiServer();

  Serial.println();
  Serial.println("Initializing Adafruit_ADS1015...");

  Wire.begin();
  ads.begin();  //инициализация внешнего АЦП

  ads.setGain(GAIN_ONE);
  Serial.println("Initialization complete");

  reset_config();

  setupDisplay();

  GetReadings();


  //figure out initial servo position
  data_array[SERVO_REAL_PERCENTAGE_DARRAY_INDEX] 
    = 100 * (data_array[SERVO_VOLTAGE_DARRAY_INDEX] - config_array[SERVO_VOLTAGE_MIN_CARRAY_INDEX]) 
    / (config_array[SERVO_VOLTAGE_MAX_CARRAY_INDEX] - config_array[SERVO_VOLTAGE_MIN_CARRAY_INDEX]);
  
  data_array[SERVO_REAL_POSITION_DARRAY_INDEX]
    = config_array[SERVO_POS_MIN_CARRAY_INDEX]
    + data_array[SERVO_REAL_PERCENTAGE_DARRAY_INDEX]
    * (config_array[SERVO_POS_MAX_CARRAY_INDEX]-config_array[SERVO_POS_MIN_CARRAY_INDEX])/100;




  findSensors();

  Serial.println();
  Serial.println("Setup Complete");


  StartCalibration();
  delay(1000);
}

void loop() {
  server.handleClient();

  //printData();
  GetReadings();
  
  if(isCalibrationModeOn==true){
    CalibrationLoop();
  }
  else{
    ChooseTargetServoPosition();
    moveServo();
  }
}
