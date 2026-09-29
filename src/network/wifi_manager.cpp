#include "wifi_manager.h"
#include "config/project_config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>
namespace wifi {
static portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
static Status current = {false, "Wi-Fi: Disconnected"};
static bool configured = false;
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
void retry() {
    if (!configured || status().connected) return;
    setStatus(false, "Wi-Fi: Connecting");
    esp_err_t error = esp_wifi_connect();
    if (error != ESP_OK) ESP_LOGW("wifi", "Connect: %s", esp_err_to_name(error));
}
static void onEvent(void *, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) retry();
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        setStatus(false, "Wi-Fi: Disconnected (retrying)");
        ESP_LOGI("wifi", "Wi-Fi disconnected; retry in 5 seconds");
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto event = static_cast<ip_event_got_ip_t *>(data);
        char text[80];
        snprintf(text, sizeof(text), "Wi-Fi: Connected\nIP: " IPSTR, IP2STR(&event->ip_info.ip));
        setStatus(true, text);
        ESP_LOGI("wifi", "%s", text);
    }
}
static void reconnectTask(void *) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(config::WIFI_RETRY_MS));
        retry();
    }
}
void start() {
    configured = strcmp(config::WIFI_SSID, "YOUR_WIFI_SSID") != 0 && strlen(config::WIFI_SSID) > 0;
    if (!configured) { setStatus(false, "Wi-Fi: Disconnected\nConfigure Wi-Fi first"); return; }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta() ? ESP_OK : ESP_FAIL);
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, onEvent, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, onEvent, nullptr));
    wifi_config_t credentials = {};
    static_assert(sizeof(config::WIFI_SSID) <= 33, "SSID too long");
    static_assert(sizeof(config::WIFI_PASSWORD) <= 65, "Password too long");
    memcpy(credentials.sta.ssid, config::WIFI_SSID, strlen(config::WIFI_SSID));
    memcpy(credentials.sta.password, config::WIFI_PASSWORD, strlen(config::WIFI_PASSWORD));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &credentials));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(xTaskCreate(reconnectTask, "wifi_retry", 3072, nullptr, 2, nullptr) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
}
