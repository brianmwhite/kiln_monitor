#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_MAX31856.h>
#include "KilnUtilities.h"

#ifdef KILN_BLYNK_AUTH
  char auth[] = KILN_BLYNK_AUTH;
#endif

#ifdef KILN_WIFI_SSID
  char ssid[] = KILN_WIFI_SSID;
#endif

#ifdef KILN_WIFI_PWD
  char pass[] = KILN_WIFI_PWD;
#endif
KilnUtilities kiln;

BlynkTimer timer;
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(12, 13, 14, 15);

bool notifiedMaxTemp = false;
float maxTempForNotify = kiln.LookUpTemperatureValueFromCone("6");
float coolDownTemperature = 90.0;

bool HasFault(uint8_t fault)
{
    if (fault) {
      if (fault & MAX31856_FAULT_CJRANGE) Serial.println("Cold Junction Range Fault");
      if (fault & MAX31856_FAULT_TCRANGE) Serial.println("Thermocouple Range Fault");
      if (fault & MAX31856_FAULT_CJHIGH)  Serial.println("Cold Junction High Fault");
      if (fault & MAX31856_FAULT_CJLOW)   Serial.println("Cold Junction Low Fault");
      if (fault & MAX31856_FAULT_TCHIGH)  Serial.println("Thermocouple High Fault");
      if (fault & MAX31856_FAULT_TCLOW)   Serial.println("Thermocouple Low Fault");
      if (fault & MAX31856_FAULT_OVUV)    Serial.println("Over/Under Voltage Fault");
      if (fault & MAX31856_FAULT_OPEN)    Serial.println("Thermocouple Open Fault");
      return true;
    }
    else
    {
      return false;
    }
    
}

void TemperatureTimeProcess() 
{ 
  if (!HasFault(maxthermo.readFault())) {
    float kilnTempInCelsius = maxthermo.readThermocoupleTemperature();
    float kilnTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(kilnTempInCelsius);

    float boardTempInCelsius = maxthermo.readCJTemperature();
    float boardTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(boardTempInCelsius);

    Serial.println(kilnTempInFahrenheit);
    Blynk.virtualWrite(V5, kilnTempInFahrenheit);
    Blynk.virtualWrite(V6, boardTempInFahrenheit);

    if (kilnTempInFahrenheit >= maxTempForNotify && !notifiedMaxTemp)
    {
      Blynk.notify("Max Temp Target Reached");
      notifiedMaxTemp = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  timer.setInterval(1000L, TemperatureTimeProcess);
  
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() {
  Blynk.run();
  timer.run(); 
}