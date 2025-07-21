#include <Arduino.h>
#include "SdCardManager.h"
#include <SPI.h>
#include <Arduino.h>

bool SdCardManager::saveWifiCredentials(const String& ssid, const String& password) {
    if (!_ready) return false;
    File file = SD.open("/wifi.txt", FILE_WRITE);
    if (!file) return false;
    file.println(ssid);
    file.println(password);
    file.close();
    return true;
}

bool SdCardManager::loadWifiCredentials(String& ssid, String& password) {
    if (!_ready) return false;
    File file = SD.open("/wifi.txt", FILE_READ);
    if (!file) return false;
    ssid = file.readStringUntil('\n');
    ssid.trim();
    password = file.readStringUntil('\n');
    password.trim();
    file.close();
    return ssid.length() > 0 && password.length() > 0;
}


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
    if (!_ready) {
        Serial.println("SD not ready");
        return false;
    }
    File file = SD.open("/times.csv", FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open /times.csv for writing");
        return false;
    }
    unsigned int minutes = ms / 60000;
    unsigned int seconds = (ms % 60000) / 1000;
    unsigned int milliseconds = ms % 1000;
    char buf[20];
    sprintf(buf, "%02u:%02u:%03u\n", minutes, seconds, milliseconds);
    file.print(buf);
    Serial.print("Writing to SD: ");
    Serial.print(buf);
    file.close();
    Serial.println("File closed");
    return true;
}