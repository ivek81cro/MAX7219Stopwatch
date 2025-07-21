#ifndef SDCARDMANAGER_H
#define SDCARDMANAGER_H

#include <Arduino.h>
#include <SD.h>

class SdCardManager {
public:
    SdCardManager(uint8_t csPin, uint8_t mosiPin, uint8_t misoPin, uint8_t clkPin);
    bool begin();
    bool logTime(unsigned long ms);
    bool isReady() const;
    bool saveWifiCredentials(const String& ssid, const String& password);
    bool loadWifiCredentials(String& ssid, String& password);
private:
    uint8_t _csPin, _mosiPin, _misoPin, _clkPin;
    bool _ready;
};

#endif // SDCARDMANAGER_H
