#include "SerialTriggerTest.h"
#include "configuration.h"

#if ENABLE_SERIAL_TRIGGER_TEST

namespace SerialTriggerTest {

namespace {
bool triggerPending = false;
}

void begin() {
    Serial.println("Serial test mode ready.");
    Serial.println("Type 't' in Serial Monitor to simulate one laser trigger.");
    Serial.println("Type 'h' or '?' for help.");
}

void handleInput() {
    while (Serial.available() > 0) {
        const char cmd = static_cast<char>(Serial.read());

        if (cmd == 't' || cmd == 'T') {
            triggerPending = true;
            Serial.println("[SERIAL TEST] Trigger queued (ACTIVE -> INACTIVE)");
        } else if (cmd == 'h' || cmd == 'H' || cmd == '?') {
            Serial.println("[SERIAL TEST] Commands:");
            Serial.println("  t - simulate laser break trigger");
            Serial.println("  h/? - show this help");
        }
    }
}

bool consumeTriggerIfReady(unsigned long now, unsigned long ignoreUntil) {
    if (!triggerPending) {
        return false;
    }

    if ((long)(now - ignoreUntil) < 0) {
        return false;
    }

    triggerPending = false;
    return true;
}

} // namespace SerialTriggerTest

#endif
