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
  PIN = pin;
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW);
}

void LEDContainer::updateLED()
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