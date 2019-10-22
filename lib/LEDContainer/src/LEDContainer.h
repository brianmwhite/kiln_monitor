#ifndef LEDContainer_H
#define LEDContainer_H

class LEDContainer
{
private:
    int PIN;
    int NEXT_TIME_TO_BLINK;
    int LED_STATE;
    int BLINK_DELAY_MILLISECONDS;
    int READY_STATE;

public:
    LEDContainer();
    void init(int pin);
    void init(int pin, int blink_delay_in_milliseconds);
    void setReadyState(int state);
    void updateLED();
};

#endif