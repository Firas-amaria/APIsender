#include "wifi_manager.h"
#include "config/project_config.h"
#include "config/settings.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <cstdio>
#include <cstring>
namespace wifi {
static portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
static Status current = {false, "Wi-Fi: Choose a network in Settings"};
static ScanResults results = {};
enum class Operation { Scan, Connect, Retry };
struct Request { Operation operation; settings::WifiCredentials credentials; };
static QueueHandle_t requests;
static bool working = false;
static void setStatus(bool connected, const char *text) {
    portENTER_CRITICAL(&mutex);
    current.connected = connected;
    snprintf(current.text, sizeof(current.text), "%s", text);
    portEXIT_CRITICAL(&mutex);
}
Status status() {
    portENTER_CRITICAL(&mutex);
    Status copy = current;
    portEXIT_CRITICAL(&mutex);
    return copy;
}
ScanResults scanResults() {
    portENTER_CRITICAL(&mutex);
    ScanResults copy = results;
    portEXIT_CRITICAL(&mutex);
    return copy;
}
static bool enqueue(const Request &request) {
    if (!requests) return false;
    portENTER_CRITICAL(&mutex);
    bool available = !working;
    if (available) working = true;
    portEXIT_CRITICAL(&mutex);
    if (!available) return false;
    if (xQueueSend(requests, &request, 0) == pdTRUE) return true;
    portENTER_CRITICAL(&mutex);
    working = false;
    portEXIT_CRITICAL(&mutex);
    return false;
}
void retry() { Request r = {}; r.operation = Operation::Retry; enqueue(r); }
bool scan() { Request r = {}; r.operation = Operation::Scan; return enqueue(r); }
bool connect(const char *ssid, const char *password) {
    if (!ssid || !password || !*ssid || strlen(ssid) > 32 || strlen(password) > 64) return false;
    Request r = {}; r.operation = Operation::Connect;
    snprintf(r.credentials.ssid, sizeof(r.credentials.ssid), "%s", ssid);
    snprintf(r.credentials.password, sizeof(r.credentials.password), "%s", password);
    return enqueue(r);
}
static void onEvent(void *, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        auto event = static_cast<wifi_event_sta_disconnected_t *>(data);
        const bool auth = event->reason == WIFI_REASON_AUTH_FAIL ||
                          event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
                          event->reason == WIFI_REASON_HANDSHAKE_TIMEOUT;
        setStatus(false, auth ? "Wi-Fi: Check password (retrying)" : "Wi-Fi: Disconnected (retrying)");
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto event = static_cast<ip_event_got_ip_t *>(data);
        char text[80];
        snprintf(text, sizeof(text), "Wi-Fi: Connected\nIP: " IPSTR, IP2STR(&event->ip_info.ip));
        setStatus(true, text);
    }
}
static bool apply(const settings::WifiCredentials &saved) {
    wifi_config_t credentials = {};
    memcpy(credentials.sta.ssid, saved.ssid, strlen(saved.ssid));
    memcpy(credentials.sta.password, saved.password, strlen(saved.password));
    return esp_wifi_set_config(WIFI_IF_STA, &credentials) == ESP_OK;
}
static void worker(void *) {
    settings::WifiCredentials saved = {};
    bool configured = settings::loadWifi(saved);
    if (!configured && strcmp(config::WIFI_SSID, "YOUR_WIFI_SSID") && *config::WIFI_SSID) {
        static_assert(sizeof(config::WIFI_SSID) <= 33, "SSID too long");
        static_assert(sizeof(config::WIFI_PASSWORD) <= 65, "Password too long");
        snprintf(saved.ssid, sizeof(saved.ssid), "%s", config::WIFI_SSID);
        snprintf(saved.password, sizeof(saved.password), "%s", config::WIFI_PASSWORD);
        configured = true;
    }
    configured = configured && apply(saved);
    if (configured) { setStatus(false, "Wi-Fi: Connecting..."); esp_wifi_connect(); }
    for (;;) {
        Request request = {};
        if (xQueueReceive(requests, &request, pdMS_TO_TICKS(config::WIFI_RETRY_MS)) == pdTRUE) {
            if (request.operation == Operation::Scan) {
                ScanResults next = {};
                // A connecting station cannot scan. Pause that attempt; connected stations can scan in place.
                if (!status().connected) esp_wifi_disconnect();
                wifi_scan_config_t options = {};
                if (esp_wifi_scan_start(&options, true) == ESP_OK) {
                    wifi_ap_record_t records[MAX_NETWORKS] = {};
                    uint16_t count = MAX_NETWORKS;
                    if (esp_wifi_scan_get_ap_records(&count, records) == ESP_OK) {
                        for (int i = 0; i < count; ++i) {
                            if (!records[i].ssid[0]) continue;
                            bool duplicate = false;
                            for (int j = 0; j < next.count; ++j)
                                if (!strcmp(next.networks[j].ssid, reinterpret_cast<char *>(records[i].ssid))) duplicate = true;
                            if (duplicate) continue;
                            auto &network = next.networks[next.count++];
                            snprintf(network.ssid, sizeof(network.ssid), "%s", records[i].ssid);
                            network.secured = records[i].authmode != WIFI_AUTH_OPEN;
                        }
                        snprintf(next.message, sizeof(next.message), "%s", next.count ? "Select a network below" : "No networks found. Scan again.");
                    } else snprintf(next.message, sizeof(next.message), "Could not read networks. Try again.");
                } else snprintf(next.message, sizeof(next.message), "Scan failed. Try again.");
                portENTER_CRITICAL(&mutex);
                next.revision = results.revision + 1;
                results = next;
                portEXIT_CRITICAL(&mutex);
            } else if (request.operation == Operation::Connect) {
                esp_wifi_disconnect();
                setStatus(false, "Wi-Fi: Connecting...");
                configured = apply(request.credentials);
                if (!configured) setStatus(false, "Wi-Fi: Invalid network settings");
                else if (!settings::saveWifi(request.credentials)) {
                    setStatus(false, "Wi-Fi: Could not save. Try Connect again.");
                    configured = false;
                }
            }
            // Password copies are never displayed or logged.
            memset(&request, 0, sizeof(request));
            portENTER_CRITICAL(&mutex);
            working = false;
            portEXIT_CRITICAL(&mutex);
        }
        if (configured && !status().connected) esp_wifi_connect();
    }
}
void start() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta() ? ESP_OK : ESP_FAIL);
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, onEvent, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, onEvent, nullptr));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    requests = xQueueCreate(1, sizeof(Request));
    ESP_ERROR_CHECK(requests ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(worker, "wifi_worker", 6144, nullptr, 2, nullptr) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
}
