# Architect Decisions — `b06_hil`

Append dated decisions here. Do not rely on chat history as the source of truth.

## 2026-11-03 — Docs-only import from base

- Copied architecture, philosophy, and methodology docs from `b00_basehil`
  into this firmware tree.
- Did **not** copy firmware source (`components/`, `main/`, sdkconfig, build).
- Board particular files left as stubs until human supplies particularities.

## 2026-11-03 — Particular function and hardware from PCB

- Product: passive USB-C 5 V splitter, 1 input → 3 outputs, INA219 per output.
- Sources: `pcb/b06.kicad_sch`, `pcb/b06.kicad_pcb` (KiCad 7).
- **Canonical channel IDs follow silkscreen** `OUT 1` / `OUT 2` / `OUT 3`, not
  the schematic `OUT_PW_LED1..3` numerals (those are inverted vs silk for
  channels 1 and 3).
- Mapping frozen in `docs/board_hardware_description.md`:
  - OUT1 → J2 → U3 @ `0x42`
  - OUT2 → J3 → U2 @ `0x41`
  - OUT3 → J4 → U1 @ `0x40`
  - IN → J1 (`/PSWI`)
- MCU pins used: GPIO0 SCL, GPIO1 SDA, GPIO7 factory reset, GPIO8 error LED
  (same pattern as base bench for those four signals).
- Host API command names for metering remain TBD (draft intent only in
  `board_host_api_extensions.md`).

## 2026-11-03 — Passive channel LEDs out of firmware scope

- Human clarification: LEDs not connected to the MCU have no firmware
  importance.
- Firmware owns only error LED `D3` (`GPIO8`). `D1`/`D2`/`D4` and
  `/OUT_PW_LED1..3` (including `J5` pins 10/12/14) are PCB-only visual cues —
  no drivers, no host API, no acceptance criteria.

## 2026-11-03 — `B06_MAIN_METERING_OLED`

- Human decisions: (1) metering OLED only while STA connected, replacing base
  `WIFI OK` / IP / MAC screen; provisioning UX stays base. (2) host API
  metering deferred — not part of this handoff.
- Normative contract: `docs/board_main_metering_oled_architecture.md`.
- Layout: `FULL_FOUR_LINES` MEDIUM — line0 `HIL-06-<MAC4>`; lines1–3 fixed
  14-char columns with reserved current sign column (` ` or `-`) so digits do
  not shift: `"%d %4.2fV %c%4.2fA"` + `fabsf(I)`; fail `N ----V  ----A`.
- Sample period 500 ms; read order OUT1→OUT2→OUT3; power not shown on OLED.
- Provisional `BOARD_INA219_SHUNT_OHMS = 0.1f` until BOM `R5`/`R6`/`R8`.
- Implementer package recorded in `../implementer/handoff.md`.

## 2026-11-03 — Metering OLED current sign column

- Human request: keep numeric columns stable when current is negative.
- Spec change in `docs/board_main_metering_oled_architecture.md`: always emit
  current sign column (space if `I >= 0`, `-` if `I < 0`); magnitude via
  `fabsf`; display clamp `0.00`..`9.99` per field body.

## 2026-11-03 — Absorb `BASE-CHANGE-20261103-6` (4 MB flash / large factory)

- Imported base `docs/flash_partition_architecture.md` and synced overview
  pointers (`architecture.md`, platform scope, reuse contract, changelog).
- `b06_hil` adopts the same 4 MB / `factory` `0x3F0000` / NVS `0x9000` contract
  (SuperMini module); recorded in `board_hardware_description.md` + sync log.
- Did **not** edit `partitions.csv` / `sdkconfig.defaults` (architect hard stop;
  implementer pending).

## 2026-11-03 — `B06_HOST_API_METERING`

- Human request: architecture/docs for remote metering commands.
- Normative: `docs/board_host_api_metering_architecture.md`.
- Commands frozen: `meter_read` (one silk channel), `meter_read_all` (OUT 1..3).
- Units: `voltage_v`, `current_a` (signed), `power_w = voltage_v * current_a`.
- Soft per-channel `ok: false` on I2C fail; hard `bad_request` on bad args.
- No particular streams; no `echo`/`loopback` on this board.
- Sample path owned by `app_core_meter_*`; serialized with OLED metering loop.
- Checklist: `docs/board_host_api_extensions.md`. Implementer package pending.
