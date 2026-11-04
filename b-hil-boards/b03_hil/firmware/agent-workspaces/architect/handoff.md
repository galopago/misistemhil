# Architect handoff — `b03_hil`

Status: **docs updated** for OE power-cycle glitch (2026-11-03). Firmware patch
is **pending implementer**.

## Active delta

| ID | Doc / action |
| --- | --- |
| `B03_RELAY_OE_LEVEL_BEFORE_DIR` | Harden `docs/board_relay_control_architecture.md` init sequence |
| (related) | `docs/board_product_architecture.md`, `docs/board_hardware_description.md` |

Prior closed handoffs: `B03_PRODUCT_BRINGUP`, `B03_RELAY_CONTROL`,
`B03_HOST_API_RELAY`.

## Tester input absorbed

`agent-workspaces/tester/handoff.md` RCA: `gpio_config(OE, OUTPUT)` before
inactive `gpio_set_level` drives `/KOE` low (active) briefly → short relay
chatter on power cycle.

## Architect deliverables (this turn)

- Normative **level-before-direction** init sequence + forbidden anti-pattern
- Power-cycle acceptance criterion #10 on relay control doc
- Decision log entry; implementer reopened for `relay_ctrl_init` patch only

## Pending (implementer)

See `agent-workspaces/implementer/handoff.md` — patch `relay_ctrl_init` only;
no host API / pin map changes.

## Pending (tester after patch)

Power-cycle / reset listen (and optional `/KOE` scope) per relay acceptance #10.
