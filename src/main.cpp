#include <Arduino.h>
#include "configuration.h"
#include "LaserSensor.h"
#include "Stopwatch.h"
#include "StopwatchDisplay.h"
#include "WebServerManager.h"
#include <WiFi.h>
#include "StatusLed.h"
#include "SdCardManager.h"
#include <FS.h>
#include <SPIFFS.h>




LaserSensor laserSensor(LASER_SENSOR_PIN);
StatusLed statusLed(12);
// SD card pins: CS=4, MOSI=3 MISO=1, CLK=2
SdCardManager sdCard(4, 3, 1, 2);
WebServerManager webServer;
unsigned long bestTime = 0;
unsigned long totalTime = 0;
int finishedCount = 0;

void setup() {
    Serial.begin(115200);
    bool sdOk = sdCard.begin();
    Serial.println(sdOk ? "SD card initialized." : "SD card initialization failed!");
    laserSensor.begin();
    StopwatchDisplay::getInstance(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES, TOTAL_COLUMNS).begin();
    StopwatchDisplay::getInstance().setIntensity(8); // Set medium brightness
    StopwatchDisplay::getInstance().clear();
    statusLed.begin();

    String ssid, password;
    bool credsOk = sdOk && sdCard.loadWifiCredentials(ssid, password);
    if (credsOk) {
        Serial.print("Connecting to WiFi SSID: ");
        Serial.println(ssid);
        WiFi.begin(ssid.c_str(), password.c_str());
        int tries = 0;
        while (WiFi.status() != WL_CONNECTED && tries < 20) {
            delay(500);
            Serial.print(".");
            tries++;
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Connected! IP address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("WiFi connect failed, starting AP mode.");
            WiFi.softAP("StopwatchAP", "12345678");
            delay(100);
            Serial.print("ESP32 AP SSID: StopwatchAP\nAP IP address: ");
            Serial.println(WiFi.softAPIP());
        }
    } else {
        WiFi.softAP("StopwatchAP", "12345678");
        delay(100);
        Serial.print("ESP32 AP SSID: StopwatchAP\nAP IP address: ");
        Serial.println(WiFi.softAPIP());
    }

    webServer.begin(&sdCard);

    // Load times from SD card if available
    if (sdOk) {
        File file = SD.open("/times.csv", FILE_READ);
        if (file) {
            while (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) {
                    // Parse mm:ss:ms
                    int firstColon = line.indexOf(':');
                    int secondColon = line.lastIndexOf(':');
                    if (firstColon > 0 && secondColon > firstColon) {
                        unsigned int minutes = line.substring(0, firstColon).toInt();
                        unsigned int seconds = line.substring(firstColon + 1, secondColon).toInt();
                        unsigned int milliseconds = line.substring(secondColon + 1).toInt();
                        unsigned long ms = minutes * 60000UL + seconds * 1000UL + milliseconds;
                        webServer.addElapsed(ms);
                    }
                }
            }
            file.close();
        }
    }

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
    }
}




int transitionCount = 0;
bool lastLaserState = true; // assume starts ACTIVE
unsigned long ignoreUntil = 0;

void loop() {
    bool active = laserSensor.isActive();
    statusLed.set(active);
    // Ignore sensor for 3 seconds after each break
    unsigned long now = millis();
    if (now < ignoreUntil) {
        lastLaserState = active;
        static char lastBuffer[20] = "";
        char buffer[20];
        unsigned long e = Stopwatch::getInstance().elapsed();
        unsigned int minutes = e / 60000;
        unsigned int seconds = (e % 60000) / 1000;
        unsigned int centiseconds = (e % 1000) / 10; // two decimals
        sprintf(buffer, "%02u:%02u:%02u", minutes, seconds, centiseconds);
        if (strcmp(buffer, lastBuffer) != 0) {
            StopwatchDisplay::getInstance().showTime(buffer);
            strcpy(lastBuffer, buffer);
        }
        if (Stopwatch::getInstance().isRunning()) {
            Stopwatch::getInstance().printElapsed(Serial);
        }
        delay(50);
        return;
    }

    // Detect transition from ACTIVE to INACTIVE
    if (lastLaserState && !active) {
        transitionCount++;
        ignoreUntil = now + 3000; // ignore for next 3 seconds
        if (transitionCount == 1) {
            Stopwatch::getInstance().start();
            Serial.println("Stopwatch started!");
        } else if (transitionCount == 2 && Stopwatch::getInstance().isRunning()) {
            Stopwatch::getInstance().stop();
            Stopwatch::getInstance().printResult(Serial);
            unsigned long lastTime = Stopwatch::getInstance().elapsed();
            webServer.addElapsed(lastTime);
            if (sdCard.isReady()) {
                if (sdCard.logTime(lastTime)) {
                    Serial.println("Time logged to SD card.");
                } else {
                    Serial.println("Failed to log time to SD card.");
                }
            }
            if (bestTime == 0 || lastTime < bestTime) bestTime = lastTime;
            totalTime += lastTime;
            finishedCount++;
            unsigned long avgTime = finishedCount ? totalTime / finishedCount : 0;
            webServer.updateStats(lastTime, bestTime, avgTime, finishedCount);
        } else if (transitionCount == 3 && Stopwatch::getInstance().isStopped()) {
            Stopwatch::getInstance().reset();
            transitionCount = 0;
            Serial.println("Stopwatch reset and ready for new start.");
        }
    }
    lastLaserState = active;

    static char lastBuffer[20] = "";
    char buffer[20];
    unsigned long e = Stopwatch::getInstance().elapsed();
    unsigned int minutes = e / 60000;
    unsigned int seconds = (e % 60000) / 1000;
    unsigned int centiseconds = (e % 1000) / 10; // two decimals
    sprintf(buffer, "%02u:%02u:%02u", minutes, seconds, centiseconds);
    if (strcmp(buffer, lastBuffer) != 0) {
        StopwatchDisplay::getInstance().showTime(buffer);
        strcpy(lastBuffer, buffer);
    }

    if (Stopwatch::getInstance().isRunning()) {
        Stopwatch::getInstance().printElapsed(Serial);
    }

    webServer.handleClient();
    delay(50);
}