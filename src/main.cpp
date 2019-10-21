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
// #define LED_THERMO 4

#define spi_cs 12
#define spi_mosi 13
#define spi_miso 14
#define spi_clk 15

class LED_Container
{
public:
  int PIN;
  int LED_STATE = LOW;
  int READY_STATE = false;
  int BLINK_DELAY_MILLISECONDS = 1000;
  int NEXT_TIME_TO_BLINK = 0;

  void init(int pin)
  {
    PIN = pin;
    pinMode(PIN, OUTPUT);
    digitalWrite(PIN, LOW);
  }

  void updateLED()
  {
    if (READY_STATE)
    {
      digitalWrite(PIN, HIGH);
    }
    else
    {
      int TIME_NOW = millis();
      if (TIME_NOW >= NEXT_TIME_TO_BLINK)
      {
        if (LED_STATE == HIGH)
        {
          LED_STATE = LOW;
        }
        else
        {
          LED_STATE = HIGH;
        }
        digitalWrite(PIN, LED_STATE);
        NEXT_TIME_TO_BLINK = TIME_NOW + BLINK_DELAY_MILLISECONDS;
      }
    }
  }
};

LED_Container LED_Thermocouple;

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
      LED_Thermocouple.READY_STATE = false;

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
    LED_Thermocouple.READY_STATE = true;

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

void toggleLED(LED_Container ledToToggle)
{
  if (ledToToggle.LED_STATE == HIGH) 
  {
    ledToToggle.LED_STATE = LOW;
  }
  else {
    ledToToggle.LED_STATE = HIGH;
  }
  digitalWrite(ledToToggle.PIN, ledToToggle.LED_STATE);
}

void setup() {
  Serial.begin(115200);

  LED_Thermocouple.init(4);

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_BLYNK, OUTPUT);

  digitalWrite(LED_POWER, LOW);
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_BLYNK, LOW);

  Blynk.begin(auth, ssid, pass);
  digitalWrite(LED_WIFI, HIGH);

  timer.setInterval(1000L, TemperatureTimeProcess);
  
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);
}

void loop() {
  Blynk.run();
  timer.run();
  LED_Thermocouple.updateLED();
  digitalWrite(LED_BLYNK, Blynk.connected());
  digitalWrite(LED_POWER, HIGH);

}