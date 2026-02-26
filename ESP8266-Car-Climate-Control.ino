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


void setup() {

}

void loop() {

}
