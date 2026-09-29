#pragma once
#include <cstddef>
using esp_err_t = int;
constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_TIMEOUT=1, ESP_ERR_HTTP_EAGAIN=2;
constexpr int HTTP_EVENT_ON_DATA=1, HTTP_METHOD_POST=1;
struct esp_http_client_event_t { int event_id; void *user_data; void *data; int data_len; };
struct esp_http_client_config_t {
 const char *url; int timeout_ms;
 esp_err_t (*event_handler)(esp_http_client_event_t *);
 void *user_data; bool disable_auto_redirect;
};
using esp_http_client_handle_t = void *;
const char *esp_err_to_name(esp_err_t);
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *);
int esp_http_client_set_header(void *,const char *,const char *);
int esp_http_client_set_method(void *,int);
int esp_http_client_set_post_field(void *,const char *,int);
int esp_http_client_perform(void *);
int esp_http_client_get_errno(void *);
int esp_http_client_get_status_code(void *);
int esp_http_client_cleanup(void *);
