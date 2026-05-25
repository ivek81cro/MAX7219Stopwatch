#ifndef SERIALTRIGGERTEST_H
#define SERIALTRIGGERTEST_H

#include <Arduino.h>

namespace SerialTriggerTest {

void begin();
void handleInput();
bool consumeTriggerIfReady(unsigned long now, unsigned long ignoreUntil);

} // namespace SerialTriggerTest

#endif // SERIALTRIGGERTEST_H
