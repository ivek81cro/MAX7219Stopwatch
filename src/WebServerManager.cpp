#include "WebServerManager.h"


#include <vector>
#include <algorithm>

WebServerManager::WebServerManager(uint16_t port) : _server(port), _lastTime(0), _bestTime(0), _avgTime(0), _count(0), _elapsedTimes() {}

void WebServerManager::begin() {
    _server.on("/", [this]() { handleRoot(); });
    _server.begin();
}


void WebServerManager::updateStats(unsigned long lastTime, unsigned long bestTime, unsigned long avgTime, int count) {
    _lastTime = lastTime;
    _bestTime = bestTime;
    _avgTime = avgTime;
    _count = count;
}

void WebServerManager::addElapsed(unsigned long elapsed) {
    _elapsedTimes.push_back(elapsed);
}

void WebServerManager::handleClient() {
    _server.handleClient();
}

String WebServerManager::formatTime(unsigned long ms) const {
    unsigned int minutes = ms / 60000;
    unsigned int seconds = (ms % 60000) / 1000;
    unsigned int milliseconds = ms % 1000;
    char buf[16];
    sprintf(buf, "%02u:%02u:%03u", minutes, seconds, milliseconds);
    return String(buf);
}

void WebServerManager::handleRoot() {
    String html = "<html><head><title>Stopwatch Stats</title><style>table{border-collapse:collapse;}td,th{border:1px solid #888;padding:4px;}th{background:#eee;}</style></head><body>";
    html += "<h1>Stopwatch Statistics</h1>";
    html += "<p>Last Time: " + formatTime(_lastTime) + "</p>";
    html += "<p>Best Time: " + formatTime(_bestTime) + "</p>";
    html += "<p>Average Time: " + formatTime(_avgTime) + "</p>";
    html += "<p>Count: " + String(_count) + "</p>";
    html += "<h2>All Elapsed Times</h2>";
    html += "<table><tr><th>#</th><th>Time</th></tr>";
    std::vector<unsigned long> sorted = _elapsedTimes;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 0; i < sorted.size(); ++i) {
        html += "<tr><td>" + String(i+1) + "</td><td>" + formatTime(sorted[i]) + "</td></tr>";
    }
    html += "</table>";
    html += "</body></html>";
    _server.send(200, "text/html", html);
}
