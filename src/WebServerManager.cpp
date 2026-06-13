#include "WebServerManager.h"
#include "StopwatchDisplay.h"
#include "Stopwatch.h"
#include <vector>
#include <algorithm>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

WebServerManager::WebServerManager(uint16_t port)
    : _server(port),
      _lastTime(0),
      _bestTime(0),
      _revision(0),
      _triggerArmed(true),
      _displayBrightness(8),
      _elapsedTimes(),
      _trackingResetHandler(),
      _cachedTimesJson(),
      _cachedStatsJson(),
      _cachedDashboardJson(),
      _timesJsonDirty(true),
      _statsJsonDirty(true),
      _dashboardJsonDirty(true) {}


void WebServerManager::begin() {
    _server.on("/", std::bind(&WebServerManager::handleRoot, this));
    _server.on("/index.html", std::bind(&WebServerManager::handleRoot, this));
    // Common captive-portal probes (phones/laptops) should land on app root.
    _server.on("/generate_204", HTTP_GET, [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "Redirecting...");
    });
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "Redirecting...");
    });
    _server.on("/ncsi.txt", HTTP_GET, [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "Redirecting...");
    });
    _server.on("/connecttest.txt", HTTP_GET, [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "Redirecting...");
    });
    _server.on("/fwlink", HTTP_GET, [this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "Redirecting...");
    });
    _server.on("/wifi", std::bind(&WebServerManager::handleWifiForm, this));
    _server.on("/savewifi", HTTP_POST, std::bind(&WebServerManager::handleWifiSave, this));
    _server.on("/clear", HTTP_POST, std::bind(&WebServerManager::handleClear, this));
    _server.on("/reset", HTTP_POST, std::bind(&WebServerManager::handleReset, this));
    _server.on("/api/trigger", HTTP_GET, std::bind(&WebServerManager::handleTriggerState, this));
    _server.on("/api/trigger", HTTP_POST, std::bind(&WebServerManager::handleTriggerControl, this));
    _server.on("/api/brightness", HTTP_GET, std::bind(&WebServerManager::handleBrightnessState, this));
    _server.on("/api/brightness", HTTP_POST, std::bind(&WebServerManager::handleBrightnessControl, this));
    _server.on("/api/revision", HTTP_GET, std::bind(&WebServerManager::handleRevisionState, this));
    _server.on("/api/dashboard", HTTP_GET, std::bind(&WebServerManager::handleDashboardState, this));
    _server.on("/style.css", HTTP_GET, [this]() {
        File file = SPIFFS.open("/style.css", "r");
        if (file) {
            String css = file.readString();
            file.close();
            if (css.length() > 0) {
                _server.send(200, "text/css", css);
            } else {
                _server.send(200, "text/css", "body{font-family:Arial,sans-serif;margin:20px;background:#f4f4f4;color:#333}h1,h2{text-align:center}table{width:100%;max-width:600px;margin:20px auto;border-collapse:collapse;background:#fff}th,td{padding:12px;border:1px solid #ddd;text-align:center}.btn{display:block;width:200px;margin:10px auto;padding:10px;background:#007bff;color:#fff;border:none;border-radius:4px;cursor:pointer}.btn-reset{background:#28a745}.btn-clear{background:#dc3545}");
            }
        } else {
            _server.send(200, "text/css", "body{font-family:Arial,sans-serif;margin:20px;background:#f4f4f4;color:#333}h1,h2{text-align:center}table{width:100%;max-width:600px;margin:20px auto;border-collapse:collapse;background:#fff}th,td{padding:12px;border:1px solid #ddd;text-align:center}.btn{display:block;width:200px;margin:10px auto;padding:10px;background:#007bff;color:#fff;border:none;border-radius:4px;cursor:pointer}.btn-reset{background:#28a745}.btn-clear{background:#dc3545}");
        }
    });
    _server.on("/api/times", HTTP_GET, [this]() {
        _server.send(200, "application/json", buildTimesJson());
    });
    
    _server.on("/api/stats", HTTP_GET, [this]() {
        _server.send(200, "application/json", buildStatsJson());
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

    if (ssid.length() == 0) {
        _server.send(400, "text/html",
            "<html><body>Error: SSID cannot be empty.</body></html>");
        return;
    }
    
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

bool WebServerManager::isTriggerArmed() const {
    return _triggerArmed;
}

bool WebServerManager::setTriggerArmed(bool armed) {
    if (_triggerArmed == armed) {
        return false;
    }

    _triggerArmed = armed;

    if (!armed) {
        if (_trackingResetHandler) {
            _trackingResetHandler();
        }
        Serial.println("Laser trigger disarmed via web.");
    } else {
        if (_trackingResetHandler) {
            _trackingResetHandler();
        }
        Serial.println("Laser trigger armed via web.");
    }

    touchRevision();
    invalidateDashboardCache();

    return true;
}

uint8_t WebServerManager::getDisplayBrightness() const {
    return _displayBrightness;
}

bool WebServerManager::setDisplayBrightness(uint8_t brightness) {
    if (brightness > 15) {
        brightness = 15;
    }

    if (_displayBrightness == brightness) {
        return false;
    }

    _displayBrightness = brightness;
    StopwatchDisplay::getInstance().setIntensity(_displayBrightness);
    Serial.printf("Display brightness set to %u\n", _displayBrightness);
    touchRevision();
    invalidateDashboardCache();
    return true;
}

void WebServerManager::setTrackingResetHandler(std::function<void()> handler) {
    _trackingResetHandler = std::move(handler);
}

unsigned long WebServerManager::getRevision() const {
    return _revision;
}

void WebServerManager::updateStats(unsigned long lastTime, unsigned long bestTime) {
    _lastTime = lastTime;
    _bestTime = bestTime;
    invalidateStatsCache();
    invalidateDashboardCache();
    touchRevision();
}

void WebServerManager::addElapsed(unsigned long elapsed) {
    // Check for overflow protection
    if (_elapsedTimes.size() >= MAX_TIMES) {
        Serial.println("WARNING: Max times limit reached. Removing oldest entry.");
        _elapsedTimes.erase(_elapsedTimes.begin()); // Remove oldest
    }
    _elapsedTimes.push_back(elapsed);
    saveTimes(); // Auto-save to SPIFFS
    invalidateTimesCache();
    invalidateStatsCache();
    invalidateDashboardCache();
    touchRevision();
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
    _bestTime = _elapsedTimes.empty() ? 0 : *std::min_element(_elapsedTimes.begin(), _elapsedTimes.end());
    invalidateTimesCache();
    invalidateStatsCache();
    invalidateDashboardCache();
    return true;
}

void WebServerManager::handleClient() {
    _server.handleClient();
}

String WebServerManager::buildTriggerStateJson() const {
    String json = "{";
    json += "\"armed\":";
    json += _triggerArmed ? "true" : "false";
    json += ",\"label\":\"";
    json += _triggerArmed ? "ARMED" : "DISARMED";
    json += "\"}";
    return json;
}

String WebServerManager::buildBrightnessStateJson() const {
    String json = "{";
    json += "\"value\":";
    json += String(_displayBrightness);
    json += "}";
    return json;
}

String WebServerManager::buildRevisionJson() const {
    String json = "{";
    json += "\"revision\":";
    json += String(_revision);
    json += "}";
    return json;
}

const String& WebServerManager::buildTimesJson() {
    if (!_timesJsonDirty) {
        return _cachedTimesJson;
    }

    String json = "[";
    for (size_t i = 0; i < _elapsedTimes.size(); ++i) {
        if (i > 0) {
            json += ",";
        }
        json += "\"";
        json += formatTime(_elapsedTimes[i]);
        json += "\"";
    }
    json += "]";

    _cachedTimesJson = std::move(json);
    _timesJsonDirty = false;
    return _cachedTimesJson;
}

const String& WebServerManager::buildStatsJson() {
    if (!_statsJsonDirty) {
        return _cachedStatsJson;
    }

    String json = "{";
    json += "\"last\":\"";
    json += formatTime(_lastTime);
    json += "\",\"best\":\"";
    json += formatTime(_bestTime);
    json += "\",\"count\":";
    json += String(_elapsedTimes.size());
    json += "}";

    _cachedStatsJson = std::move(json);
    _statsJsonDirty = false;
    return _cachedStatsJson;
}

const String& WebServerManager::buildDashboardJson() {
    if (!_dashboardJsonDirty) {
        return _cachedDashboardJson;
    }

    String json = "{";
    json += "\"last\":\"";
    json += formatTime(_lastTime);
    json += "\",\"best\":\"";
    json += formatTime(_bestTime);
    json += "\",\"count\":";
    json += String(_elapsedTimes.size());
    json += ",\"trigger\":{";
    json += "\"armed\":";
    json += _triggerArmed ? "true" : "false";
    json += ",\"label\":\"";
    json += _triggerArmed ? "ARMED" : "DISARMED";
    json += "\"},\"brightness\":";
    json += String(_displayBrightness);
    json += ",\"times\":";
    json += buildTimesJson();
    json += "}";

    _cachedDashboardJson = std::move(json);
    _dashboardJsonDirty = false;
    return _cachedDashboardJson;
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
    }

    if (html.length() == 0) {
        html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
               "<title>SPIFFS Error</title></head><body>"
               "<h1>Greska pri ucitavanju SPIFFS sadrzaja</h1>"
               "<p>Datoteka /index.html nije dostupna ili je prazna.</p>"
               "</body></html>";
        _server.send(200, "text/html", html);
        return;
    }

    html.replace("%LAST%", formatTime(_lastTime));
    html.replace("%BEST%", formatTime(_bestTime));
    html.replace("%TRIGGER_STATE%", _triggerArmed ? "ARMED" : "DISARMED");
    html.replace("%TRIGGER_BUTTON%", _triggerArmed ? "Disarm Trigger" : "Arm Trigger");
    html.replace("%TRIGGER_ARMED%", _triggerArmed ? "true" : "false");
    html.replace("%BRIGHTNESS_VALUE%", String(_displayBrightness));
    _server.send(200, "text/html", html);
}

void WebServerManager::handleTriggerState() {
    _server.send(200, "application/json", buildTriggerStateJson());
}

void WebServerManager::handleTriggerControl() {
    if (!_server.hasArg("armed")) {
        _server.send(400, "application/json", "{\"error\":\"Missing armed parameter\"}");
        return;
    }

    String value = _server.arg("armed");
    value.toLowerCase();
    bool armed = (value == "1" || value == "true" || value == "on");
    setTriggerArmed(armed);
    _server.send(200, "application/json", buildTriggerStateJson());
}

void WebServerManager::handleBrightnessState() {
    _server.send(200, "application/json", buildBrightnessStateJson());
}

void WebServerManager::handleRevisionState() {
    _server.send(200, "application/json", buildRevisionJson());
}

void WebServerManager::handleDashboardState() {
    _server.send(200, "application/json", buildDashboardJson());
}

void WebServerManager::handleBrightnessControl() {
    if (!_server.hasArg("value")) {
        _server.send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
        return;
    }

    int brightness = _server.arg("value").toInt();
    if (brightness < 0 || brightness > 15) {
        _server.send(400, "application/json", "{\"error\":\"Brightness must be between 0 and 15\"}");
        return;
    }

    setDisplayBrightness(static_cast<uint8_t>(brightness));
    _server.send(200, "application/json", buildBrightnessStateJson());
}


void WebServerManager::handleReset() {
    updateStats(0, 0);
    StopwatchDisplay::getInstance().showTime("00:00:00");
    Stopwatch::getInstance().reset();
    if (_trackingResetHandler) {
        _trackingResetHandler();
    }
    _server.send(200, "application/json", "{\"ok\":true}");
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
    
    invalidateTimesCache();
    invalidateStatsCache();
    invalidateDashboardCache();
    updateStats(0, 0);
    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebServerManager::invalidateTimesCache() {
    _timesJsonDirty = true;
    _dashboardJsonDirty = true;
}

void WebServerManager::invalidateStatsCache() {
    _statsJsonDirty = true;
    _dashboardJsonDirty = true;
}

void WebServerManager::invalidateDashboardCache() {
    _dashboardJsonDirty = true;
}

void WebServerManager::touchRevision() {
    ++_revision;
}
