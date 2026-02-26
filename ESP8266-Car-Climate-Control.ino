#include "webpage.h"

#if __has_include("private.h")
    #include "private.h"
#endif

#ifndef APSSID
  #define APSSID "ESP8266-Climate-Control"
  #define APPSK "password"
#endif


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


//WiFi Connection configuration
const char *ssid = APSSID;
const char *password = APPSK;

IPAddress my_ip = 0;
ESP8266WebServer server(80);


///////////////////////////////////////////////////////////////////////////////
////Wifi Server

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

void setup() {
  Serial.begin(9600);
  delay(500);

  setupWifiServer();
}

void loop() {
  server.handleClient();
}
