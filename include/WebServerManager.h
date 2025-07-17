#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <WebServer.h>
#include <Arduino.h>


#include <vector>
#include <algorithm>

class WebServerManager {
public:
    WebServerManager(uint16_t port = 80);
    void begin();
    void updateStats(unsigned long lastTime, unsigned long bestTime, unsigned long avgTime, int count);
    void addElapsed(unsigned long elapsed);
    void handleClient();
private:
    WebServer _server;
    unsigned long _lastTime;
    unsigned long _bestTime;
    unsigned long _avgTime;
    int _count;
    std::vector<unsigned long> _elapsedTimes;
    void handleRoot();
    String formatTime(unsigned long ms) const;
};

#endif // WEBSERVERMANAGER_H
