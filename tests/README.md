# Host regression checks

Run `powershell -ExecutionPolicy Bypass -File tests/run.ps1` on Windows with
Visual Studio C++ Build Tools and the PlatformIO ESP-IDF package installed.
The script compiles the **production** `api_client.cpp`, `base_url.cpp` and
ESP-IDF's real cJSON parser. Small transport stubs provide controlled HTTP replies.
No ESP32 or network is used; output files stay in ignored `tests/build/`.

Checks cover methods, paths, JSON bodies for all colors/buttons, 2xx POST semantics,
strict health/random JSON including integer boundaries, HTTP errors, timeouts,
disconnection, allocation failure, chunked/oversized response accumulation and
Base URL normalization/rejection. These do not test the real Wi-Fi/HTTP transport,
NVS hardware, LVGL rendering or touch. Use `docs/TESTING.md` for device acceptance.

Run `powershell -ExecutionPolicy Bypass -File tests/run_ui.ps1` to render the real
LVGL/UI source to PPM images in `tests/build/`. This checks navigation between all
screens, opening/closing both keyboards, selecting scan results, rejecting short
passwords, and submitting secured/open network credentials using LVGL events. It uses fake app
and Wi-Fi state, so it does not replace board acceptance testing.
