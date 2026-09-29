#include "display/display.h"
#include "app/app.h"
#include "ui/ui.h"
#include "config/project_config.h"
extern "C" void app_main() {
    display::start();
    if (config::TOUCH_TEST_ONLY) {
        display::lock(); ui::touchTest(); display::unlock();
    } else {
        app::start();
    }
}
