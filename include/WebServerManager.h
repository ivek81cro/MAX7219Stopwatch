
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
    void updateStats(unsigned long lastTime, unsigned long bestTime);
    void addElapsed(unsigned long elapsed);
    void handleClient();
    bool saveTimes();
    bool loadTimes();
    bool isTriggerArmed() const;
    bool setTriggerArmed(bool armed);
    uint8_t getDisplayBrightness() const;
    bool setDisplayBrightness(uint8_t brightness);
    void setTrackingResetHandler(std::function<void()> handler);
    unsigned long getRevision() const;
private:
    static const size_t MAX_TIMES = 10000; // Max 10000 times (~40KB in SPIFFS)
    WebServer _server;
    unsigned long _lastTime;
    unsigned long _bestTime;
    unsigned long _revision;
    bool _triggerArmed;
    uint8_t _displayBrightness;
    std::vector<unsigned long> _elapsedTimes;
    std::function<void()> _trackingResetHandler;
    String _cachedTimesJson;
    String _cachedStatsJson;
    String _cachedDashboardJson;
    bool _timesJsonDirty;
    bool _statsJsonDirty;
    bool _dashboardJsonDirty;
    void handleRoot();
    void handleWifiForm();
    void handleWifiSave();
    void handleClear();
    void handleReset();
    void handleTriggerState();
    void handleTriggerControl();
    void handleBrightnessState();
    void handleBrightnessControl();
    void handleRevisionState();
    void handleDashboardState();
    String formatTime(unsigned long ms) const;
    String buildTriggerStateJson() const;
    String buildBrightnessStateJson() const;
    String buildRevisionJson() const;
    const String& buildTimesJson();
    const String& buildStatsJson();
    const String& buildDashboardJson();
    void invalidateTimesCache();
    void invalidateStatsCache();
    void invalidateDashboardCache();
    void touchRevision();
};

#endif // MAX7219STOPWATCH_WEBSERVERMANAGER_H
