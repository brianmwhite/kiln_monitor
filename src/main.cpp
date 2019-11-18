#define BLYNK_PRINT Serial

#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include <Adafruit_MAX31856.h>
// #include <BlynkSimpleEsp8266.h>
#include <WiFi101.h>
#include <WiFiMDNSResponder.h>
#include <BlynkSimpleWiFiShield101.h>
#include "KilnUtilities.h"
#include "LEDContainer.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

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

//wifi ap provisioning
char mdnsName[] = "kilnmonitor";
WiFiServer server(80);
WiFiMDNSResponder mdnsResponder;

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

// #ifdef KILN_WIFI_SSID
//   char ssid[] = KILN_WIFI_SSID;
// #endif

// #ifdef KILN_WIFI_PWD
//   char pass[] = KILN_WIFI_PWD;
// #endif


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

int GetOLEDVerticalCoordiatesFromLine(int line)
{
  return (line - 1) * 8;
}

void PrepDisplayLineForWriting (int line) 
{
  int verticalCoordinates = GetOLEDVerticalCoordiatesFromLine(line);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0,verticalCoordinates);
}

void WriteThermocoupleFaultToDisplay(String faultText) 
{
  PrepDisplayLineForWriting(1);
  display.print(faultText);
  display.display();
}

void WriteTemperatureToDisplay(float kilnTemp, float boardTemp) 
{
  PrepDisplayLineForWriting(1);
  display.print("Kiln Temp:");
  display.print(kilnTemp);
  display.cp437(true);
  display.write(167);
  display.print("F");
  display.display();
}

void WriteBlynkStatusToDisplay(bool blynkStatus)
{
  PrepDisplayLineForWriting(2);
  display.print("Blynk:");
  if (blynkStatus) {
    display.print("Connected");
  } else {
    display.print("Disconnected");
  }
  display.display();
}

void WriteWiFiStatusToDisplay(char* SSID, String ipAddr)
{
  PrepDisplayLineForWriting(3);
  display.print("IP:");
  display.print(ipAddr);
  display.setCursor(0,GetOLEDVerticalCoordiatesFromLine(4));
  display.print("AP:");
  display.print(SSID);
  display.display();
}

void WriteNotificationStatusToDisplay(String notification)
{
  PrepDisplayLineForWriting(4);
  display.print("*");
  display.print(notification);
  display.display();
}

bool CheckForThermocoupleFault(uint8_t fault)
{
  if (fault)
  {
    LED_Thermocouple_Status.setStatus(LED_Thermocouple_Status.BLINK);
    Serial.println("DEBUG: Thermocouple Fault Detected");
    WriteThermocoupleFaultToDisplay("Thermocouple Error");

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
    WriteNotificationStatusToDisplay("Kiln has cooled down");
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
    WriteNotificationStatusToDisplay("Target Temp Achieved");
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
    WriteNotificationStatusToDisplay("Exceeded Target Temp");
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
      WriteTemperatureToDisplay(kilnTempInFahrenheit, boardTempInFahrenheit);
      Blynk.virtualWrite(V5, kilnTempInFahrenheit);
      Blynk.virtualWrite(V6, boardTempInFahrenheit);

      SendNotifications(kilnTempInFahrenheit);
    }
  }
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
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
  // delay(5000); //for debugging purposes, enough time to start the serial console


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();

  delay(1000);
 
  // Clear the buffer.
  display.clearDisplay();
  display.display();

  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  
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

  WiFi.beginProvision();

  LED_Wifi_Status.setStatus(LED_Wifi_Status.BLINK);
  while (WiFi.status() != WL_CONNECTED)
  {
    LED_Wifi_Status.updateLED();
  }
  LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);

  server.begin();

  if (!mdnsResponder.begin(mdnsName)) {
    Serial.println("Failed to start MDNS responder!");
    while(1);
  }

  Serial.print("Server listening at http://");
  Serial.print(mdnsName);
  Serial.println(".local/");

  Blynk.config(BLYNK_AUTH_TOKEN);
  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  WriteWiFiStatusToDisplay(WiFi.SSID(), ipToString(WiFi.localIP()));

  timer.setInterval(TIME_BETWEEN_TEMPERATURE_READING, TemperatureTimeProcess);

  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop()
{
  mdnsResponder.poll();

  Blynk.run();
  timer.run();

  LED_BLYNK_Status.setStatus(Blynk.connected());
  WriteBlynkStatusToDisplay(Blynk.connected());

  LED_Power_Status.updateLED();
  LED_Thermocouple_Status.updateLED();
  LED_Wifi_Status.updateLED();
  LED_BLYNK_Status.updateLED();

  // if (!digitalRead(BUTTON_A))
  //   display.print("A");
  // if (!digitalRead(BUTTON_B))
  //   display.print("B");
  // if (!digitalRead(BUTTON_C))
  //   display.print("C");
  // delay(10);
  // yield();
  // display.display();
}