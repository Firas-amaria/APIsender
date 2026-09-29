#include "display.h"
#include "vendor/esp_bsp.h"
#include "vendor/display.h"
#include "esp_err.h"
namespace display {
void start() {
    bsp_display_cfg_t config = {};
    config.lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    config.buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES;
    config.rotate = LV_DISP_ROT_NONE;
    ESP_ERROR_CHECK(bsp_display_start_with_config(&config) ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(bsp_display_backlight_on());
}
void lock() { bsp_display_lock(0); }
void unlock() { bsp_display_unlock(); }
}
