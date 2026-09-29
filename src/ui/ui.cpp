#include "ui.h"
#include "app/app.h"
#include "network/wifi_manager.h"
#include "config/project_config.h"
#include "lvgl.h"
#include <cstdio>
#include <cstdint>
namespace ui {
enum class Screen { Menu, Health, Random, Color, Event, Settings };
enum Command { Home, OpenHealth, OpenRandom, OpenColor, OpenEvent, OpenSettings,
               Health, Random, Red, Green, Blue, Yellow, A, B, C, Save, Test, Retry };
static Screen screen = Screen::Menu;
static uint32_t view = 0;
static lv_obj_t *resultLabel, *numberLabel, *wifiLabel, *serverLabel;
static lv_obj_t *urlInput, *keyboard, *settingsButtons;
static void show(Screen next);
static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int width) {
    auto object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_width(object, width);
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
    return object;
}
static void setResult(const char *text) {
    if (resultLabel) lv_label_set_text(resultLabel, text);
}
static void closeKeyboard() {
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(settingsButtons, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(urlInput, LV_STATE_FOCUSED);
}
static void send(app::Action action, const char *value = "") {
    if (app::submit(action, value, view)) {
        if (numberLabel) lv_label_set_text(numberLabel, "--");
        setResult(action == app::Action::Save ? "Saving..." : "Request in progress...");
    } else setResult("Please wait for current request");
}
static void onCommand(lv_event_t *event) {
    auto command = static_cast<Command>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    switch (command) {
    case Home: show(Screen::Menu); break;
    case OpenHealth: show(Screen::Health); break;
    case OpenRandom: show(Screen::Random); break;
    case OpenColor: show(Screen::Color); break;
    case OpenEvent: show(Screen::Event); break;
    case OpenSettings: show(Screen::Settings); break;
    case Health: send(app::Action::Health); break;
    case Random: send(app::Action::Random); break;
    case Red: send(app::Action::Color, "red"); break;
    case Green: send(app::Action::Color, "green"); break;
    case Blue: send(app::Action::Color, "blue"); break;
    case Yellow: send(app::Action::Color, "yellow"); break;
    case A: send(app::Action::Event, "A"); break;
    case B: send(app::Action::Event, "B"); break;
    case C: send(app::Action::Event, "C"); break;
    case Save: send(app::Action::Save, lv_textarea_get_text(urlInput)); break;
    case Test: send(app::Action::Test, lv_textarea_get_text(urlInput)); break;
    case Retry: wifi::retry(); setResult("Wi-Fi retry requested"); break;
    }
}
static lv_obj_t *button(lv_obj_t *parent, const char *text, int y, Command command,
                        uint32_t color = 0x2463A0) {
    auto object = lv_btn_create(parent);
    lv_obj_set_pos(object, 12, y);
    lv_obj_set_size(object, 296, 46);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_text_color(object, lv_color_white(), 0);
    auto caption = lv_label_create(object);
    lv_label_set_text(caption, text); lv_obj_center(caption);
    lv_obj_add_event_cb(object, onCommand, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(command)));
    return object;
}
static void updateConnection() {
    if (wifiLabel) lv_label_set_text(wifiLabel, wifi::status().text);
    if (serverLabel) lv_label_set_text_fmt(serverLabel, "API Server:\n%s", app::baseUrl());
}
static void buildSettings(lv_obj_t *root) {
    wifiLabel = label(root, "", 12, 42, 296);
    serverLabel = label(root, "", 12, 91, 296);
    lv_obj_set_style_text_font(serverLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_height(serverLabel, 42);
    lv_label_set_long_mode(serverLabel, LV_LABEL_LONG_DOT);
    urlInput = lv_textarea_create(root);
    lv_obj_set_pos(urlInput, 12, 142); lv_obj_set_size(urlInput, 296, 48);
    lv_textarea_set_one_line(urlInput, true);
    lv_textarea_set_max_length(urlInput, config::URL_CAPACITY - 1);
    lv_textarea_set_text(urlInput, app::baseUrl());
    settingsButtons = lv_obj_create(root);
    lv_obj_remove_style_all(settingsButtons);
    lv_obj_set_pos(settingsButtons, 0, 198); lv_obj_set_size(settingsButtons, 320, 282);
    lv_obj_clear_flag(settingsButtons, LV_OBJ_FLAG_SCROLLABLE);
    button(settingsButtons, "Save", 0, Save);
    button(settingsButtons, "Test Connection", 54, Test);
    button(settingsButtons, "Retry Wi-Fi", 108, Retry);
    resultLabel = label(settingsButtons, "Test uses the entered URL.\nSave keeps it after restart.", 12, 161, 296);
    button(settingsButtons, "Back", 228, Home);
    keyboard = lv_keyboard_create(root);
    lv_obj_set_size(keyboard, 320, 230); lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard, urlInput);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(urlInput, [](lv_event_t *) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(settingsButtons, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(keyboard, [](lv_event_t *event) {
        if (lv_event_get_code(event) == LV_EVENT_READY || lv_event_get_code(event) == LV_EVENT_CANCEL) closeKeyboard();
    }, LV_EVENT_ALL, nullptr);
}
static void show(Screen next) {
    screen = next; ++view;
    auto root = lv_scr_act();
    lv_obj_clean(root);
    resultLabel = numberLabel = wifiLabel = serverLabel = nullptr;
    urlInput = keyboard = settingsButtons = nullptr;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0xF2F5FA), 0);
    lv_obj_set_style_text_color(root, lv_color_hex(0x15243B), 0);
    lv_obj_set_style_text_font(root, &lv_font_montserrat_18, 0);
    static const char *titles[] = {"ESP32 API Demo", "Health Check", "Random Number", "Send Color", "Send Event", "Settings"};
    auto title = label(root, titles[static_cast<int>(screen)], 12, 10, 296);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    if (screen == Screen::Menu) {
        wifiLabel = label(root, "", 12, 48, 296);
        serverLabel = label(root, "", 12, 100, 296);
        lv_obj_set_style_text_font(serverLabel, &lv_font_montserrat_14, 0);
        lv_obj_set_height(serverLabel, 42);
        lv_label_set_long_mode(serverLabel, LV_LABEL_LONG_DOT);
        button(root, "Health Check", 166, OpenHealth);
        button(root, "Get Random Number", 222, OpenRandom);
        button(root, "Send Color", 278, OpenColor);
        button(root, "Send Event", 334, OpenEvent);
        button(root, "Settings", 390, OpenSettings);
    } else if (screen == Screen::Settings) {
        buildSettings(root);
    } else {
        resultLabel = label(root, app::busy() ? "A request is still in progress" : "Ready", 12, 346, 296);
        button(root, "Back", 426, Home);
        if (screen == Screen::Health) {
            label(root, "GET /api/health", 12, 66, 296);
            button(root, "Check Server", 140, Health);
        } else if (screen == Screen::Random) {
            label(root, "Returned value:", 12, 70, 296);
            numberLabel = label(root, "--", 12, 112, 296);
            lv_obj_set_style_text_font(numberLabel, &lv_font_montserrat_32, 0);
            button(root, "Get Random Number", 205, Random);
        } else if (screen == Screen::Color) {
            button(root, "RED", 66, Red, 0xBC293A);
            button(root, "GREEN", 128, Green, 0x24773C);
            button(root, "BLUE", 190, Blue, 0x245ABD);
            auto yellow = button(root, "YELLOW", 252, Yellow, 0xF3D84C);
            lv_obj_set_style_text_color(yellow, lv_color_hex(0x15243B), 0);
        } else {
            button(root, "Button A", 80, A);
            button(root, "Button B", 158, B);
            button(root, "Button C", 236, C);
        }
    }
    updateConnection();
}
static void tick(lv_timer_t *) {
    app::Completion completion;
    if (app::poll(completion) && completion.view == view) {
        // Never apply a result to a screen opened after that request started.
        char text[220];
        if (completion.action == app::Action::Color || completion.action == app::Action::Event) {
            snprintf(text, sizeof(text), "%s: %.6s\nStatus: %s", completion.action == app::Action::Color ? "Sent" : "Last Event",
                     completion.value, completion.result.message);
            setResult(text);
        } else setResult(completion.result.message);
        if (numberLabel && completion.action == app::Action::Random && completion.result.success)
            lv_label_set_text_fmt(numberLabel, "%d", completion.result.number);
    }
    updateConnection();
}
void start() { show(Screen::Menu); lv_timer_create(tick, config::UI_POLL_MS, nullptr); }
void touchTest() {
    auto root = lv_scr_act();
    label(root, "Display / Touch Test", 12, 20, 296);
    label(root, "Tap the button. Check all\nfour corners after flashing.", 12, 70, 296);
    auto object = lv_btn_create(root);
    lv_obj_set_size(object, 270, 100); lv_obj_center(object);
    auto text = lv_label_create(object); lv_label_set_text(text, "Touch to test"); lv_obj_center(text);
    lv_obj_add_event_cb(object, [](lv_event_t *event) {
        lv_label_set_text(static_cast<lv_obj_t *>(lv_event_get_user_data(event)), "Touch works!");
    }, LV_EVENT_CLICKED, text);
}
}

