#if __has_include("private.h")
    #include "private.h"
#endif


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


///////////////////////////////////////////////////////////////////////////////
/////DATA READINGS

#define DATA_ARRAY_SIZE 8

#define OUTDOOR_T_ARRAY_INDEX 0
#define STREAM_T_ARRAY_INDEX 1
#define INTERIOR_T_ARRAY_INDEX 2
#define DESIRED_STREAM_T_ARRAY_INDEX 3
#define DESIRED_INTERIOR_T_ARRAY_INDEX 4
#define POT_PERCENTAGE_ARRAY_INDEX 5
#define SERVO_PERCENTAGE_ARRAY_INDEX 6
#define LIGHT_LEVEL_ARRAY_INDEX 7

#define POT_ARRAY_SIZE 5 //pot readings smoothing array size


int data_array[DATA_ARRAY_SIZE] = {0};

void GetReadings() {

  float pot_value = readPot(analogRead(POT_PIN));
  data_array[POT_PERCENTAGE_ARRAY_INDEX] = 100 * (pot_value - pot_pos_min) / (pot_pos_max - pot_pos_min);

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





///////////////////////////////////////////////////////////////////////////////
//////main

void setup() {
  Serial.begin(9600);
  delay(500);

  setupWifiServer();
}

void loop() {
  server.handleClient();

  delay(100);
  GetReadings();
  printData();
  
}
