
#ifndef MAX7219STOPWATCH_WEBSERVERMANAGER_H
#define MAX7219STOPWATCH_WEBSERVERMANAGER_H

#include <WebServer.h>
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include <functional>

class WebServerManager {
public:
    WebServerManager(uint16_t port = 80);
    void begin();
    void updateStats(unsigned long lastTime, unsigned long bestTime, unsigned long avgTime, int count);
    void addElapsed(unsigned long elapsed);
    void handleClient();
    bool saveTimes();
    bool loadTimes();
    bool isTriggerArmed() const;
    bool setTriggerArmed(bool armed);
    void setTrackingResetHandler(std::function<void()> handler);
private:
    static const size_t MAX_TIMES = 10000; // Max 10000 times (~40KB in SPIFFS)
    WebServer _server;
    unsigned long _lastTime;
    unsigned long _bestTime;
    unsigned long _avgTime;
    int _count;
    bool _triggerArmed;
    std::vector<unsigned long> _elapsedTimes;
    std::function<void()> _trackingResetHandler;
    void handleRoot();
    void handleWifiForm();
    void handleWifiSave();
    void handleClear();
    void handleReset();
    void handleTriggerState();
    void handleTriggerControl();
    String formatTime(unsigned long ms) const;
    String buildTriggerStateJson() const;
};

#endif // MAX7219STOPWATCH_WEBSERVERMANAGER_H
