#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_MAX31856.h>
#include "KilnUtilities.h"

//pin definition
#define LED_WIFI 2
#define LED_BLYNK 16
#define LED_POWER 5
#define LED_THERMO 4

#define spi_cs 12
#define spi_mosi 13
#define spi_miso 14
#define spi_clk 15

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
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(spi_cs, spi_mosi, spi_miso, spi_clk);

bool notifiedMaxTemp = false;
float maxTempForNotify = kiln.LookUpTemperatureValueFromCone("6");
float coolDownTemperature = 90.0;

bool HasFault(uint8_t fault)
{
    if (fault) {

      for (int i = 0; i < 10; i++) {
        digitalWrite(LED_THERMO, HIGH);
        delay(100);
        digitalWrite(LED_THERMO, LOW);
        delay(100);
      }

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
    else {
      return false;
    }
    
}

void TemperatureTimeProcess() 
{ 
  if (!HasFault(maxthermo.readFault())) {
    digitalWrite(LED_THERMO, HIGH);

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

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_BLYNK, OUTPUT);
  pinMode(LED_THERMO, OUTPUT);

  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_BLYNK, LOW);
  digitalWrite(LED_THERMO, LOW);


  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_POWER, HIGH);
    delay(100);
    digitalWrite(LED_POWER, LOW);
    delay(100);
  }

  Blynk.begin(auth, ssid, pass);
  digitalWrite(LED_WIFI, HIGH);

  timer.setInterval(1000L, TemperatureTimeProcess);
  
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() {
  Blynk.run();
  timer.run();
  digitalWrite(LED_BLYNK, Blynk.connected());
  digitalWrite(LED_POWER, HIGH);
}