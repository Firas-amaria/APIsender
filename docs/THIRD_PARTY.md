# Third-party notices

- Board definition, BSP/display/touch integration and LVGL configuration: adapted
  from NorthernMan54/JC3248W535EN, commit
  `94ffc13bd011e897e82e4097afd272d6932f3cab`.
  Repository MIT license: `src/display/vendor/LICENSE`.
- Individual Espressif driver files retain their Apache-2.0 copyright/SPDX
  notices. License text: `src/display/vendor/LICENSE-APACHE-2.0`.
- LVGL 8.4.0 source from that reference: MIT,
  `components/lvgl/LICENCE.txt`. Its ESP-IDF build wrapper is replaced by a small
  component CMake file; library source is unchanged.
- ESP-IDF and PlatformIO packages are downloaded by PlatformIO and carry their
  own licenses. They are not copied into this repository.

See `HARDWARE.md` for configuration changes and source provenance.
