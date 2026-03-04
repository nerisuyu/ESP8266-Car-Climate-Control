#if __has_include("private.h")
    #include "private.h"
#endif


static inline int8_t sign(int val) {
  if (val < 0) return -1;
  if (val==0) return 0;
  return 1;
}


//PIN Adresses
#define POT_PIN A0
#define SERVO_PIN D8
#define THERMO_PIN D6
#define LED 2

#define IN1 D7   //"Engine On" Signal
#define OUT1 D5  //headlights

////////////////////////////////////////////////////////////////////////////////
////Servo and Pot calibration

int pot_pos_min = 0;
int pot_pos_max = 1024;

int servo_pos_min = 1500;
int servo_pos_max = 2380;

int servo_voltage_min = -4;
int servo_voltage_max = 1658;


///////////////////////////////////////////////////////////////////////////////
/////DATA READINGS
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1015 ads;    // задаём тип АЦП


#define DATA_ARRAY_SIZE 13

#define OUTDOOR_T_ARRAY_INDEX 0
#define STREAM_T_ARRAY_INDEX 1
#define INTERIOR_T_ARRAY_INDEX 2
#define DESIRED_STREAM_T_ARRAY_INDEX 3
#define DESIRED_INTERIOR_T_ARRAY_INDEX 4

#define POT_VALUE_ARRAY_INDEX 5
#define POT_PERCENTAGE_ARRAY_INDEX 6

#define SERVO_VOLTAGE_ARRAY_INDEX 7
#define SERVO_TARGET_POSITION_ARRAY_INDEX 8
#define SERVO_REAL_POSITION_ARRAY_INDEX 9
#define SERVO_TARGET_PERCENTAGE_ARRAY_INDEX 10 
#define SERVO_REAL_PERCENTAGE_ARRAY_INDEX 11

#define LIGHT_LEVEL_ARRAY_INDEX 12


#define POT_ARRAY_SIZE 5 //pot readings smoothing array size


int data_array[DATA_ARRAY_SIZE] = {0};

void GetReadings() {

  float pot_value = readPot(analogRead(POT_PIN));
  data_array[POT_VALUE_ARRAY_INDEX] = pot_value;
  data_array[POT_PERCENTAGE_ARRAY_INDEX] = 100 * (pot_value - pot_pos_min) / (pot_pos_max - pot_pos_min);

  float servo_voltage=ads.readADC_SingleEnded(0);

  data_array[SERVO_VOLTAGE_ARRAY_INDEX]=servo_voltage;
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

///////////////////////////////////////////////////////////////////////////////
////Wifi Server

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "webpage.h"
#ifndef APSSID
  #define APSSID "ESP8266-Climate-Control"
  #define APPSK "password"
#endif


//WiFi Connection configuration
const char *ssid = APSSID;
const char *password = APPSK;

IPAddress my_ip = 0;
ESP8266WebServer server(80);

void setupWifiServer(){
// setup WiFi Network
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
////////Screen

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

  //display.clearDisplay();
}

/////////////////////////////////////////////////////////////////////////////////
//////Servo

#include <Servo.h>

bool is_servo_attached=false;
Servo my_servo;  // create servo object to control a servo

#define MAX_STEP 8

#define EPSILON_SERVO_POS 10

void moveServo()  {

  //target percentage --> target position
  //real position --> real percentage 


  data_array[SERVO_TARGET_POSITION_ARRAY_INDEX] = servo_pos_min + data_array[SERVO_TARGET_PERCENTAGE_ARRAY_INDEX] * (servo_pos_max-servo_pos_min)/100;

  if(
    abs(data_array[SERVO_TARGET_POSITION_ARRAY_INDEX]-data_array[SERVO_REAL_POSITION_ARRAY_INDEX])>EPSILON_SERVO_POS //if real servo pos differs from needed position
    )
    {
      int new_servo_real_position=
        data_array[SERVO_REAL_POSITION_ARRAY_INDEX]
        +sign(
          data_array[SERVO_TARGET_POSITION_ARRAY_INDEX]-data_array[SERVO_REAL_POSITION_ARRAY_INDEX]
          )
        *constrain(
          abs(
            data_array[SERVO_TARGET_POSITION_ARRAY_INDEX]-data_array[SERVO_REAL_POSITION_ARRAY_INDEX]
            )
            ,1,MAX_STEP
          );//последняя цифра -максимальный шаг движения сервы за один цикл


    my_servo.writeMicroseconds(new_servo_real_position);
    display.display();

    data_array[SERVO_REAL_POSITION_ARRAY_INDEX]=new_servo_real_position;
    data_array[SERVO_REAL_PERCENTAGE_ARRAY_INDEX]=100*(new_servo_real_position-servo_pos_min)/(servo_pos_max-servo_pos_min);

    if (is_servo_attached == false) {
      my_servo.attach(SERVO_PIN, 500, 2500);
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



///////////////////////////////////////////////////////////////////////////////
//////main

void setup() {
  Serial.begin(9600);
  delay(500);

  setupWifiServer();

  Serial.println();
  Serial.println("Initializing Adafruit_ADS1015...");

  Wire.begin();
  ads.begin();  //инициализация внешнего АЦП

  ads.setGain(GAIN_ONE);
  Serial.println("Initialization complete");

  setupDisplay();

  GetReadings();

  data_array[SERVO_REAL_PERCENTAGE_ARRAY_INDEX] = 100 * (data_array[SERVO_VOLTAGE_ARRAY_INDEX] - servo_voltage_min) / (servo_voltage_max - servo_voltage_min);
  data_array[SERVO_REAL_POSITION_ARRAY_INDEX] = servo_pos_min + data_array[SERVO_REAL_PERCENTAGE_ARRAY_INDEX]*(servo_pos_max-servo_pos_min)/100;

}

void loop() {
  //server.handleClient();

  //delay(100);
  printData();
  GetReadings();
  
  data_array[SERVO_TARGET_PERCENTAGE_ARRAY_INDEX]=data_array[POT_PERCENTAGE_ARRAY_INDEX];

  moveServo();
}
