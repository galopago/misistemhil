# Architect decisions — `b03_hil`

Append-only. Record architectural decisions for this board.

## 2026-11-03 — Pin authority and relay OE

- **PCB copper / pad pinfunction** in `../pcb/b03.kicad_pcb` is authoritative
  for GPIO assignment. Schematic net labels near the SuperMini symbol are not
  trusted when they disagree with the PCB.
- Relay path is MCU `KnCTR` → `U8` 74HCT541 → `KnDRV` → `U2` ULN2003 → coil.
- `/KOE` on `GPIO2` is active-low OE for `U8` (G1+G2), pulled high by `R18`.
- Firmware MUST set all six CTR outputs low before driving `/KOE` low.
- Shared base-like pins on this PCB: I2C `GPIO0/1`, error LED `GPIO8`,
  factory reset `GPIO7`.

## 2026-11-03 — Implementation-ready freeze (`B03_*`)

- Adapt-copy base firmware; no submodule / live path dependency.
- New component `components/relay_ctrl/` owns GPIO + mutex + shadow state.
- `relay_ctrl_init` runs in `app_core_start` **before** I2C/display/WiFi.
- Cold boot: all relays off; no NVS persistence of relay state in v1.
- OLED: keep base UX only (no particular relay main screen in v1).
- Host commands frozen: `relay_set`, `relay_set_all`, `relay_get`,
  `relay_get_all`.
- `relay_set_all` accepts **exactly one** of `mask` (0…63) or `states`[6].
- Do not ship `echo` / `loopback` on this board.
- `capabilities.summary` exact string:
  `Six-channel relay HIL board with boot-safe buffered GPIO drive`.

## 2026-11-03 — OE level-before-direction (`B03_RELAY_OE_LEVEL_BEFORE_DIR`)

Source: tester RCA in `agent-workspaces/tester/handoff.md` (power-cycle short
relay chatter).

- ESP-IDF `gpio_config(OUTPUT)` / `gpio_set_direction(OUTPUT)` on ESP32-C3
  enables the pad with the current OUT bit (default **0**).
- `/KOE` is active-low → configuring OE as output **before**
  `gpio_set_level(OE, 1)` briefly **enables** the 74HCT541 while CTR pins are
  still unsafe → audible short oscillation.
- Normative init **MUST** preload inactive levels with `gpio_set_level`, then
  enable output direction, for OE and all CTR; only then drive OE active.
- “Configure as OUTPUT, then set level” is an explicit anti-pattern for this
  board.
- Implementer must patch `relay_ctrl_init`; architect does not edit firmware.
