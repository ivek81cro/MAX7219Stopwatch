
#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include <Arduino.h>
#include <vector>
#include <algorithm>

class SdCardManager; // Forward declaration

class WebServerManager {
public:
    WebServerManager(uint16_t port = 80);
    void begin(SdCardManager* sdCard = nullptr);
    void updateStats(unsigned long lastTime, unsigned long bestTime, unsigned long avgTime, int count);
    void addElapsed(unsigned long elapsed);
    void handleClient();
    bool saveTimes();
    bool loadTimes();
private:
    static const size_t MAX_TIMES = 10000; // Max 10000 times (~40KB in SPIFFS)
    WebServer _server;
    unsigned long _lastTime;
    unsigned long _bestTime;
    unsigned long _avgTime;
    int _count;
    std::vector<unsigned long> _elapsedTimes;
    SdCardManager* _sdCard = nullptr;
    void handleRoot();
    void handleWifiForm();
    void handleWifiSave();
    void handleClear();
    void handleReset();
    String formatTime(unsigned long ms) const;
};

#endif // WEBSERVERMANAGER_H
