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
#include <FlashStorage.h> 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//TODO: review ascii only led library
//GitHub - greiman/SSD1306Ascii: Text only Arduino Library for SSD1306 OLED displays
//https://github.com/greiman/SSD1306Ascii
 
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

//time to wait to only capture a single button press
#define DEBOUNCE  150

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

long TIME_BETWEEN_TEMPERATURE_READING = 1000L;
float temperatureForCoolDownNotification = 90.0;

float temperatureForTargetTemperatureNotification = 2232;//kiln.LookUpTemperatureValueFromCone(targetCone);
float temperatureForTooHotTargetTemperatureNotification = temperatureForTargetTemperatureNotification + 10;
int LowTemperatureThreshold = 10;

int SERIAL_BAUD_RATE = 9600;
int MAX_THERMOCOUPLE_TEMPERATURE_CELSIUS = 1370;

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
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(spi_cs);

bool hasNotifiedForTargetHighTemperature = false;
bool hasNotifiedForTargetLowTemperature = false;
bool hasLowTemperatureNotificationBeenUnlocked = false;
bool hasNotifiedForTargetTooHighTemperature = false;

int coneSequenceValues[] = { 2345,2300,2273,2262,2232,2167,2142,2106,2088,2079,2046,2016,1987,1945,1888,1828,1789,1728,1688,1657,1607,1582,1539,1485,1456,1422,1360,1252,1252,1159,1112,1087 };
String coneSequenceLabels[] = { "10","9","8","7","6","5","4","3","2","1","01","02","03","04","05","06","07","08","09","010","011","012","013","014","015","016","017","018","019","020","021","022" };
int coneSequenceNumberOfItems = 32;
int coneSequenceDefaultStartLocation = 4;

int coolDownTargetSequenceValues [] = {80,90,100,110,120,130,140,150};
String coolDownTargetSequenceLabels [] = {"80","90","100","110","120","130","140","150"};
int coolDownSequenceNumberOfItems = 8;
int coolDownTempDefaultStartLocation = 1;

typedef struct {
  int ConeTargetSequenceLocation;
  int CoolDownTargetSequenceLocation;
  boolean HasSavedValues;
} UserSetVariablesStructure;

UserSetVariablesStructure UserSetVariables;
FlashStorage(Storage_UserSetVariables, UserSetVariablesStructure);

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
  display.print("                     ");
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
  display.print("T:");
  display.print(kilnTemp);
  display.cp437(true);
  display.write(167);
  display.print("F");
  display.print(" ");
  display.write(30);
  display.print("C");
  display.print(coneSequenceLabels[UserSetVariables.ConeTargetSequenceLocation]);
  display.print(" ");
  display.write(31);
  display.print(coolDownTargetSequenceLabels[UserSetVariables.CoolDownTargetSequenceLocation]);
  display.write(167);
  // display.print("F");
  display.display();
}

void WriteProvisioningInstructions() 
{
  PrepDisplayLineForWriting(1);
  display.println("Set up by connecting");
  display.println("to this Wifi AP:");
  display.println("");
	//copied ssid logic from wifi.cpp
  //TODO: modify library to create a customized SSID and have a method to retrieve it
  //could use mac address or maybe create a random suffix using something like https://github.com/marvinroger/ESP8266TrueRandom

  uint8_t mac[6];
	char provSsid[13];
  WiFi.macAddress(mac);
  sprintf(provSsid, "wifi101-%.2X%.2X", mac[1], mac[0]);
  display.println(provSsid);
  display.display();
}

void WriteBlynkStatusToDisplay(bool blynkStatus)
{
  PrepDisplayLineForWriting(2);
  display.print("Blynk:");
  if (blynkStatus) {
    display.print("Connected   ");
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

void PrintCurrentSequenceValueToDisplay(String label, String valueSequence[], int sequenceLocation) 
{
    PrepDisplayLineForWriting(3);
    display.print(label);
    display.print(" ");
    display.print(valueSequence[sequenceLocation]);
    display.display();
}

void ClearAllDisplayLines()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0,0);
    display.display();
}

int ShowSequenceMenu(String label, String valueSequence[], int numberOfItemsInSequence, int StartLocation)
{
  //TODO: figure out how to display temp symbol just for target temperature display
  bool inMenu = true;
  int sequenceLocation = StartLocation;
  ClearAllDisplayLines();
  
  PrepDisplayLineForWriting(1);
  display.println("KILN MONITOR");
  display.print("Set target:");
  display.display();

  while (inMenu)
  {
    PrintCurrentSequenceValueToDisplay(label, valueSequence, sequenceLocation);

    if (!digitalRead(BUTTON_A))
    {
      delay(DEBOUNCE);
      inMenu = false;
    }
    if (!digitalRead(BUTTON_B))
    {
      delay(DEBOUNCE);
      sequenceLocation++;
      if (sequenceLocation >= numberOfItemsInSequence)
      {
        sequenceLocation = 0;
      }
      PrintCurrentSequenceValueToDisplay(label, valueSequence, sequenceLocation);
    }
    if (!digitalRead(BUTTON_C))
    {
      delay(DEBOUNCE);
      sequenceLocation--;
      if (sequenceLocation < 0)
      {
        sequenceLocation = numberOfItemsInSequence - 1;
      }
      PrintCurrentSequenceValueToDisplay(label, valueSequence, sequenceLocation);
    }
  }
  ClearAllDisplayLines();
  return sequenceLocation;
}

void ShowMenu()
{
  int coneSequenceStartLocation = coneSequenceDefaultStartLocation;
  int coolDownTempStartLocation = coolDownTempDefaultStartLocation;

  if (UserSetVariables.HasSavedValues)
  {
    coneSequenceStartLocation = UserSetVariables.ConeTargetSequenceLocation;
    coolDownTempStartLocation = UserSetVariables.CoolDownTargetSequenceLocation;
  }

  int ConeTargetSequenceLocation = ShowSequenceMenu("CONE", coneSequenceLabels, coneSequenceNumberOfItems, coneSequenceStartLocation);
  int CooldownTempSequenceLocation = ShowSequenceMenu("COOLDOWN", coolDownTargetSequenceLabels, coolDownSequenceNumberOfItems, coolDownTempStartLocation);
  
  temperatureForTargetTemperatureNotification = coneSequenceValues[ConeTargetSequenceLocation];
  temperatureForCoolDownNotification = coolDownTargetSequenceValues[CooldownTempSequenceLocation];
  
  UserSetVariables.ConeTargetSequenceLocation = ConeTargetSequenceLocation;
  UserSetVariables.CoolDownTargetSequenceLocation = CooldownTempSequenceLocation;
  UserSetVariables.HasSavedValues = true;

  Storage_UserSetVariables.write(UserSetVariables);
}

bool CheckForThermocoupleFault(uint8_t fault)
{
  if (fault)
  {
    LED_Thermocouple_Status.setStatus(LED_Thermocouple_Status.BLINK);
    Serial.println("DEBUG: Thermocouple Fault Detected");
    WriteThermocoupleFaultToDisplay("Thermocouple Error");

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
    notification += coneSequenceLabels[UserSetVariables.ConeTargetSequenceLocation];
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
    notification += coneSequenceLabels[UserSetVariables.ConeTargetSequenceLocation];
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

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  
  //for debugging purposes, wait for serial port to connect
  // while (!Serial) {;}

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.display();

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

  WiFi.beginProvision();
  Serial.println("completed WiFi.beginProvision()");    

  while (WiFi.status() != WL_CONNECTED)
  {
    LED_Wifi_Status.setStatus(LED_Wifi_Status.BLINK);
    LED_Wifi_Status.updateLED();
    WriteProvisioningInstructions();
    Serial.print(".");
  }

  LED_Wifi_Status.setStatus(LED_Wifi_Status.ON);

  Blynk.config(BLYNK_AUTH_TOKEN);
  Serial.println("Connected to Wifi AP:");
  Serial.println(WiFi.SSID());
  WriteWiFiStatusToDisplay(WiFi.SSID(), ipToString(WiFi.localIP()));

  UserSetVariables = Storage_UserSetVariables.read();

  if (!UserSetVariables.HasSavedValues) 
  {
    Serial.println("No user saved values, show menu");
    ShowMenu();
  } 
  else 
  {
    temperatureForTargetTemperatureNotification = coneSequenceValues[UserSetVariables.ConeTargetSequenceLocation];
    temperatureForCoolDownNotification = coolDownTargetSequenceValues[UserSetVariables.CoolDownTargetSequenceLocation];
    
    Serial.println("UserSetVariables....");
    Serial.print("target cone sequence location: ");
    Serial.print(UserSetVariables.ConeTargetSequenceLocation);
    Serial.println(".......");

    Serial.print("cool down temp: ");
    Serial.print(UserSetVariables.CoolDownTargetSequenceLocation);
    Serial.println(".......");
  }

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
  //update SSID and IP as sometimes the Wifi101 library returned a blank SSID
  WriteWiFiStatusToDisplay(WiFi.SSID(), ipToString(WiFi.localIP()));

  LED_Power_Status.updateLED();
  LED_Thermocouple_Status.updateLED();
  LED_Wifi_Status.updateLED();
  LED_BLYNK_Status.updateLED();

  if (!digitalRead(BUTTON_A)) {
    //TODO: re-test whether it the A button still needed a longer delay
    delay(DEBOUNCE * 2); //to prevent a double a button click, for some reason, it needed a longer delay
    ShowMenu();
  }
  
}