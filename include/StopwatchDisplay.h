#ifndef STOPWATCHDISPLAY_H
#define STOPWATCHDISPLAY_H

#include <MD_MAX72XX.h>
#include <Arduino.h>

class StopwatchDisplay {
public:
    // Singleton: first call must provide parameters, subsequent calls ignore them
    static StopwatchDisplay& getInstance(
        MD_MAX72XX::moduleType_t hardwareType = MD_MAX72XX::FC16_HW,
        uint8_t dataPin = 23,
        uint8_t clkPin = 18,
        uint8_t csPin = 5,
        uint8_t maxDevices = 4,
        uint8_t totalColumns = 32);
    void begin();
    void showTime(const char* timeStr);
    void setIntensity(uint8_t intensity);
    void clear();
private:
    StopwatchDisplay(MD_MAX72XX::moduleType_t hardwareType, uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t maxDevices, uint8_t totalColumns);
    StopwatchDisplay(const StopwatchDisplay&) = delete;
    StopwatchDisplay& operator=(const StopwatchDisplay&) = delete;
    static StopwatchDisplay* _instance;
    MD_MAX72XX _mx;
    uint8_t _totalColumns;
};

#endif // STOPWATCHDISPLAY_H
