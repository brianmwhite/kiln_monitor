#ifndef LEDContainer_H
#define LEDContainer_H

class LEDContainer
{
private:
    int PIN;
    int NEXT_TIME_TO_BLINK;
    int LED_STATE;

public:
    LEDContainer();
    int READY_STATE;
    int BLINK_DELAY_MILLISECONDS;
    void init(int pin);
    void updateLED();
};

#endif