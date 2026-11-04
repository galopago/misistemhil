# Architect handoff — `b03_hil`

Status: **`B03_RELAY_OLED_VIEW_MODE` docs complete** (2026-11-03). Firmware
binding is **pending implementer**.

## Active handoff

| ID | Doc |
| --- | --- |
| `B03_RELAY_OLED_VIEW_MODE` | `docs/board_relay_oled_view_mode_architecture.md` |

Also updated: `docs/board_main_relay_oled_architecture.md` (supersede reserved
token rule).

## Tester input absorbed

During pulse / one-shot / train / encoder, OLED stayed `N:[0]` while coils/LEDs
moved. Expected under old “emit only 0/1” rule; product now requires mode arming.

## Pending (implementer)

See `agent-workspaces/implementer/handoff.md`:

1. Add `relay_ctrl_set_view_mode`.
2. Arm/clear modes in `relay_sched` per binding table.
3. Keep `drive*` view-inert; OLED refreshes via existing callback.
