#include <stddef.h>
#include "StopwatchDisplay.h"

StopwatchDisplay* StopwatchDisplay::_instance = nullptr;

StopwatchDisplay& StopwatchDisplay::getInstance(
    MD_MAX72XX::moduleType_t hardwareType,
    uint8_t dataPin,
    uint8_t clkPin,
    uint8_t csPin,
    uint8_t maxDevices,
    uint8_t totalColumns) {
    if (!_instance) {
        _instance = new StopwatchDisplay(hardwareType, dataPin, clkPin, csPin, maxDevices, totalColumns);
    }
    return *_instance;
}

StopwatchDisplay::StopwatchDisplay(MD_MAX72XX::moduleType_t hardwareType, uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t maxDevices, uint8_t totalColumns)
    : _mx(hardwareType, dataPin, clkPin, csPin, maxDevices), _totalColumns(totalColumns) {}

void StopwatchDisplay::begin() {
    _mx.begin();
    _mx.clear();
}

void StopwatchDisplay::setIntensity(uint8_t intensity) {
    _mx.control(MD_MAX72XX::INTENSITY, intensity);
}

void StopwatchDisplay::clear() {
    _mx.clear();
}

void StopwatchDisplay::setFlipUpsideDown(bool flip) {
    _flipUpsideDown = flip;
}

void StopwatchDisplay::showTime(const char* timeStr) {
    _mx.update(MD_MAX72XX::OFF);
    _mx.clear();

    if (timeStr == nullptr) {
        return;
    }

    uint8_t len = strlen(timeStr);
    if (len == 0) {
        return;
    }

    // Format m:ss:mmm is 7 chars; if a longer string is provided,
    // render only the right-most 7 chars so the display stays aligned.
    const char* renderStr = timeStr;
    if (len > 9) {
        renderStr = timeStr + (len - 9);
        len = 9;
    }

    int col = _totalColumns - 1;
    for (int i = 0; i < len; i++) {
        if (renderStr[i] == ':') {
            _mx.setColumn(col, 0x24); // 0b00100100, dots at rows 2 and 5
            col -= 2;
        } else {
            _mx.setChar(col, renderStr[i]);
            col -= 6;
        }
        if (col < 0) break;
    }

    if (_flipUpsideDown) {
        _mx.transform(MD_MAX72XX::TFLR);
        _mx.transform(MD_MAX72XX::TFUD);
    }

    _mx.update();
    _mx.update(MD_MAX72XX::ON);
}
