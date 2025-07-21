#ifndef STATUSLED_H
#define STATUSLED_H

#include <Arduino.h>

class StatusLed {
public:
    explicit StatusLed(uint8_t pin);
    void begin() const;
    void set(bool on) const;
private:
    uint8_t _pin;
};

#endif // STATUSLED_H
