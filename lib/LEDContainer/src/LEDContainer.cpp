#include "LEDContainer.h"
#include <Arduino.h>

LEDContainer::LEDContainer()
{
  LED_STATE = LOW;
  READY_STATE = false;
  BLINK_DELAY_MILLISECONDS = 1000;
  NEXT_TIME_TO_BLINK = 0;
}

void LEDContainer::init(int pin)
{
  init(pin, BLINK_DELAY_MILLISECONDS);
}

void LEDContainer::init(int pin, int delay_in_milliseconds)
{
  PIN = pin;
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW);
  BLINK_DELAY_MILLISECONDS = delay_in_milliseconds;
  updateLED();
}

void LEDContainer::setReadyState(bool status) {
  READY_STATE = status;
}

void LEDContainer::updateLED()
{
  if (READY_STATE)
  {
    LED_STATE = HIGH;
  }
  else
  {
    delay(2000);
    int TIME_NOW = millis();

    Serial.println(String("Now1: ") + TIME_NOW);
    Serial.println(String("Next Time To blink: ") + NEXT_TIME_TO_BLINK);

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
      NEXT_TIME_TO_BLINK = TIME_NOW + BLINK_DELAY_MILLISECONDS;

      Serial.println(String("LED State: ") + LED_STATE);
      Serial.println(String("New Next Time to Blink: ") + NEXT_TIME_TO_BLINK);

    }
  }
  digitalWrite(PIN, LED_STATE);
}