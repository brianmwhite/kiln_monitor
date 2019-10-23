#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_MAX31856.h>
#include "KilnUtilities.h"
#include "LEDContainer.h"

//needed for wifimanager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

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

float temperatureForTooHotTargetTemperatureNotification = temperatureForTargetTemperatureNotification + 100;
int LowTemperatureThreshold = 10;

long TIME_BETWEEN_TEMPERATURE_READING = 10000L;
int SERIAL_BAUD_RATE = 115200;

//make sure to also update the build flags in platformio.ini to include something like:
//-D EXAMPLE_ENV_VARIABLE_NAME=\"${sysenv.EXAMPLE_ENV_VARIABLE_NAME}\"

#ifdef KILN_BLYNK_AUTH_TOKEN_ENVIRONMENT_VARIABLE
char BLYNK_AUTH_TOKEN[] = KILN_BLYNK_AUTH_TOKEN_ENVIRONMENT_VARIABLE;
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
bool hasNotifiedForTargetTooHighTemperature = false;

bool HasFault(uint8_t fault)
{
  if (fault)
  {
    LED_Thermocouple_Status.setStatus(LED_Thermocouple_Status.BLINK);
    Serial.println("DEBUG: Thermocouple Fault Detected");

    if (fault & MAX31856_FAULT_CJRANGE)
      Serial.println("Cold Junction Range Fault");
    if (fault & MAX31856_FAULT_TCRANGE)
      Serial.println("Thermocouple Range Fault");
    if (fault & MAX31856_FAULT_CJHIGH)
      Serial.println("Cold Junction High Fault");
    if (fault & MAX31856_FAULT_CJLOW)
      Serial.println("Cold Junction Low Fault");
    if (fault & MAX31856_FAULT_TCHIGH)
      Serial.println("Thermocouple High Fault");
    if (fault & MAX31856_FAULT_TCLOW)
      Serial.println("Thermocouple Low Fault");
    if (fault & MAX31856_FAULT_OVUV)
      Serial.println("Over/Under Voltage Fault");
    if (fault & MAX31856_FAULT_OPEN)
      Serial.println("Thermocouple Open Fault");
    return true;
  }
  else
  {
    return false;
  }
}

void SendNotifications(float kilnTemperature)
{
  if (kilnTemperature > temperatureForCoolDownNotification + LowTemperatureThreshold && !hasLowTemperatureNotificationBeenUnlocked)
  {
    hasLowTemperatureNotificationBeenUnlocked = true;
    Serial.println("DEBUG: Low Temperature Threshold Unlocked");
  }

  if (kilnTemperature <= temperatureForCoolDownNotification && !hasNotifiedForTargetLowTemperature && hasLowTemperatureNotificationBeenUnlocked)
  {
    Blynk.notify("Kiln has cooled down.");
    hasNotifiedForTargetLowTemperature = true;
    Serial.println("DEBUG: Notification Sent for Kiln Cooled Down");
  }

  if (kilnTemperature >= temperatureForTargetTemperatureNotification && !hasNotifiedForTargetHighTemperature)
  {
    Blynk.notify("Kiln has reached target cone temperature");
    hasNotifiedForTargetHighTemperature = true;
    Serial.println("DEBUG: Notification Sent for Target Cone Temperature Reached");
  }

  if (kilnTemperature >= temperatureForTooHotTargetTemperatureNotification && !hasNotifiedForTargetTooHighTemperature)
  {
    Blynk.notify("Warning: Kiln has exceeded target cone temperature.");
    hasNotifiedForTargetTooHighTemperature = true;
    Serial.println("DEBUG: Notification Sent for exceeding target cone temperature");
  }
}

void TemperatureTimeProcess()
{
  if (!HasFault(maxthermo.readFault()))
  {
    LED_Thermocouple_Status.setStatus(LED_Thermocouple_Status.ON);

    float kilnTempInCelsius = maxthermo.readThermocoupleTemperature();
    float kilnTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(kilnTempInCelsius);

    float boardTempInCelsius = maxthermo.readCJTemperature();
    float boardTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(boardTempInCelsius);

    Serial.println(String("DEBUG: ") + kilnTempInFahrenheit);
    Blynk.virtualWrite(V5, kilnTempInFahrenheit);
    Blynk.virtualWrite(V6, boardTempInFahrenheit);

    SendNotifications(kilnTempInFahrenheit);
  }
}

void WifiManagerPortalDisplayed(WiFiManager *myWiFiManager)
{
  Serial.println("DEBUG: Wifi Portal Displayed");
}

void WifiManagerWifiConnected()
{
  Serial.println("DEBUG: Wifi Connected");
  LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);
}

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(5000); //for debugging purposes, enough time to start the serial console

  LED_Thermocouple_Status.init(PIN_THERMOCOUPLE_LED_STATUS);
  LED_Wifi_Status.init(PIN_WIFI_LED_STATUS);
  LED_Power_Status.init(PIN_LED_POWER_STATUS);
  LED_BLYNK_Status.init(PIN_LED_BLYNK_STATUS);

  // delay(5000);

  LED_Power_Status.setStatus(LED_Power_Status.ON);

  WiFiManager wifiManager;
  wifiManager.setAPCallback(WifiManagerPortalDisplayed);
  wifiManager.setSaveConfigCallback(WifiManagerWifiConnected);

  //TODO: maybe create a random suffix using https://github.com/marvinroger/ESP8266TrueRandom

  if (wifiManager.autoConnect("KilnMonitor_4da994b3"))
  {
    LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);
    Serial.println("DEBUG: WifiManager reports true");
  }
  else
  {
    Serial.println("DEBUG: WifiManager reports false");
  }

  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(TIME_BETWEEN_TEMPERATURE_READING, TemperatureTimeProcess);

  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop()
{
  Blynk.run();
  timer.run();

  LED_BLYNK_Status.setStatus(Blynk.connected());

  LED_Power_Status.updateLED();
  LED_Thermocouple_Status.updateLED();
  LED_Wifi_Status.updateLED();
  LED_BLYNK_Status.updateLED();
}