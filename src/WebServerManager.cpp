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
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <meta http-equiv="refresh" content="5">
        <title>Stopwatch Stats</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 20px;
                padding: 0;
                background: #f4f4f4;
                color: #333;
            }
            h1, h2 {
                text-align: center;
            }
            table {
                width: 100%;
                max-width: 600px;
                margin: 20px auto;
                border-collapse: collapse;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                background: #fff;
            }
            th, td {
                padding: 12px;
                border: 1px solid #ddd;
                text-align: center;
            }
            th {
                background-color: #f0f0f0;
            }
            .btn {
                display: block;
                width: 200px;
                margin: 10px auto;
                padding: 10px;
                text-align: center;
                background-color: #007bff;
                color: white;
                border: none;
                border-radius: 4px;
                text-decoration: none;
                font-size: 16px;
                cursor: pointer;
            }
            .btn:hover {
                opacity: 0.9;
            }
            .btn-reset { background-color: #28a745; }
            .btn-clear { background-color: #dc3545; }
            @media (max-width: 600px) {
                table, th, td {
                    font-size: 14px;
                }
                .btn {
                    width: 90%;
                    font-size: 14px;
                }
            }
        </style>
    </head>
    <body>
        <h1>Stopwatch Statistics</h1>
        <p style='text-align:center;'>Last Time: %LAST%</p>
        <p style='text-align:center;'>Best Time: %BEST%</p>

        <h2>All Elapsed Times</h2>
        <table>
            <tr><th>#</th><th>Time</th></tr>
    )rawliteral";

    // Dinamički sadržaj: sortiranje i popunjavanje tablice
    std::vector<unsigned long> sorted = _elapsedTimes;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 0; i < sorted.size(); ++i) {
        html += "<tr><td>" + String(i + 1) + "</td><td>" + formatTime(sorted[i]) + "</td></tr>";
    }

    // Dodavanje gumba
    html += R"rawliteral(
        </table>
        <form action='/wifi' method='get'><button class='btn'>WiFi Setup</button></form>
        <form action='/reset' method='post'><button class='btn btn-reset'>Reset Timer</button></form>
        <form action='/clear' method='post' onsubmit='return confirm("Clear all times?");'>
            <button class='btn btn-clear'>Clear All Times</button>
        </form>
    </body>
    </html>
    )rawliteral";

    // Zamjena placeholdera s pravim vrijednostima
    html.replace("%LAST%", formatTime(_lastTime));
    html.replace("%BEST%", formatTime(_bestTime));

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
