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

## Open / for tester

- Power-cycle retest: no short relay chatter from OE bring-up.
- Confirm OLED address and flash size on HW (architect open questions).
