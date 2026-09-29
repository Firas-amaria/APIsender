#pragma once
#include <cstddef>
namespace config {
// Teacher configures Wi-Fi before uploading. Students only build the server.
constexpr char WIFI_SSID[] = "YOUR_WIFI_SSID";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char DEFAULT_BASE_URL[] = "http://192.168.1.100:3000";
constexpr char HEALTH_PATH[] = "/api/health";
constexpr char RANDOM_PATH[] = "/api/random";
constexpr char COLOR_PATH[] = "/api/color";
constexpr char EVENT_PATH[] = "/api/event";
constexpr size_t URL_CAPACITY = 192;
constexpr size_t RESPONSE_CAPACITY = 1024;
constexpr int HTTP_TIMEOUT_MS = 5000;
constexpr int WIFI_RETRY_MS = 5000;
constexpr int UI_POLL_MS = 100;
// Set to true for a minimal display/touch test without Wi-Fi.
constexpr bool TOUCH_TEST_ONLY = false;
}
