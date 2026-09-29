# ESP32 API Client Lesson Device

A ready-made touchscreen HTTP client for a programming lesson. Students build a
website/backend; the teacher uploads this firmware once. The device checks server
health, requests a random number, sends a color, and sends button events.

## Hardware and software

- JC3248W535EN: ESP32-S3, 3.5-inch 320 x 480 capacitive touch, 8 MB PSRAM, 16 MB flash.
- PlatformIO Espressif32 **6.6.0**, ESP-IDF **5.2.1**, vendored LVGL **8.4.0**.
- ESP-IDF Wi-Fi, HTTP client, cJSON and NVS; no Arduino framework.
- Display/touch source adapted from [NorthernMan54's reference](https://github.com/NorthernMan54/JC3248W535EN).
  See [hardware provenance](docs/HARDWARE.md) for pins, revision and changes.
- No external wiring. Use a USB data cable and a 2.4 GHz Wi-Fi network.

## Folder map

```text
platformio.ini             Build, upload, pinned platform
CMakeLists.txt              ESP-IDF project
partitions.csv             NVS and 4 MB application partition
sdkconfig.api-device       ESP-IDF / flash / PSRAM settings
boards/320x480.json         Reference board definition
components/lvgl/           Pinned upstream GUI library (normally leave alone)
src/
  main.cpp                 Startup only
  app/app.*                Request queue and application coordination
  display/display.*        Small display wrapper
  display/vendor/          Proven panel, touch and LVGL-port C drivers
  ui/ui.*                  Six screens, keyboard and touch events
  network/wifi_manager.*   Wi-Fi connection, status and retries
  network/api_client.*     Four API calls, HTTP and JSON handling
  config/project_config.h Wi-Fi, endpoint paths, limits, touch-test switch
  config/settings.*        NVS persistence
  config/base_url.cpp      HTTP Base URL validation
  config/lv_conf.h         Reference LVGL configuration
  CMakeLists.txt            Application component build
 docs/                     API assignment, project notes and hardware checks
 tests/                    Host-side request/validation regression checks
```

## Build and upload (teacher)

1. Install VS Code and the PlatformIO IDE extension. Open this folder.
2. Edit `src/config/project_config.h`: set `WIFI_SSID` and `WIFI_PASSWORD`.
   Do not publish real credentials. An empty password supports an open classroom network.
3. Use **PlatformIO: Build**, or run in a PlatformIO terminal:

   ```sh
   pio run
   pio run -t upload
   pio device monitor -b 115200
   ```

   If automatic port selection fails: `pio run -t upload --upload-port COM5`
   (replace `COM5` with your board's port). Close Serial Monitor before uploading.
4. For a display-only check, set `TOUCH_TEST_ONLY = true`, build and upload.
   Tap the test button, then restore `false` and upload the complete application.
5. If needed, hold BOOT while tapping RESET to enter download mode.

The first build needs internet access to download PlatformIO tools. The checked-in
SDK configuration is intentional: retain the octal PSRAM and flash settings.
A custom partition table allocates 4 MB to the application; unused flash is reserved.
No filesystem or OTA service is needed.

## Use the lesson device

1. Start the student's backend on a laptop on the same network as the ESP32.
   Listen on `0.0.0.0`, not just `127.0.0.1`. Allow the server port through the laptop firewall.
2. Open **Settings**. Tap the URL field, enter e.g. `http://192.168.1.100:3000`,
   and press the keyboard's checkmark to close it. **Test Connection** tests the
   text currently entered. **Save** writes it to NVS and makes it the active URL.
3. Return to the menu and try Health Check, Get Random Number, Send Color and Send Event.
4. The menu/settings show Wi-Fi status and the ESP32 IP. Wi-Fi retries every five
   seconds; Settings also has **Retry Wi-Fi**. Requests report short readable errors.
5. Restart the board: the saved API URL remains. Wi-Fi credentials come from firmware.

Use an HTTP origin: `http://hostname[:port]` or `http://IPv4[:port]`.
A trailing slash is removed. Paths, query strings, credentials, HTTPS, IPv6 and
`localhost` are intentionally unsupported. A bare IP needs the `http://` prefix.
The endpoint path is appended automatically; do not include `/api` in the Base URL.
The maximum Base URL is 191 characters. On the ESP32, localhost would mean the ESP32,
not the student's laptop. Numeric LAN addresses are simplest; `.local` discovery
is not configured. Guest Wi-Fi may prevent devices from reaching one another.

Read **[docs/API.md](docs/API.md)** as the student assignment specification.
Read **[docs/PROJECT_NOTES.md](docs/PROJECT_NOTES.md)** to understand the code.

## Useful settings and diagnostics

`project_config.h` contains the four paths, a 5000 ms HTTP socket timeout,
1024-byte response limit, 100 ms UI poll interval, Wi-Fi settings and default URL.
Only one request is allowed at a time; Back remains usable while waiting.
Serial logs show method, URL, JSON sent, HTTP status and response (not the Wi-Fi password).
A network request never runs inside an LVGL callback.

`Invalid Response` means a successful GET returned the wrong JSON shape/value.
`Request Failed (HTTP ...)` means the server replied with an unsuccessful status.
`Server Unreachable` usually means the IP/port, server binding or network is wrong.
`Request Timeout` means a network read/connect operation timed out. A whole request
can take longer than one socket timeout because it includes several network steps.

## Verification

Both the initial touch screen and integrated firmware were compiled with PlatformIO.
Host regression checks exercise the actual API client against a fake HTTP transport,
including JSON parsing, HTTP errors, payloads, response limits and URL validation.
A native LVGL preview also checks all six screens, navigation and keyboard events.
See `tests/README.md`. Actual screen rendering, touch alignment, Wi-Fi connectivity
and NVS persistence still require the physical board: follow `docs/TESTING.md`.
