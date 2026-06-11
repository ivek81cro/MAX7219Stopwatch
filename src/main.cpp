#include <Arduino.h>
#include "configuration.h"
#include "LaserSensor.h"
#include "Stopwatch.h"
#include "StopwatchDisplay.h"
#include "WebServerManager.h"
#include <WiFi.h>
#include "StatusLed.h"
#if ENABLE_SERIAL_TRIGGER_TEST
#include "SerialTriggerTest.h"
#endif
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>


LaserSensor laserSensor(LASER_SENSOR_PIN);
StatusLed statusLed(12);
WebServerManager webServer;
Preferences preferences;
unsigned long bestTime = 0;
unsigned long totalTime = 0;
int finishedCount = 0;

namespace {
enum class RaceState {
    Idle,
    Running,
    Finished
};

RaceState raceState = RaceState::Idle;
bool lastLaserState = true;
unsigned long ignoreUntil = 0;
unsigned long lastDisplayUpdateMs = 0;

void resetRaceTracking() {
    if (Stopwatch::getInstance().isRunning()) {
        raceState = RaceState::Running;
    } else if (Stopwatch::getInstance().isStopped()) {
        raceState = RaceState::Finished;
    } else {
        raceState = RaceState::Idle;
    }
    ignoreUntil = 0;
    lastLaserState = laserSensor.isActive();
}
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial monitor to connect
    Serial.println("\n\n=== STOPWATCH STARTING ===");
    
    Serial.println("Initializing peripherals...");
    laserSensor.begin();
    StopwatchDisplay::getInstance(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES, TOTAL_COLUMNS).begin();
    StopwatchDisplay::getInstance().setIntensity(8); // Set medium brightness
    StopwatchDisplay::getInstance().clear();
    statusLed.begin();
    Serial.println("Peripherals initialized.");

    // Load WiFi credentials from NVS (non-volatile storage)
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();
    
    bool credsOk = (ssid.length() > 0);
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

    Serial.println("Mounting SPIFFS...");
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
    } else {
        Serial.println("SPIFFS mounted successfully.");
    }

    Serial.println("Starting web server...");
    webServer.setTrackingResetHandler(resetRaceTracking);
    webServer.begin();
    Serial.println("Web server started.");
    
    // Load saved times from SPIFFS
    Serial.println("\nLoading saved times from SPIFFS...");
    if (webServer.loadTimes()) {
        Serial.println("Times loaded successfully.");
    } else {
        Serial.println("No previous times found or load failed.");
    }
    
    // Check SPIFFS space
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    Serial.printf("SPIFFS: %d KB used / %d KB total (%.1f%% full)\n", 
                  usedBytes/1024, totalBytes/1024, 
                  (usedBytes * 100.0) / totalBytes);

    Serial.println("\n=== SETUP COMPLETE ===\n");
#if ENABLE_SERIAL_TRIGGER_TEST
    SerialTriggerTest::begin();
#endif
}

static void updateDisplayFromElapsed(unsigned long elapsedMs) {
    const unsigned long now = millis();
    if (now - lastDisplayUpdateMs < 20) {
        return;
    }
    lastDisplayUpdateMs = now;

    static char lastBuffer[20] = "";
    char buffer[20];

    unsigned int minutes = elapsedMs / 60000;
    unsigned int minuteDigit = minutes % 10;
    unsigned int seconds = (elapsedMs % 60000) / 1000;
    unsigned int milliseconds = elapsedMs % 1000;
    snprintf(buffer, sizeof(buffer), "%1u:%02u:%03u", minuteDigit, seconds, milliseconds);

    if (strcmp(buffer, lastBuffer) != 0) {
        StopwatchDisplay::getInstance().showTime(buffer);
        strcpy(lastBuffer, buffer);
    }
}

static void printElapsedThrottled(unsigned long now) {
    static unsigned long lastPrintMs = 0;
    if (!Stopwatch::getInstance().isRunning()) {
        return;
    }

    if (now - lastPrintMs >= 250) {
        Stopwatch::getInstance().printElapsed(Serial);
        lastPrintMs = now;
    }
}

void loop() {
#if ENABLE_SERIAL_TRIGGER_TEST
    SerialTriggerTest::handleInput();
#endif

    if (!webServer.isTriggerArmed()) {
        lastLaserState = laserSensor.isActive();
        statusLed.set(false);
        updateDisplayFromElapsed(Stopwatch::getInstance().elapsed());
        webServer.handleClient();
        return;
    }

    bool active = laserSensor.isActive();
#if ENABLE_SERIAL_TRIGGER_TEST
    unsigned long now = millis();
    if (SerialTriggerTest::consumeTriggerIfReady(now, ignoreUntil)) {
        // Force one loop pass to look like beam interruption.
        active = false;
    }
#else
    unsigned long now = millis();
#endif

    statusLed.set(active);
    // Ignore sensor for 3 seconds after each break
    if (now < ignoreUntil) {
        lastLaserState = active;
        updateDisplayFromElapsed(Stopwatch::getInstance().elapsed());
        printElapsedThrottled(now);
        webServer.handleClient();
        return;
    }

    // Detect transition from ACTIVE to INACTIVE
    if (lastLaserState && !active) {
        ignoreUntil = now + 3000; // ignore for next 3 seconds
        switch (raceState) {
            case RaceState::Idle:
                Stopwatch::getInstance().start();
                raceState = RaceState::Running;
                Serial.println("Stopwatch started!");
                break;
            case RaceState::Running: {
                Stopwatch::getInstance().stop();
                Stopwatch::getInstance().printResult(Serial);
                const unsigned long lastTime = Stopwatch::getInstance().elapsed();

                webServer.addElapsed(lastTime);
                Serial.println("Time saved to SPIFFS.");

                if (bestTime == 0 || lastTime < bestTime) {
                    bestTime = lastTime;
                }
                totalTime += lastTime;
                finishedCount++;
                const unsigned long avgTime = finishedCount ? totalTime / finishedCount : 0;
                webServer.updateStats(lastTime, bestTime, avgTime, finishedCount);
                raceState = RaceState::Finished;
                break;
            }
            case RaceState::Finished:
                Stopwatch::getInstance().reset();
                Stopwatch::getInstance().start();
                raceState = RaceState::Running;
                Serial.println("Stopwatch reset and new run started.");
                break;
        }
    }
    lastLaserState = active;

    updateDisplayFromElapsed(Stopwatch::getInstance().elapsed());
    printElapsedThrottled(now);

    webServer.handleClient();
}
