# Architect handoff — `b03_hil`

Status: **`B03_RELAY_OLED_COLUMN_ORDER` docs complete** (2026-11-04). Small GUI
pairing change for implementer.

## Active delta

| ID | Doc |
| --- | --- |
| `B03_RELAY_OLED_COLUMN_ORDER` | `docs/board_main_relay_oled_architecture.md` |

## Change

OLED channel pairing must match hardware columns:

```text
1:[X]  4:[X]
2:[X]  5:[X]
3:[X]  6:[X]
```

(Same `N:[X]` cells and two-space separator; only left/right channel numbers
change. Forbidden: old `1–2` / `3–4` / `5–6` line pairing.)

## Pending (implementer)

Update `app_core_relay_oled_show` / `format_pair_line` calls to `(1,4)`,
`(2,5)`, `(3,6)`. See `agent-workspaces/implementer/handoff.md`.
