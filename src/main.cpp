
#include <Arduino.h>
#include "configuration.h"

#include "LaserSensor.h"

#include "Stopwatch.h"


#include "StopwatchDisplay.h"
#include "WebServerManager.h"
#include <WiFi.h>




LaserSensor laserSensor(LASER_SENSOR_PIN);
Stopwatch stopwatch;


StopwatchDisplay stopwatchDisplay(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES, TOTAL_COLUMNS);




WebServerManager webServer;
unsigned long bestTime = 0;
unsigned long totalTime = 0;
int finishedCount = 0;

void setup() {
    Serial.begin(115200);
    laserSensor.begin();
    stopwatchDisplay.begin();
    stopwatchDisplay.setIntensity(8); // Set medium brightness
    stopwatchDisplay.clear();

    // Set up ESP32 as Access Point
    const char* ap_ssid = "StopwatchAP";
    const char* ap_password = "12345678";
    WiFi.softAP(ap_ssid, ap_password);
    delay(100); // Give AP time to start
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("ESP32 AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP address: ");
    Serial.println(apIP);

    webServer.begin();
}




int transitionCount = 0;
bool lastLaserState = true; // assume starts ACTIVE
unsigned long ignoreUntil = 0;

// ...existing code...

void loop() {
    bool active = laserSensor.isActive();
    // Ignore sensor for 3 seconds after each break
    unsigned long now = millis();
    if (now < ignoreUntil) {
        lastLaserState = active;
        // ...existing code...
        static char lastBuffer[20] = "";
        char buffer[20];
        unsigned long e = stopwatch.elapsed();
        unsigned int minutes = e / 60000;
        unsigned int seconds = (e % 60000) / 1000;
        unsigned int centiseconds = (e % 1000) / 10; // two decimals
        sprintf(buffer, "%02u:%02u:%02u", minutes, seconds, centiseconds);
        if (strcmp(buffer, lastBuffer) != 0) {
            stopwatchDisplay.showTime(buffer);
            strcpy(lastBuffer, buffer);
        }
        if (stopwatch.isRunning()) {
            stopwatch.printElapsed(Serial);
        }
        delay(50);
        return;
    }

    // Detect transition from ACTIVE to INACTIVE
    if (lastLaserState && !active) {
        transitionCount++;
        ignoreUntil = now + 3000; // ignore for next 3 seconds
        if (transitionCount == 1) {
            stopwatch.start();
            Serial.println("Stopwatch started!");
        } else if (transitionCount == 2 && stopwatch.isRunning()) {
            stopwatch.stop();
            stopwatch.printResult(Serial);
            unsigned long lastTime = stopwatch.elapsed();
            webServer.addElapsed(lastTime);
            if (bestTime == 0 || lastTime < bestTime) bestTime = lastTime;
            totalTime += lastTime;
            finishedCount++;
            unsigned long avgTime = finishedCount ? totalTime / finishedCount : 0;
            webServer.updateStats(lastTime, bestTime, avgTime, finishedCount);
        } else if (transitionCount == 3 && stopwatch.isStopped()) {
            stopwatch.reset();
            transitionCount = 0;
            Serial.println("Stopwatch reset and ready for new start.");
        }
    }
    lastLaserState = active;

    static char lastBuffer[20] = "";
    char buffer[20];
    unsigned long e = stopwatch.elapsed();
    unsigned int minutes = e / 60000;
    unsigned int seconds = (e % 60000) / 1000;
    unsigned int centiseconds = (e % 1000) / 10; // two decimals
    sprintf(buffer, "%02u:%02u:%02u", minutes, seconds, centiseconds);
    if (strcmp(buffer, lastBuffer) != 0) {
        stopwatchDisplay.showTime(buffer);
        strcpy(lastBuffer, buffer);
    }

    if (stopwatch.isRunning()) {
        stopwatch.printElapsed(Serial);
    }

    webServer.handleClient();
    delay(50);
}