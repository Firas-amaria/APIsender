#include "settings.h"
#include "project_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
namespace settings {
void initialize() {
    // Do not silently erase saved settings if NVS is damaged or incompatible.
    ESP_ERROR_CHECK(nvs_flash_init());
}
std::string loadBaseUrl() {
    nvs_handle_t handle;
    if (nvs_open("lesson", NVS_READONLY, &handle) != ESP_OK)
        return config::DEFAULT_BASE_URL;
    char value[config::URL_CAPACITY] = {};
    size_t length = sizeof(value);
    esp_err_t error = nvs_get_str(handle, "base_url", value, &length);
    nvs_close(handle);
    std::string normalized;
    if (error == ESP_OK && normalizeBaseUrl(value, normalized)) return normalized;
    return config::DEFAULT_BASE_URL;
}
bool saveBaseUrl(const std::string &url) {
    nvs_handle_t handle;
    esp_err_t error = nvs_open("lesson", NVS_READWRITE, &handle);
    if (error != ESP_OK) return false;
    error = nvs_set_str(handle, "base_url", url.c_str());
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    if (error != ESP_OK) ESP_LOGE("settings", "Save failed: %s", esp_err_to_name(error));
    return error == ESP_OK;
}
}
