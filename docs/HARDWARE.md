# Hardware provenance

Hardware source: https://github.com/NorthernMan54/JC3248W535EN
Commit: `94ffc13bd011e897e82e4097afd272d6932f3cab`.

`src/display/vendor/` preserves the reference C drivers and their notices.
`boards/320x480.json`, `src/config/lv_conf.h`, and the initial SDK configuration
come from that revision. LVGL 8.4.0 source is vendored in `components/lvgl/`;
examples and demo assets are omitted. A minimal component CMake file builds the
library using `src/config/lv_conf.h`; `LV_KCONFIG_IGNORE` prevents Kconfig defaults
from silently replacing the reference colors, fonts and allocator. The demos are disabled.
The reference README says LVGL 8.3 / IDF 5.3, but its actual library metadata
says 8.4.0 and its PlatformIO pin (6.6.0) selects ESP-IDF 5.2.1. We follow the
actual pinned build configuration and check it by compiling.

| Interface | Reference configuration |
| --- | --- |
| LCD | AXS15231B, 320 x 480, RGB565, QSPI / SPI2 |
| QSPI CS / clock | GPIO 45 / 47 |
| QSPI D0 / D1 / D2 / D3 | GPIO 21 / 48 / 40 / 39 |
| Backlight / tear signal | GPIO 1 / 38 |
| Touch I2C SDA / SCL | GPIO 4 / 8, 400 kHz |
| Touch reset / interrupt | Unconnected (-1), polled |
| Memory | 16 MB flash, octal PSRAM at 80 MHz |

The reference demo selects 90-degree landscape rotation. This application
selects the reference driver's supported `LV_DISP_ROT_NONE` portrait mode.
Both display and touch use the same rotation configuration. No panel command
sequence or GPIO assignments were changed. The application partition is enlarged
to 4 MB (within the existing 16 MB flash) for the GUI and network code. All wiring is internal to the board;
only USB power/data is required.

Hardware validation is still required: flash the board, check colors and touch
at all corners, and verify PSRAM startup messages. A compiler cannot verify wiring.
