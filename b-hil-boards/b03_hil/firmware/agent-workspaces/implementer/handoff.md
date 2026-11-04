# Implementer handoff — `b03_hil`

Status: **`B03_RELAY_OLED_VIEW_MODE` code complete** (2026-11-03). Awaiting
tester (pulse → `P`, train → `T`, encoder → `E`, then clear to `0`).

## Delivered

### `relay_ctrl`

- `relay_ctrl_set_view_mode(channel, mode)` — mode only; notify on change; no
  GPIO.
- Header comments: `P`/`T`/`E` are product tokens (not “reserved”).
- `drive*` remains view-inert.

### `relay_sched`

| Action | While active | On end / cancel / preempt |
| --- | --- | --- |
| pulse / hold / hold_many | `RELAY_VIEW_PULSE` | `STATIC_OFF` |
| `encoder_pulse_train` | `RELAY_VIEW_TRAIN` | `STATIC_OFF` |
| `encoder_step` A+B | `RELAY_VIEW_ENCODER` | both `STATIC_OFF` |

Edges still use `relay_ctrl_drive*` only. No display includes.

### OLED

Existing view-changed callback → `app_core_relay_oled_show` (unchanged).

## For tester

Acceptance in `docs/board_relay_oled_view_mode_architecture.md` § Acceptance.
