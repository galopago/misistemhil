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

## 2026-11-03 — Main relay OLED (`B03_MAIN_RELAY_OLED`)

- STA-connected OLED replaces base `WIFI OK` five-line screen.
- Layout: line 0 = `HIL-03-<MAC4>`; lines 1–3 = hardware columns
  `1:[X]  4:[X]` / `2:[X]  5:[X]` / `3:[X]  6:[X]` (left 1→2→3, right 4→5→6;
  exactly two spaces; square brackets; 12 chars/line).
- Token alphabet: `0`/`1` stable static; `P` pulse; `T` train; `E` encoder.
  Dynamic modes must **not** flicker `0`/`1` on the OLED.
- Three tokens per line forbidden (overflow MEDIUM 16-char budget).
- Pulse/train/encoder drivers deferred; formatter must accept reserved tokens.
- Event-driven refresh on STA connect and static set; no required 2 Hz loop.

## 2026-11-03 — OLED view-mode binding (`B03_RELAY_OLED_VIEW_MODE`)

Tester: pulse/hold/train/encoder left OLED at `0` (LEDs moved). Cause:
`relay_sched` used `relay_ctrl_drive*` only.

- Authorize product `P`/`T`/`E` (no longer “reserved only”).
- Add `relay_ctrl_set_view_mode` (mode only + callback; no GPIO).
- `drive*` stays view-inert; edges do not flicker tokens.
- Mapping: pulse/hold/hold_many → `P`; pulse_train → `T`; encoder_step A+B →
  `E`; end/cancel → `STATIC_OFF` (then static set may apply `0`/`1`).
- Hold shares token `P` (no separate `H`).
- `relay_sched` must not call display APIs.

## 2026-11-03 — Relay host API v2 for LLM (`B03_HOST_API_RELAY_V2_LLM`)

- Timed actions are **non-blocking**: arm `esp_timer` / state machine; never
  `vTaskDelay` for pulse/hold/encoder duration; HTTP returns immediately.
- Per-channel one-shot release timers; `relay_pulse` and `relay_hold` share
  backend (ranges 20…4000 vs 20…60000 ms).
- `relay_hold_many` arms multiple channels in one request; one timer per channel.
- Preempt: `relay_set` / `set_all` / `release_all` cancel timers and abort
  encoder if channels overlap; re-arm replaces timer.
- Electromechanical floors: min ON 20 ms; pulse-train period min 40 ms; out of
  range → `bad_request` (no silent clamp); publish `capabilities.timing_limits`.
- `encoder_step`: exactly two channels + `direction` + `period_ms` (speed);
  4 quadrature states per step.
- `encoder_pulse_train`: exactly one channel; no `direction`; `pulse_ms` +
  `period_ms`.
- One encoder run at a time; second → `busy`.
- Suggested module `relay_sched` calling `relay_ctrl`; host_api must not GPIO.
- Authoritative doc: `docs/board_host_api_relay_v2_architecture.md`.

