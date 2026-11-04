# Implementer handoff — `b03_hil`

Status: **OE glitch patch complete** (2026-11-03). Awaiting tester power-cycle
retest.

## Delta delivered — `B03_RELAY_OE_LEVEL_BEFORE_DIR`

File: `components/relay_ctrl/relay_ctrl.c` (`relay_ctrl_init` only).

Init now uses **level-before-direction**:

1. `gpio_set_level(OE, inactive=1)` then `gpio_config(OE, OUTPUT)`.
2. `gpio_set_level` all CTR inactive, then `gpio_config` all CTR as OUTPUT.
3. Re-assert CTR lows; shadow `0`.
4. Only then `gpio_set_level(OE, active=0)`.

No public API / pin / host / boot-site changes.

## Prior packages A–E

Still delivered (bring-up). See notes for Option B host wiring.

## For tester

- Power-cycle / reset listen (and optional `/KOE` scope) per relay acceptance
  #10 and product criterion #9.
- If short chatter remains, return to architect (possible UART remux — out of
  this patch scope).
