/*
 Temperature logger for Fatima's kiln
 NodeMCU v1.0 with MAX31855K thermocouple
 blynk app showing internal temp and TC temp
 Tom Tobback - BuffaloLabs Feb 2018

V0 internal temp
 V1 thermocouple temp

************************************************************
 Download latest Blynk library here:
 https://github.com/blynkkk/blynk-library/releases/latest

Blynk is a platform with iOS and Android apps to control
 Arduino, Raspberry Pi and the likes over the Internet.
 You can easily build graphic interfaces for all your
 projects by simply dragging and dropping widgets.

Downloads, docs, tutorials: http://www.blynk.cc
 Sketch generator: http://examples.blynk.cc
 Blynk community: http://community.blynk.cc
 Social networks: http://www.fb.com/blynkapp
 http://twitter.com/blynk_app

Blynk library is licensed under MIT license
 This example code is in public domain.

*************************************************************
 This example runs directly on NodeMCU.

Note: This requires ESP8266 support package:
 https://github.com/esp8266/Arduino

Please be sure to select the right NodeMCU module
 in the Tools -> Board menu!

For advanced settings please follow ESP examples :
 - ESP8266_Standalone_Manual_IP.ino
 - ESP8266_Standalone_SmartConfig.ino
 - ESP8266_Standalone_SSL.ino

Change WiFi ssid, pass, and Blynk auth token to run :)
 Feel free to apply it to any other example. It's simple!
 *************************************************************/

#define LED_WIFI 5 // D1
#define LED_BLYNK 4 // D2

///////////////////// THERMOCOUPLE SETUP /////////////////////
/***************************************************
 This is an example for the Adafruit Thermocouple Sensor w/MAX31855K
 Designed specifically to work with the Adafruit Thermocouple Sensor
 ----> https://www.adafruit.com/products/269
 These displays use SPI to communicate, 3 pins are required to
 interface
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 Written by Limor Fried/Ladyada for Adafruit Industries.
 BSD license, all text above must be included in any redistribution
 ****************************************************/
#include <SPI.h>
#include "Adafruit_MAX31855.h"
// Default connection is using software SPI, but comment and uncomment one of
// the two examples below to switch between software SPI and hardware SPI:
#define MAXDO 12 // D6
#define MAXCS 13 // D7
#define MAXCLK 15 // D8
// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

//////////////////// BLYNK SETUP //////////////////////////////
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xxxx";
// Your WiFi credentials.
// Set password to "" for open networks.
//char ssid[] = "xxxx";
//char pass[] = "xxxx";

BlynkTimer timer;

//////////////////// WIFI MANAGER ////////////////////////////
#include <DNSServer.h> //needed for wifimanager library
#include <ESP8266WebServer.h> //needed for wifimanager library
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

///////////////////////////////////////////////////////////////

void myTimerEvent() // read thermocouple and push data to blynk
{
 float internal_t = thermocouple.readInternal();
 int tc_t = thermocouple.readCelsius();

if (isnan(internal_t) || isnan(tc_t) || tc_t > 1500) { // if thermocouple does not return a number
 Serial.println("Something wrong with thermocouple!");
 } else {
 Serial.print("Internal Temp = ");
 Serial.print(internal_t);
 Serial.print("\t Thermocouple Temp = ");
 Serial.print(tc_t);
 if (tc_t < 2000) {
 Serial.println("\t Sending to blynk..");
 Blynk.virtualWrite(V0, internal_t);
 Blynk.virtualWrite(V1, tc_t);}
 else {
 Serial.println("\t Temperature not right, not sending to Blynk..");
 }
 }

}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void setup()
{
 Serial.begin(9600);

pinMode(LED_BUILTIN, OUTPUT);
 pinMode(LED_WIFI, OUTPUT);
 pinMode(LED_BLYNK, OUTPUT);
 digitalWrite(LED_BUILTIN, LOW); // on board LED (inverted) ON when powered on
 digitalWrite(LED_WIFI, LOW);
 digitalWrite(LED_BLYNK, LOW);

for (int i = 0; i < 10; i++) {
 digitalWrite(LED_WIFI, HIGH);
 delay(100);
 digitalWrite(LED_WIFI, LOW);
 delay(100);
 }

WiFiManager wifiManager;
 // wifiManager.resetSettings(); // known networks are saved so need to reset for testing
 wifiManager.autoConnect("BuffaloLabsAP", "BuffaloLabs");
 digitalWrite(LED_WIFI, HIGH); // wifi connected successfully

// Blynk.begin(auth, ssid, pass); // original line
 Blynk.config(auth);
 // Setup a function to be called every 10 seconds
 timer.setInterval(10000L, myTimerEvent);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void loop()
{
 Blynk.run();
 timer.run(); // Initiates BlynkTimer
 digitalWrite(LED_BLYNK, Blynk.connected()); // LED on when connected to Blynk
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////