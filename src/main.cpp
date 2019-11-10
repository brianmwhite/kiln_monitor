#define BLYNK_PRINT Serial

#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include <Adafruit_MAX31856.h>
// #include <BlynkSimpleEsp8266.h>
#include <WiFi101.h>
#include <BlynkSimpleWiFiShield101.h>
#include "KilnUtilities.h"
#include "LEDContainer.h"
// #include <string>

//needed for wifimanager
// #include <DNSServer.h>
// #include <ESP8266WebServer.h>
// #include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

//pin definitions
#define PIN_THERMOCOUPLE_LED_STATUS 13 //yellow
#define PIN_WIFI_LED_STATUS 11 //blue
#define PIN_LED_BLYNK_STATUS 10 //green
#define PIN_LED_POWER_STATUS 12 //red

//thermocouple pins
#define spi_cs 19
#define spi_mosi 23
#define spi_miso 22
#define spi_clk 24

KilnUtilities kiln;

// std::string targetCone = "6";
long TIME_BETWEEN_TEMPERATURE_READING = 1000L;
float temperatureForCoolDownNotification = 90.0;

float temperatureForTargetTemperatureNotification = 2232;//kiln.LookUpTemperatureValueFromCone(targetCone);
float temperatureForTooHotTargetTemperatureNotification = temperatureForTargetTemperatureNotification + 10;
int LowTemperatureThreshold = 10;

int SERIAL_BAUD_RATE = 9600;
int MAX_THERMOCOUPLE_TEMPERATURE_CELSIUS = 1800;

//make sure to also update the build flags in platformio.ini to include something like:
//-D EXAMPLE_ENV_VARIABLE_NAME=\"${sysenv.EXAMPLE_ENV_VARIABLE_NAME}\"

#ifdef KILN_BLYNK_AUTH_TOKEN_ENVIRONMENT_VARIABLE
char BLYNK_AUTH_TOKEN[] = KILN_BLYNK_AUTH_TOKEN_ENVIRONMENT_VARIABLE;
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
// Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(spi_cs, spi_mosi, spi_miso, spi_clk);
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(spi_cs);

bool hasNotifiedForTargetHighTemperature = false;
bool hasNotifiedForTargetLowTemperature = false;
bool hasLowTemperatureNotificationBeenUnlocked = false;
bool hasNotifiedForTargetTooHighTemperature = false;

bool CheckForThermocoupleFault(uint8_t fault)
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
    String notification = "Kiln has cooled down to ";
    notification += kilnTemperature;
    notification += "\u00B0F";

    Blynk.notify(notification);
    hasNotifiedForTargetLowTemperature = true;
    Serial.println(notification);
  }

  if (kilnTemperature >= temperatureForTargetTemperatureNotification && !hasNotifiedForTargetHighTemperature)
  {
    String notification = "Kiln has reached target cone temperature of CONE ";
    notification += "6";//targetCone.c_str();
    notification += " [";
    notification += temperatureForTargetTemperatureNotification;
    notification += "\u00B0F] / Actual temperature is ";
    notification += kilnTemperature;
    notification += "\u00B0F";

    Blynk.notify(notification);
    hasNotifiedForTargetHighTemperature = true;
    Serial.println(notification);
  }

  if (kilnTemperature >= temperatureForTooHotTargetTemperatureNotification && !hasNotifiedForTargetTooHighTemperature)
  {
    String notification = "Warning: Kiln has exceeded target CONE ";
    notification += "6";//targetCone.c_str();
    notification += "/ Temperature is ";
    notification += kilnTemperature;
    notification += " \u00B0F";
    
    Blynk.notify(notification);
    hasNotifiedForTargetTooHighTemperature = true;
    Serial.println("DEBUG: Notification Sent for exceeding target cone temperature");
  }
}

void TemperatureTimeProcess()
{
  if (!CheckForThermocoupleFault(maxthermo.readFault()))
  {
    LED_Thermocouple_Status.setStatus(LED_Thermocouple_Status.ON);

    float kilnTempInCelsius = maxthermo.readThermocoupleTemperature();
    float boardTempInCelsius = maxthermo.readCJTemperature();

    if (isnan(kilnTempInCelsius) || isnan(boardTempInCelsius) || kilnTempInCelsius > MAX_THERMOCOUPLE_TEMPERATURE_CELSIUS) 
    {
      Serial.println("DEBUG: Non fault related thermocouple issue with temperature reading.");
    }
    else 
    {
      float kilnTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(kilnTempInCelsius);
      float boardTempInFahrenheit = kiln.ConvertCelsiusToFahrenheit(boardTempInCelsius);

      Serial.println(String("DEBUG: ") + kilnTempInFahrenheit);
      Blynk.virtualWrite(V5, kilnTempInFahrenheit);
      Blynk.virtualWrite(V6, boardTempInFahrenheit);

      SendNotifications(kilnTempInFahrenheit);
    }
  }
}

// void WifiManagerPortalDisplayedEvent(WiFiManager *myWiFiManager)
// {
//   Serial.println("DEBUG: Wifi Portal Displayed");
// }

// void WifiManagerWifiConnectedEvent()
// {
//   Serial.println("DEBUG: Wifi Connected");
//   LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);
// }

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);

  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);

  delay(5000); //for debugging purposes, enough time to start the serial console

  LED_Thermocouple_Status.init(PIN_THERMOCOUPLE_LED_STATUS);
  LED_Wifi_Status.init(PIN_WIFI_LED_STATUS);
  LED_Power_Status.init(PIN_LED_POWER_STATUS);
  LED_BLYNK_Status.init(PIN_LED_BLYNK_STATUS);

  LED_Power_Status.setStatus(LED_Power_Status.ON);

  // WiFiManager wifiManager;
  // wifiManager.setAPCallback(WifiManagerPortalDisplayedEvent);
  // wifiManager.setSaveConfigCallback(WifiManagerWifiConnectedEvent);

  //TODO: maybe create a random suffix using https://github.com/marvinroger/ESP8266TrueRandom

  // if (wifiManager.autoConnect("KilnMonitor_4da994b3"))
  // {
  //   LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);
  //   Serial.println("DEBUG: WifiManager reports true");
  // }
  // else
  // {
  //   Serial.println("DEBUG: WifiManager reports false");
  // }

  // Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

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