# How the project works

## Startup

`main.cpp` starts the display. The reference BSP starts its own LVGL task and touch
polling. Normally `app::start()` then initializes NVS, loads the saved server URL,
creates a request queue and result queue, starts the HTTP worker and Wi-Fi, and
creates the menu while holding the display's LVGL mutex.

The optional `TOUCH_TEST_ONLY` switch stops after creating a small test screen.

## Follow one button press

1. `ui.cpp` receives a touch and calls `app::submit()` with an action and value.
2. `app.cpp` copies the action, current Base URL and value into a queue. No pointers
   into LVGL text fields are passed to another task.
3. The worker calls the named API function in `api_client.cpp`.
4. The API module adds a fixed endpoint, performs HTTP, and validates GET JSON.
5. The worker puts a small result into the completion queue.
6. An LVGL timer polls that queue every 100 ms and updates the labels.

The HTTP worker never touches LVGL. UI callbacks and the timer run under the port's
LVGL lock. Wi-Fi event callbacks protect their status snapshot with a short critical
section. Only the UI task accesses `pending` and the active Base URL after startup.
This makes the division of responsibilities simple and avoids concurrent UI access.

Only one request may be outstanding. Further action presses show a wait message.
Back and navigation remain available. Every screen gets a counter; an old result
cannot accidentally replace the labels on a newly opened screen. Successful Save
still changes the active URL even if the user already navigated away.

## Settings and Wi-Fi

`settings.cpp` reads/writes the API URL (`lesson/base_url`) and a single Wi-Fi
credentials blob (`lesson/wifi`) so SSID and password are stored together. Missing API settings use
the default URL. `base_url.cpp` validates an HTTP host/port and removes trailing
slashes. Save changes the active URL only after NVS commit succeeds. Test validates
and tests the entered text without saving it. Editing or closing the keyboard alone
does not change the active URL. An NVS initialization fault is logged and stops
startup rather than silently erasing stored settings.

Settings opens separate API Settings and Wi-Fi Settings pages. Wi-Fi uses saved
credentials, falling back to optional compiled defaults on first boot. The Wi-Fi
driver starts even without credentials, allowing touchscreen scans. A dedicated
worker serializes scans, credential saves and connection attempts; it retries every
five seconds. A scan pauses an unfinished connection attempt and resumes it afterward.
Scan snapshots and connection status are copied under a critical section; the worker
never touches LVGL. Connect & Save stores the entered credentials before attempting
connection, so a wrong password can be corrected by selecting the network again.
Passwords are masked in the UI and never logged. An HTTP failure never triggers
an automatic POST retry. Network work may block its own worker, not touch.

## Where to edit

| File | Typical edit |
| --- | --- |
| `src/config/project_config.h` | Optional Wi-Fi defaults, default URL, endpoint constants, limits |
| `src/ui/ui.cpp` | Screen wording, layout and buttons |
| `src/network/api_client.cpp` | API contract or parsing rules |
| `src/app/app.cpp` | How actions are coordinated |
| `docs/API.md` | Student-facing assignment specification |

Students doing the server lesson normally do not edit firmware at all.
Leave `src/display/vendor/`, the board JSON, `components/lvgl/`, `lv_conf.h`, and
flash/PSRAM settings alone unless deliberately working on hardware support.
The retained third-party driver code is larger than the application; it exists to
avoid asking students to debug a display controller.

## Build decisions

CMake registers ordinary ESP-IDF components. LVGL is vendored at the same source
revision as the reference so an upstream update cannot silently change its API.
`LV_KCONFIG_IGNORE` makes `lv_conf.h` authoritative rather than LVGL Kconfig defaults.
`CJSON_NESTING_LIMIT=16` bounds JSON parser recursion for malformed deeply nested
responses on the worker's small stack. The custom partition table provides a 4 MB application slot for the GUI and Wi-Fi;
NVS and PHY offsets match the usual reference layout. No unused sensor, actuator,
filesystem, cloud or database module is present.
