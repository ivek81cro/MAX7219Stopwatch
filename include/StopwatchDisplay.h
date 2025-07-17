#ifndef STOPWATCHDISPLAY_H
#define STOPWATCHDISPLAY_H

#include <MD_MAX72XX.h>
#include <Arduino.h>

class StopwatchDisplay {
public:
    StopwatchDisplay(MD_MAX72XX::moduleType_t hardwareType, uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t maxDevices, uint8_t totalColumns);
    void begin();
    void showTime(const char* timeStr);
    void setIntensity(uint8_t intensity);
    void clear();
private:
    MD_MAX72XX _mx;
    uint8_t _totalColumns;
};

#endif // STOPWATCHDISPLAY_H
