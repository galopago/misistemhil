# Tests

Host-style geometry checks for the OLED display contract are implemented as
`display_geometry_self_test()` inside the `display` component and executed at
startup from `app_core_start()`.

I2C bus phase 1 validation is documented in `docs/test_strategy.md` and
`docs/i2c_bus_architecture.md`.

## Commands

- Build firmware: `idf.py set-target esp32c3 && idf.py build`
- Geometry self-test runs automatically on boot (serial log: `display_geom`).
- I2C bus init and OLED probe logs: `i2c_bus`, `app_core` tags on serial.

## Phase 2 I2C expectations

- All device-driver I2C traffic uses `i2c_bus_transceive()` (display_driver migrated).
- With OLED connected: log `SSD1306 initialized at I2C address 0x3C` (or 0x3D).
- Demo screen `FULL_FOUR_LINES` should appear on physical OLED after boot.
- Without OLED: stub mode; firmware must not reboot.
- Tester should run concurrent two-task transaction test per `docs/test_strategy.md`.
