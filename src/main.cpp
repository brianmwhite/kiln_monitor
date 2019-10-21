#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_MAX31856.h>
#include "KilnUtilities.h"
#include "LEDContainer.h"

//pin definition
#define PIN_THERMOCOUPLE_LED_STATUS 4
#define PIN_WIFI_LED_STATUS 2
#define PIN_LED_BLYNK_STATUS 16
#define PIN_LED_POWER_STATUS 5

#define spi_cs 12
#define spi_mosi 13
#define spi_miso 14
#define spi_clk 15

LEDContainer LED_Thermocouple_Status;
LEDContainer LED_Wifi_Status;
LEDContainer LED_Power_Status;
LEDContainer LED_BLYNK_Status;

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
      LED_Thermocouple_Status.READY_STATE = false;

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
    LED_Thermocouple_Status.READY_STATE = true;

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

  LED_Thermocouple_Status.init(PIN_THERMOCOUPLE_LED_STATUS);
  LED_Wifi_Status.init(PIN_WIFI_LED_STATUS);
  LED_Power_Status.init(PIN_LED_POWER_STATUS);
  LED_BLYNK_Status.init(PIN_LED_BLYNK_STATUS);

  Blynk.begin(auth, ssid, pass);
  LED_Wifi_Status.READY_STATE = true;
  
  timer.setInterval(1000L, TemperatureTimeProcess);
  
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() {
  LED_Power_Status.READY_STATE = true;

  Blynk.run();
  LED_BLYNK_Status.READY_STATE = Blynk.connected();
 
  timer.run();
  
  LED_Power_Status.updateLED();
  LED_Thermocouple_Status.updateLED();
  LED_Wifi_Status.updateLED();
  LED_BLYNK_Status.updateLED();
}