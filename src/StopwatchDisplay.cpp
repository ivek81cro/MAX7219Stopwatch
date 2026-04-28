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
    if (flip) {
        _mx.transform(MD_MAX72XX::TRC); // Rotate 180 degrees (TRC = Rotate Clockwise)
    } else {
        _mx.clear();  // Reset by clearing
    }
}

void StopwatchDisplay::showTime(const char* timeStr) {
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

    int col = _totalColumns - 1; // Start from position that keeps everything visible
    for (int i = 0; i < len; i++) {
        if (renderStr[i] == ':') {
            // Compact separator column (rows 2 and 5).
            col -= 0; // spacing before colon
            _mx.setColumn(col, 0x24); // 0b00100100, dots at rows 2 and 5
            col -= 2; // spacing after colon
        } else {
            _mx.setChar(col, renderStr[i]);
            col -= 6; // 5px glyph width + 1px spacing
        }
        if (col < 0) break;
    }

    // Apply flip transformation if enabled
    if (_flipUpsideDown) {
        _mx.transform(MD_MAX72XX::TRC); // Rotate 180 degrees
    }
}
