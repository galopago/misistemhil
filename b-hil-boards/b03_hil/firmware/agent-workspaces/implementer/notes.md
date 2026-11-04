# Implementer notes — `b03_hil`

Handoff: `B03_PRODUCT_BRINGUP` / `B03_RELAY_CONTROL` / `B03_HOST_API_RELAY`
Date: 2026-11-03

## Done

- Adapt-copied allowed paths from `b00_basehil/firmware` into this tree (no
  submodule / no live base path dependency).
- Project renamed to `b03_hil_firmware`; identity `03` / `b03_hil`.
- Extended `components/board` with OE + six CTR macros and `b03_hil_board_pins_t`.
- Added `components/relay_ctrl/` with normative API and OE init sequence 1–7.
- `app_core_start` calls `relay_ctrl_init()` before I2C/display/WiFi; added
  `app_core_relay_*` facades (1:1 → `relay_ctrl_*`).
- Host API: `relay_set` / `relay_set_all` / `relay_get` / `relay_get_all`;
  capabilities summary exact; `streams: []`; removed `echo`; stream open rejects
  all stream_ids (no loopback).
- `info.streams_supported` = false; board strings `b03_hil` / `03`.

## Design choice (recorded)

Architecture preferred Option A (`host_api` → `app_core_relay_*`). That would
create a CMake cycle (`app_core` already `REQUIRES host_api`). Implemented
Option B for HTTP handlers: `host_api` → `relay_ctrl_*` directly. Facades still
exist on `app_core` for local callers; all sets go through `relay_ctrl` mutex.

## Commands run

- File adapt-copy via `rsync`/`cp` from base firmware (no `idf.py` / no build —
  human instructed implementer not to compile).

## 2026-11-03 — `B03_RELAY_OE_LEVEL_BEFORE_DIR`

- Patched `relay_ctrl_init`: `gpio_set_level` (inactive) **before**
  `gpio_config(OUTPUT)` for OE and all CTR; re-assert CTR lows; then OE active.
- Root cause addressed: prior `gpio_config(OE, OUTPUT)` first drove default
  OUT=0 → brief `/KOE` active while CTR unsafe.
- File touched: `components/relay_ctrl/relay_ctrl.c` only.

## 2026-11-03 — `B03_MAIN_RELAY_OLED`

- `app_core_relay_oled.c`: format/show FULL_FOUR_LINES; STA active flag.
- `app_core_wifi.c`: `show_wifi_connected_display` → relay OLED; leave active
  on SoftAP/connecting/failure.
- `relay_ctrl`: view modes + `relay_ctrl_view_token`; static set notifies
  listener; `relay_ctrl_drive*` for sched edges (no view/OLED spam).
- `relay_sched`: all former `relay_ctrl_set` calls → `relay_ctrl_drive`.

## 2026-11-03 — `B03_RELAY_OLED_VIEW_MODE`

- Added `relay_ctrl_set_view_mode` (store + callback; no GPIO).
- `relay_sched`: arm `P` on pulse/hold; `T` on pulse_train; `E` on both
  encoder_step channels; clear to `STATIC_OFF` on timer end / cancel / abort.
- `drive*` edges unchanged (no token flicker).

## Open / for tester

- Confirm OLED `P`/`T`/`E` during sched programs, then `0` on end.
- Static `0`/`1` live refresh still OK.
