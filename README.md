# MAX7219 Stopwatch (ESP32)

ESP32 stopwatch for laser pass timing with MAX7219 LED matrix output and a web UI served from SPIFFS.

- SD card support is removed (hardware is obsolete).
- Results are persisted only to SPIFFS (`/times.dat`).
- WiFi credentials are stored in NVS (`Preferences`, namespace `wifi`).
- Web app assets are served from `data/` via `uploadfs`.
- Captive-portal probe routes are redirected to `/` for better phone/laptop AP behavior.
- If `/index.html` or `/style.css` is missing/empty in SPIFFS, firmware serves a built-in fallback page/style.

## Features

- Laser-triggered stopwatch cycle (start/stop/reset flow).
- MAX7219 time display in `m:ss:mmm` format.
- Web dashboard with:
  - last and best time
  - full list of recorded times
  - reset and clear actions
  - WiFi setup page
- Persistent time history in SPIFFS.

## Project Structure

- `src/main.cpp`
  - Initialization, WiFi mode selection, sensor loop, stopwatch state transitions.
- `src/Stopwatch.cpp`
  - Stopwatch logic.
- `src/StopwatchDisplay.cpp`
  - MAX7219 rendering.
- `src/WebServerManager.cpp`
  - HTTP routes, API, SPIFFS time persistence, fallback web serving.
- `include/configuration.h`
  - Hardware pin configuration.
- `data/`
  - Web files: `index.html`, `style.css`, `wifi.html`.

## Hardware Configuration

Pins and display setup are defined in `include/configuration.h`.

Current values:

- `HARDWARE_TYPE`: `MD_MAX72XX::FC16_HW`
- `MAX_DEVICES`: `5`
- `DATA_PIN`: `14`
- `CLK_PIN`: `26`
- `CS_PIN`: `27`
- `TOTAL_COLUMNS`: `40`
- `LASER_SENSOR_PIN`: `13`

## Build And Flash

Run from project root:

1. Build firmware

   `C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run`

2. Upload firmware

   `C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run --target upload --upload-port COM12`

3. Upload web assets to SPIFFS

   `C:\Users\ivek8\.platformio\penv\Scripts\platformio.exe run --target uploadfs --upload-port COM12`

4. Open serial monitor (115200) and confirm boot logs.

If upload reports that COM port is busy, close Serial Monitor and retry.

## WiFi Behavior

- If saved SSID/password exist in NVS, device tries STA connection.
- If STA connection fails, device starts AP mode.
- If no saved credentials exist, AP mode starts immediately.

AP defaults:

- SSID: `StopwatchAP`
- Password: `12345678`

## HTTP Endpoints

- `GET /`, `GET /index.html`
  - Main dashboard.
- `GET /wifi`
  - WiFi credentials form.
- `POST /savewifi`
  - Save WiFi credentials to NVS and reboot.
- `POST /reset`
  - Reset stopwatch state.
- `POST /clear`
  - Clear all stored times (`/times.dat`).
- `GET /api/times`
  - JSON array of all recorded times.
- `GET /api/stats`
  - JSON summary (`last`, `best`, `count`).

Captive portal compatibility routes redirected to `/`:

- `GET /generate_204`
- `GET /hotspot-detect.html`
- `GET /ncsi.txt`
- `GET /connecttest.txt`
- `GET /fwlink`

## Notes

- Main web UI is loaded from SPIFFS. After changing files in `data/`, always run `uploadfs`.
- Time data file format is binary and stored in `/times.dat`.
