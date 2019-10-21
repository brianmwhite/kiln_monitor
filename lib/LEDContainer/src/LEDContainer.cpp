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
}

void LEDContainer::setStatus(int status) {
  READY_STATE = status;
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