#include "api_client.h"
#include "wifi_manager.h"
#include "config/project_config.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>
#include <cerrno>
namespace api {
static Result message(bool success, const char *text) {
    Result result;
    result.success = success;
    snprintf(result.message, sizeof(result.message), "%s", text);
    return result;
}
struct Response { std::string body; bool tooLarge = false; };
static esp_err_t receive(esp_http_client_event_t *event) {
    auto response = static_cast<Response *>(event->user_data);
    if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
        if (response->body.size() + event->data_len > config::RESPONSE_CAPACITY) {
            response->tooLarge = true;
            return ESP_FAIL;
        }
        response->body.append(static_cast<const char *>(event->data), event->data_len);
    }
    return ESP_OK;
}
static Result request(const char *base, const char *path, const char *body, std::string &responseBody) {
    if (!wifi::status().connected) return message(false, "Wi-Fi Disconnected");
    std::string url = std::string(base) + path;
    Response response;
    esp_http_client_config_t options = {};
    options.url = url.c_str();
    options.timeout_ms = config::HTTP_TIMEOUT_MS;
    options.event_handler = receive;
    options.user_data = &response;
    options.disable_auto_redirect = true;
    auto client = esp_http_client_init(&options);
    if (!client) return message(false, "Request Failed");
    esp_http_client_set_header(client, "Accept", "application/json");
    if (body) {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, body, strlen(body));
    }
    ESP_LOGI("api", "%s %s", body ? "POST" : "GET", url.c_str());
    if (body) ESP_LOGI("api", "Body: %s", body);
    esp_err_t error = esp_http_client_perform(client);
    int socketError = esp_http_client_get_errno(client);
    int status = esp_http_client_get_status_code(client);
    ESP_LOGI("api", "HTTP Status: %d\nResponse: %s", status, response.body.c_str());
    esp_http_client_cleanup(client);
    if (response.tooLarge) return message(false, "Response Too Large");
    if (error != ESP_OK) {
        ESP_LOGW("api", "Request error: %s (socket %d)", esp_err_to_name(error), socketError);
        if (error == ESP_ERR_TIMEOUT || error == ESP_ERR_HTTP_EAGAIN || socketError == ETIMEDOUT || socketError == EAGAIN)
            return message(false, "Request Timeout");
        return message(false, "Server Unreachable");
    }
    // GET lessons use exactly 200; POST accepts any successful 2xx status.
    if ((body && (status < 200 || status >= 300)) || (!body && status != 200)) {
        char text[64]; snprintf(text, sizeof(text), "Request Failed (HTTP %d)", status);
        return message(false, text);
    }
    responseBody = response.body;
    return message(true, "Success");
}
static Result getValue(const char *base, bool health) {
    std::string body;
    Result result = request(base, health ? config::HEALTH_PATH : config::RANDOM_PATH, nullptr, body);
    if (!result.success) return result;
    auto json = cJSON_ParseWithLengthOpts(body.c_str(), body.size() + 1, nullptr, true);
    auto value = cJSON_GetObjectItemCaseSensitive(json, health ? "status" : "number");
    bool valid = cJSON_IsObject(json);
    if (health) {
        valid = valid && cJSON_IsString(value) && strcmp(value->valuestring, "ok") == 0;
        result = message(valid, valid ? "Server Online" : "Invalid Response");
    } else {
        valid = valid && cJSON_IsNumber(value) && std::isfinite(value->valuedouble)
            && value->valuedouble >= INT_MIN && value->valuedouble <= INT_MAX
            && std::floor(value->valuedouble) == value->valuedouble;
        result = message(valid, valid ? "Number Received" : "Invalid Response");
        if (valid) result.number = static_cast<int>(value->valuedouble);
    }
    cJSON_Delete(json);
    return result;
}
Result checkHealth(const char *base) { return getValue(base, true); }
Result getRandomNumber(const char *base) { return getValue(base, false); }
Result sendColor(const char *base, const char *color) {
    if (strcmp(color,"red") && strcmp(color,"green") && strcmp(color,"blue") && strcmp(color,"yellow"))
        return message(false, "Invalid Color");
    char body[40]; snprintf(body, sizeof(body), "{\"color\":\"%s\"}", color);
    std::string response;
    return request(base, config::COLOR_PATH, body, response);
}
Result sendEvent(const char *base, const char *button) {
    if (strcmp(button,"A") && strcmp(button,"B") && strcmp(button,"C")) return message(false, "Invalid Button");
    char body[32]; snprintf(body, sizeof(body), "{\"button\":\"%s\"}", button);
    std::string response;
    return request(base, config::EVENT_PATH, body, response);
}
}
