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
#include <Preferences.h>


LaserSensor laserSensor(LASER_SENSOR_PIN);
StatusLed statusLed(12);
// SD card pins: CS=4, MOSI=3 MISO=1, CLK=2
SdCardManager sdCard(4, 3, 1, 2);
WebServerManager webServer;
Preferences preferences;
unsigned long bestTime = 0;
unsigned long totalTime = 0;
int finishedCount = 0;

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
    webServer.begin(&sdCard);
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

    // SD card disabled due to pin conflicts (optional, for future use)
    Serial.println("\nSD Card: DISABLED (pins 1,3 conflict with Serial)");
    Serial.println("Times are saved to SPIFFS internal storage instead.");
    
    Serial.println("\n=== SETUP COMPLETE ===\n");
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
            
            // Save to SPIFFS (auto-saves in addElapsed)
            webServer.addElapsed(lastTime);
            Serial.println("Time saved to SPIFFS.");
            
            // Optionally log to SD card if available
            if (sdCard.isReady()) {
                if (sdCard.logTime(lastTime)) {
                    Serial.println("Also logged to SD card.");
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