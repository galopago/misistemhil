# Implementer Notes — `b06_hil`

## `B06_MAIN_METERING_OLED` (2026-11-03)

- Ported reference firmware from `b00_basehil/firmware` into this tree (copy,
  not linked).
- `ina219` returns `esp_err_t` (same as active `i2c_bus`); architecture doc’s
  `status_t` treated as logical synonym.
- I2C phase 2 only (`i2c_bus_transceive`); no `i2c_broker` in tree.
- Calibration assumes ~3.2 A FS with provisional `0.1f` ohm shunt
  (`TODO(b06-hil)` until BOM R5/R6/R8).
- Metering task owns the 500 ms period; only `app_core` calls display APIs.
- Host API: removed advertised base demos `echo` / `loopback` for this board;
  metering commands still deferred.
- Build: `idf.py build` succeeded → `build/b06_hil_firmware.bin` (partition
  nearly full warning).

## Metering OLED sign column (2026-11-03)

- Verified `format_channel_line` matches architecture fixed 14-char layout:
  `"%d %4.2fV %c%4.2fA"` with sign `' '` / `'-'`, `fabsf` magnitude, clamp
  `0.00`..`9.99`; fail line `"%d ----V  ----A"`.

## `B06_HOST_API_METERING` (2026-11-03)

- Sample ownership in `components/app_meter/` to avoid `host_api` ↔ `app_core`
  CMake cycle while keeping normative `app_core_meter_*` names.
- `power_w = voltage_v * current_a` on the same sample; I2C soft-fail →
  channel `ok: false` without outer error.
- Human rule: implementer generates code only; do not compile in-session
  unless explicitly requested.
