# MAX7219 Stopwatch (ESP32)

This project is a stopwatch for measuring passes using a laser sensor, with output on a MAX7219 LED matrix and a web interface for viewing results.

## Main Features

- Time measurement using start/stop/reset logic via laser sensor
- Time display on MAX7219 in m:ss:mmm format
- Upside-down display flip support (currently enabled by default)
- Web interface for result and statistic overview
- Result storage in SPIFFS (times.dat)
- WiFi STA mode with fallback AP mode

## Project Structure

- src/main.cpp
  - Main application loop, sensor processing, and state transitions
- src/Stopwatch.cpp
  - Stopwatch logic (start, stop, reset, elapsed)
- src/StopwatchDisplay.cpp
  - Time rendering on MAX7219, including flip
- src/WebServerManager.cpp
  - HTTP routes, API, and time save/load logic
- data/
  - Web files (index.html, style.css, wifi.html)
- test/
  - Tests and test documentation

## Hardware Configuration

Pin configuration is defined in include/configuration.h.

Current values:
- HARDWARE_TYPE: MD_MAX72XX::FC16_HW
- MAX_DEVICES: 5
- DATA_PIN: 26
- CLK_PIN: 14
- CS_PIN: 27
- TOTAL_COLUMNS: 40
- LASER_SENSOR_PIN: 13

## How To Run

1. Build firmware:
   C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run

2. Upload firmware:
   C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run --target upload

3. Upload web datoteka u SPIFFS:
   C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run --target uploadfs

4. Open serial monitor (115200) and verify logs.

## WiFi Behavior

- If SSID and password are saved (NVS Preferences), the device attempts to connect.
- If connection fails, it starts AP mode:
  - SSID: StopwatchAP
  - Password: 12345678

## Web Endpoints

- / i /index.html
  - Main page with result table
- /wifi
  - WiFi credentials form
- /savewifi (POST)
  - Save WiFi credentials to NVS and restart
- /clear (POST)
  - Clear saved results
- /reset (POST)
  - Reset stopwatch
- /api/times
  - JSON list of all recorded times
- /api/stats
  - JSON with last/best/count values

## Stability Notes

Display and loop stability were recently improved:
- safer display update-cycle handling
- less duplicated render logic
- web server remains responsive during sensor ignore period
- reduced serial output spam (throttled)

## Tests

See test/README for testing details and recommended next test scenarios.
