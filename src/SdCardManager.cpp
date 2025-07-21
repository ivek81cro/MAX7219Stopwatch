#include "SdCardManager.h"
#include <SPI.h>

SdCardManager::SdCardManager(uint8_t csPin, uint8_t mosiPin, uint8_t misoPin, uint8_t clkPin)
    : _csPin(csPin), _mosiPin(mosiPin), _misoPin(misoPin), _clkPin(clkPin), _ready(false) {}

bool SdCardManager::begin() {
    SPI.begin(_clkPin, _misoPin, _mosiPin, _csPin);
    _ready = SD.begin(_csPin, SPI);
    return _ready;
}

bool SdCardManager::isReady() const {
    return _ready;
}

bool SdCardManager::logTime(unsigned long ms) {
    if (!_ready) return false;
    File file = SD.open("/times.csv", FILE_WRITE);
    if (!file) return false;
    unsigned int minutes = ms / 60000;
    unsigned int seconds = (ms % 60000) / 1000;
    unsigned int milliseconds = ms % 1000;
    char buf[20];
    sprintf(buf, "%02u:%02u:%03u\n", minutes, seconds, milliseconds);
    file.print(buf);
    file.close();
    return true;
}
