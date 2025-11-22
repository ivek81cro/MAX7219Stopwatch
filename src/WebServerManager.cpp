#include "WebServerManager.h"
#include "SdCardManager.h"
#include "StopwatchDisplay.h"
#include "Stopwatch.h"
#include <vector>
#include <algorithm>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

WebServerManager::WebServerManager(uint16_t port) : _server(port), _lastTime(0), _bestTime(0), _avgTime(0), _count(0), _elapsedTimes() {}


void WebServerManager::begin(SdCardManager* sdCard) {
    _sdCard = sdCard;
    _server.on("/", std::bind(&WebServerManager::handleRoot, this));
    _server.on("/index.html", std::bind(&WebServerManager::handleRoot, this));
    _server.on("/wifi", std::bind(&WebServerManager::handleWifiForm, this));
    _server.on("/savewifi", HTTP_POST, std::bind(&WebServerManager::handleWifiSave, this));
    _server.on("/clear", HTTP_POST, std::bind(&WebServerManager::handleClear, this));
    _server.on("/reset", HTTP_POST, std::bind(&WebServerManager::handleReset, this));
    _server.on("/style.css", HTTP_GET, [this]() {
        File file = SPIFFS.open("/style.css", "r");
        if (file) {
            String css = file.readString();
            file.close();
            _server.send(200, "text/css", css);
        } else {
            _server.send(404, "text/plain", "style.css not found");
        }
    });
    _server.on("/api/times", HTTP_GET, [this]() {
        String json = "[";
        for (size_t i = 0; i < _elapsedTimes.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + formatTime(_elapsedTimes[i]) + "\"";
        }
        json += "]";
        _server.send(200, "application/json", json);
    });
    
    _server.on("/api/stats", HTTP_GET, [this]() {
        // Calculate stats
        unsigned long best = 0;
        if (!_elapsedTimes.empty()) {
            best = *std::min_element(_elapsedTimes.begin(), _elapsedTimes.end());
        }
        
        String json = "{";
        json += "\"last\":\"" + formatTime(_lastTime) + "\",";
        json += "\"best\":\"" + formatTime(best) + "\",";
        json += "\"count\":" + String(_elapsedTimes.size());
        json += "}";
        _server.send(200, "application/json", json);
    });
    
    // Handle 404 for all other requests (favicon, etc)
    _server.onNotFound([this]() {
        String uri = _server.uri();
        Serial.print("404 Not Found: ");
        Serial.println(uri);
        if (uri == "/favicon.ico") {
            _server.send(204); // No content for favicon
        } else {
            _server.send(404, "text/plain", "Not Found");
        }
    });
    
    _server.begin();
}

void WebServerManager::handleWifiForm() {
    File file = SPIFFS.open("/wifi.html", "r");
    if (file) {
        String html = file.readString();
        file.close();
        _server.send(200, "text/html", html);
    } else {
        _server.send(500, "text/plain", "wifi.html not found");
    }
}

void WebServerManager::handleWifiSave() {
    String ssid = _server.arg("ssid");
    String password = _server.arg("password");
    
    // Save to NVS (Preferences)
    Preferences prefs;
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();
    
    _server.send(200, "text/html", "<html><body>WiFi credentials saved to NVS! Rebooting...</body></html>");
    delay(500);
    ESP.restart();
}


void WebServerManager::updateStats(unsigned long lastTime, unsigned long bestTime, unsigned long avgTime, int count) {
    _lastTime = lastTime;
    _bestTime = bestTime;
}

void WebServerManager::addElapsed(unsigned long elapsed) {
    // Check for overflow protection
    if (_elapsedTimes.size() >= MAX_TIMES) {
        Serial.println("WARNING: Max times limit reached. Removing oldest entry.");
        _elapsedTimes.erase(_elapsedTimes.begin()); // Remove oldest
    }
    _elapsedTimes.push_back(elapsed);
    saveTimes(); // Auto-save to SPIFFS
}

bool WebServerManager::saveTimes() {
    File file = SPIFFS.open("/times.dat", "w");
    if (!file) {
        Serial.println("Failed to open times.dat for writing");
        return false;
    }
    
    // Write count first
    size_t count = _elapsedTimes.size();
    file.write((uint8_t*)&count, sizeof(count));
    
    // Write all times as binary
    for (unsigned long time : _elapsedTimes) {
        file.write((uint8_t*)&time, sizeof(time));
    }
    
    file.close();
    Serial.printf("Saved %d times to SPIFFS\n", count);
    return true;
}

bool WebServerManager::loadTimes() {
    if (!SPIFFS.exists("/times.dat")) {
        Serial.println("No saved times found in SPIFFS");
        return false;
    }
    
    File file = SPIFFS.open("/times.dat", "r");
    if (!file) {
        Serial.println("Failed to open times.dat for reading");
        return false;
    }
    
    // Read count
    size_t count = 0;
    file.read((uint8_t*)&count, sizeof(count));
    
    // Overflow protection
    if (count > MAX_TIMES) {
        Serial.printf("WARNING: File contains %d times, limiting to %d\n", count, MAX_TIMES);
        count = MAX_TIMES;
    }
    
    // Read times
    _elapsedTimes.clear();
    _elapsedTimes.reserve(count);
    
    for (size_t i = 0; i < count && file.available(); i++) {
        unsigned long time = 0;
        if (file.read((uint8_t*)&time, sizeof(time)) == sizeof(time)) {
            _elapsedTimes.push_back(time);
        }
    }
    
    file.close();
    Serial.printf("Loaded %d times from SPIFFS\n", _elapsedTimes.size());
    return true;
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
    String html;
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
        html = file.readString();
        file.close();
        Serial.println("Loaded index.html from SPIFFS");
    } else {
        Serial.println("ERROR: index.html not found in SPIFFS!");
        _server.send(500, "text/html", "<html><body><h1>ERROR: SPIFFS index.html not found</h1><p>Run: platformio run --target uploadfs</p></body></html>");
        return;
    }
    // Generate table rows
    String tableRows;
    std::vector<unsigned long> sorted = _elapsedTimes;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 0; i < sorted.size(); ++i) {
        tableRows += "<tr><td>" + String(i + 1) + "</td><td>" + formatTime(sorted[i]) + "</td></tr>";
    }

    // Always select best time from vector
    unsigned long best = 0;
    if (!sorted.empty()) {
        best = sorted[0]; // sorted ascending, so first is best
    }
    html.replace("%LAST%", formatTime(_lastTime));
    html.replace("%BEST%", formatTime(best));
    html.replace("%TABLE%", tableRows);
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
    _lastTime = 0;
    _bestTime = 0;
    
    // Delete SPIFFS times file
    if (SPIFFS.exists("/times.dat")) {
        SPIFFS.remove("/times.dat");
        Serial.println("Cleared times from SPIFFS");
    }
    
    // Also clear SD if available
    if (_sdCard && _sdCard->isReady()) {
        File file = SD.open("/times.csv", FILE_WRITE);
        if (file) file.close(); // Truncate file to zero length
    }
    
    updateStats(0, 0, 0, 0);
    _server.sendHeader("Location", "/");
    _server.send(303);
}
