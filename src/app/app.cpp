#include "app.h"
#include "config/settings.h"
#include "config/project_config.h"
#include "network/wifi_manager.h"
#include "display/display.h"
#include "ui/ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"
#include <cstdio>
#include <string>
namespace app {
struct Request { Action action; uint32_t view; char value[config::URL_CAPACITY]; char base[config::URL_CAPACITY]; };
static QueueHandle_t requests, completions;
static std::string savedUrl;
static bool pending = false;
const char *baseUrl() { return savedUrl.c_str(); }
bool busy() { return pending; }
bool submit(Action action, const char *value, uint32_t view) {
    if (pending) return false;
    Request request = {};
    request.action = action; request.view = view;
    snprintf(request.value, sizeof(request.value), "%s", value ? value : "");
    snprintf(request.base, sizeof(request.base), "%s", baseUrl());
    if (xQueueSend(requests, &request, 0) != pdTRUE) return false;
    pending = true;
    return true;
}
bool poll(Completion &completion) {
    if (xQueueReceive(completions, &completion, 0) != pdTRUE) return false;
    pending = false;
    if (completion.action == Action::Save && completion.result.success) savedUrl = completion.value;
    return true;
}
static void worker(void *) {
    Request request;
    for (;;) {
        xQueueReceive(requests, &request, portMAX_DELAY);
        Completion completion = {};
        completion.action = request.action; completion.view = request.view;
        snprintf(completion.value, sizeof(completion.value), "%s", request.value);
        switch (request.action) {
        case Action::Health: completion.result = api::checkHealth(request.base); break;
        case Action::Random: completion.result = api::getRandomNumber(request.base); break;
        case Action::Color: completion.result = api::sendColor(request.base, request.value); break;
        case Action::Event: completion.result = api::sendEvent(request.base, request.value); break;
        case Action::Save:
        case Action::Test: {
            std::string normalized;
            if (!settings::normalizeBaseUrl(request.value, normalized)) {
                snprintf(completion.result.message, sizeof(completion.result.message), "Invalid Base URL\nUse http://host:port");
            } else if (request.action == Action::Test) {
                completion.result = api::checkHealth(normalized.c_str());
            } else {
                completion.result.success = settings::saveBaseUrl(normalized);
                snprintf(completion.result.message, sizeof(completion.result.message), "%s",
                         completion.result.success ? "API Server Saved" : "Save Failed");
                snprintf(completion.value, sizeof(completion.value), "%s", normalized.c_str());
            }
            break;
        }
        }
        xQueueSend(completions, &completion, portMAX_DELAY);
    }
}
void start() {
    settings::initialize();
    savedUrl = settings::loadBaseUrl();
    requests = xQueueCreate(1, sizeof(Request));
    completions = xQueueCreate(1, sizeof(Completion));
    ESP_ERROR_CHECK(requests && completions ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(worker, "api_worker", 8192, nullptr, 3, nullptr) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    wifi::start();
    display::lock(); ui::start(); display::unlock();
}
}
