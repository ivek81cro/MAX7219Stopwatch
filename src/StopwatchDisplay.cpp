#include "StopwatchDisplay.h"

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

void StopwatchDisplay::showTime(const char* timeStr) {
    _mx.clear();
    // Mirror the string for display
    uint8_t len = strlen(timeStr);
    char mirrored[20];
    for (uint8_t i = 0; i < len; i++) {
        mirrored[i] = timeStr[len - 1 - i];
    }
    mirrored[len] = '\0';
    int col = _totalColumns - 1;
    for (int i = len - 1; i >= 0; i--) {
        if (mirrored[i] == ':') {
            // Draw colon as two dots, 2px apart vertically
            // Example: dots at row 2 and row 5 (bit 2 and bit 5)
            _mx.setColumn(col, 0x24); // 0b00100100, dots at rows 2 and 5
            col -= 2; // 1px for colon, 1px space
        } else {
            _mx.setChar(col, mirrored[i]);
            col -= 6; // 5px char + 1px space
        }
        if (col < 0) break;
    }
}
