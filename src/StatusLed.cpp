#include "StatusLed.h"

StatusLed::StatusLed(uint8_t pin) : _pin(pin) {}

void StatusLed::begin() const {
    pinMode(_pin, OUTPUT);
}

void StatusLed::set(bool on) const {
    digitalWrite(_pin, on ? HIGH : LOW);
}
