#ifndef LASERSENSOR_H
#define LASERSENSOR_H

#include <Arduino.h>

class LaserSensor {
public:
    explicit LaserSensor(uint8_t pin);
    void begin();
    bool isActive();
private:
    uint8_t _pin;
    bool _lastRawState;
    bool _stableState;
    unsigned long _lastChangeMs;
};

#endif // LASERSENSOR_H
