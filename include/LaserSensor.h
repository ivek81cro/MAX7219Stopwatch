#ifndef LASERSENSOR_H
#define LASERSENSOR_H

#include <Arduino.h>

class LaserSensor {
public:
    explicit LaserSensor(uint8_t pin);
    void begin() const;
    bool isActive() const;
private:
    uint8_t _pin;
};

#endif // LASERSENSOR_H
