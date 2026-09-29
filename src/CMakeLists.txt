file(GLOB_RECURSE app_sources "*.cpp" "*.c")
idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "." "display/vendor" REQUIRES lvgl driver esp_lcd esp_timer esp_wifi esp_event esp_netif nvs_flash esp_http_client json)

