#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_MAX31856.h>
#include "KilnUtilities.h"
#include "LEDContainer.h"

//pin definitions
#define PIN_THERMOCOUPLE_LED_STATUS 4
#define PIN_WIFI_LED_STATUS 2
#define PIN_LED_BLYNK_STATUS 16
#define PIN_LED_POWER_STATUS 5

//thermocouple pins
#define spi_cs 12
#define spi_mosi 13
#define spi_miso 14
#define spi_clk 15

KilnUtilities kiln;
float temperatureForTargetTemperatureNotification = kiln.LookUpTemperatureValueFromCone("6");
float temperatureForCoolDownNotification = 90.0;

long TIME_BETWEEN_TEMPERATURE_READING = 10000L;
int SERIAL_BAUD_RATE = 115200;

#ifdef KILN_BLYNK_AUTH
  char auth[] = KILN_BLYNK_AUTH;
#endif

#ifdef KILN_WIFI_SSID
  char ssid[] = KILN_WIFI_SSID;
#endif

#ifdef KILN_WIFI_PWD
  char pass[] = KILN_WIFI_PWD;
#endif

LEDContainer LED_Thermocouple_Status;
LEDContainer LED_Wifi_Status;
LEDContainer LED_Power_Status;
LEDContainer LED_BLYNK_Status;

BlynkTimer timer;
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(spi_cs, spi_mosi, spi_miso, spi_clk);

bool hasNotifiedForTargetHighTemperature = false;
bool hasNotifiedForTargetLowTemperature = false;
bool hasLowTemperatureNotificationBeenUnlocked = false;
int LowTemperatureThreshold = 10;

bool HasFault(uint8_t fault)
{
    if (fault) {
      LED_Thermocouple_Status.setStatus(false);

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
    LED_Thermocouple_Status.setStatus(true);

    float kilnTempInCelsius = maxthermo.readThermocoupleTemperature();
    float kilnTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(kilnTempInCelsius);

    float boardTempInCelsius = maxthermo.readCJTemperature();
    float boardTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(boardTempInCelsius);

    Serial.println(kilnTempInFahrenheit);
    Blynk.virtualWrite(V5, kilnTempInFahrenheit);
    Blynk.virtualWrite(V6, boardTempInFahrenheit);

    if (kilnTempInFahrenheit > temperatureForCoolDownNotification + LowTemperatureThreshold && !hasLowTemperatureNotificationBeenUnlocked) {
      hasLowTemperatureNotificationBeenUnlocked = true;
    }

    if (kilnTempInFahrenheit <= temperatureForCoolDownNotification && !hasNotifiedForTargetLowTemperature && hasLowTemperatureNotificationBeenUnlocked) {
      Blynk.notify("Kiln has cooled down.");
      hasNotifiedForTargetLowTemperature = true;
    }

    if (kilnTempInFahrenheit >= temperatureForTargetTemperatureNotification && !hasNotifiedForTargetHighTemperature)
    {
      Blynk.notify("Kiln has reached target cone temperature");
      hasNotifiedForTargetHighTemperature = true;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  LED_Thermocouple_Status.init(PIN_THERMOCOUPLE_LED_STATUS);
  LED_Wifi_Status.init(PIN_WIFI_LED_STATUS);
  LED_Power_Status.init(PIN_LED_POWER_STATUS);
  LED_BLYNK_Status.init(PIN_LED_BLYNK_STATUS);

  LED_Power_Status.setStatus(true);

  Blynk.begin(auth, ssid, pass);
  LED_Wifi_Status.setStatus(true);
  
  timer.setInterval(TIME_BETWEEN_TEMPERATURE_READING, TemperatureTimeProcess);
  
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() {
  Blynk.run();
  LED_BLYNK_Status.setStatus(Blynk.connected());
 
  LED_Power_Status.updateLED();
  LED_Thermocouple_Status.updateLED();
  LED_Wifi_Status.updateLED();
  LED_BLYNK_Status.updateLED();

  timer.run();
}