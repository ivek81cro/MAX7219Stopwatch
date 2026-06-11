#include "configuration.h"
#include "LaserSensor.h"

LaserSensor::LaserSensor(uint8_t pin)
    : _pin(pin),
      _lastRawState(true),
      _stableState(true),
      _lastChangeMs(0) {}

void LaserSensor::begin() {
    pinMode(_pin, INPUT_PULLUP);
    const bool initialState = digitalRead(_pin) == HIGH;
    _lastRawState = initialState;
    _stableState = initialState;
    _lastChangeMs = millis();
}

bool LaserSensor::isActive() {
    const bool rawState = digitalRead(_pin) == HIGH;
    const unsigned long now = millis();

    if (rawState != _lastRawState) {
        _lastRawState = rawState;
        _lastChangeMs = now;
    }

    if (now - _lastChangeMs >= LASER_SENSOR_DEBOUNCE_MS) {
        _stableState = rawState;
    }

    return _stableState;
}
