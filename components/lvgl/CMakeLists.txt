# Build the pinned upstream LVGL source with this board's lv_conf.h.
file(GLOB_RECURSE lvgl_sources "src/*.c")
idf_component_register(SRCS ${lvgl_sources}
    INCLUDE_DIRS "." "src" "${CMAKE_SOURCE_DIR}/src/config"
    REQUIRES esp_timer)
