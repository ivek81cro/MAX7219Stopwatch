#include "LaserSensor.h"

LaserSensor::LaserSensor(uint8_t pin) : _pin(pin) {}

void LaserSensor::begin() const {
    pinMode(_pin, INPUT_PULLUP);
}

bool LaserSensor::isActive() const {
    return digitalRead(_pin) == HIGH;
}
