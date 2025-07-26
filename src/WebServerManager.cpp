#include "WebServerManager.h"
#include "SdCardManager.h"
#include "StopwatchDisplay.h"
#include "Stopwatch.h"
#include <vector>
#include <algorithm>

WebServerManager::WebServerManager(uint16_t port) : _server(port), _lastTime(0), _bestTime(0), _avgTime(0), _count(0), _elapsedTimes() {}


void WebServerManager::begin(SdCardManager* sdCard) {
    _sdCard = sdCard;
    _server.on("/", std::bind(&WebServerManager::handleRoot, this));
    _server.on("/wifi", std::bind(&WebServerManager::handleWifiForm, this));
    _server.on("/savewifi", HTTP_POST, std::bind(&WebServerManager::handleWifiSave, this));
    _server.on("/clear", HTTP_POST, std::bind(&WebServerManager::handleClear, this));
    _server.on("/reset", HTTP_POST, std::bind(&WebServerManager::handleReset, this));
    _server.begin();
}

void WebServerManager::handleWifiForm() {
    String html = "<html><head><title>WiFi Setup</title></head><body>";
    html += "<h1>Enter WiFi Credentials</h1>";
    html += "<form method='POST' action='/savewifi'>";
    html += "SSID: <input name='ssid'><br>";
    html += "Password: <input name='password' type='password'><br>";
    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    _server.send(200, "text/html", html);
}

void WebServerManager::handleWifiSave() {
    if (!_sdCard) {
        _server.send(500, "text/plain", "SD not available");
        return;
    }
    String ssid = _server.arg("ssid");
    String password = _server.arg("password");
    if (_sdCard->saveWifiCredentials(ssid, password)) {
        _server.send(200, "text/html", "<html><body>Saved! Rebooting...</body></html>");
        delay(500);
        ESP.restart();
    } else {
        _server.send(500, "text/html", "<html><body>Failed to save credentials.</body></html>");
    }
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
    String html = "<html><head><title>Stopwatch Stats</title>";
    html += "<meta http-equiv='refresh' content='5'>";
    html += "<style>table{border-collapse:collapse;}td,th{border:1px solid #888;padding:4px;}th{background:#eee;}</style></head><body>";
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
    html += "<br><form action='/wifi' method='get'><button type='submit'>WiFi Setup</button></form>";
    html += "<br><form action='/reset' method='post'><button type='submit' style='background:#06c;color:#fff;'>Reset Timer</button></form>";
    html += "<br><form action='/clear' method='post' onsubmit='return confirm(\"Clear all times?\");'><button type='submit' style='background:#c00;color:#fff;'>Clear All Times</button></form>";
    html += "</body></html>";
    _server.send(200, "text/html", html);
}

void WebServerManager::handleReset() {
    updateStats(0, 0, 0, _count);
    StopwatchDisplay::getInstance().showTime("00:00:00");
    Stopwatch::getInstance().reset();
    extern int transitionCount;
    transitionCount = 0;
    _server.sendHeader("Location", "/", true);
    _server.send(302, "text/plain", "Redirecting...");
}

void WebServerManager::handleClear() {
    _elapsedTimes.clear();
    if (_sdCard && _sdCard->isReady()) {
        File file = SD.open("/times.csv", FILE_WRITE);
        if (file) file.close(); // Truncate file to zero length
    }
    updateStats(0, 0, 0, 0);
    _server.sendHeader("Location", "/", true);
    _server.send(302, "text/plain", "Redirecting...");
}
