#include <Arduino.h>
#include "SdCardManager.h"

SdCardManager sdCard(13, 15, 2, 14);

void setup() {
    Serial.begin(115200);
    bool ok = sdCard.begin();
    Serial.print("SD init: ");
    Serial.println(ok ? "OK" : "FAIL");

    // Test write
    unsigned long testTime = 123456;
    bool writeOk = sdCard.logTime(testTime);
    Serial.print("Write test: ");
    Serial.println(writeOk ? "OK" : "FAIL");

    // Test read
    File file = SD.open("/times.csv", FILE_READ);
    if (file) {
        Serial.println("Read test: OK");
        while (file.available()) {
            Serial.write(file.read());
        }
        file.close();
    } else {
        Serial.println("Read test: FAIL");
    }
}

void loop() {}
