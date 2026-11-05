# Implementer handoff — `b03_hil`

Status: **`B03_RELAY_OLED_COLUMN_ORDER` code complete** (2026-11-04).

## Delivered

File: `components/app_core/app_core_relay_oled.c`

STA relay lines now pair hardware columns:

```text
HIL-03-<MAC4>
1:[X]  4:[X]
2:[X]  5:[X]
3:[X]  6:[X]
```

Token grammar, separators, and view-mode binding unchanged.

## For tester

Confirm left/right cells are `1/4`, `2/5`, `3/6` on STA screen.
