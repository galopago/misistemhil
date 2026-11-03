# Board Functional Description — `b06_hil`

Particular product/functional role for **this** repository only.

Template: `docs/hil_board_particular_spec_template.md`.

Authoritative hardware pins and connector mapping:
`docs/board_hardware_description.md` (from `../pcb/b06.kicad_sch` /
`../pcb/b06.kicad_pcb`, KiCad 7).

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b06_hil
Board number: 06
One-line role:
  Passive USB 5 V power splitter (1 in → 3 out) with per-output voltage and
  current measurement for HIL supervision.
```

## Functional mission

`b06_hil` is a **passive USB-C 5 V power splitter** for the HIL family:

- One USB-C **power-only** input receives 5 V from a host/supply.
- Three USB-C **power-only** outputs distribute that same 5 V rail to DUTs or
  downstream loads (no DC-DC conversion; VBUS is passively split).
- Each output channel has an **INA219** that measures bus voltage and current
  (and thus power) on that channel.
- An ESP32-C3 SuperMini runs the shared HIL base pillars (WiFi onboarding,
  mDNS `HIL-06-<MAC4>`, host API) and exposes channel measurements to an
  external host/agent.
- On-board OLED and MCU-driven error LED (`D3`) provide the shared base
  provisioning/status UX; after STA connect, the **main product OLED** shows
  mDNS hostname plus per-output voltage/current (see
  `docs/board_main_metering_oled_architecture.md`).
- Optional remote connector (`J5`) can mirror I2C, reset, and the MCU error LED.
  Passive channel power LEDs (`D1`/`D2`/`D4` and related nets) are PCB-only and
  out of firmware scope.

## In scope for this board

- Passive 1→3 USB-C 5 V split with per-channel polyfuse protection.
- Per-output INA219 telemetry (voltage, current, power) on a shared I2C bus.
- Shared base platform: WiFi provisioning, factory reset, mDNS identity
  `HIL-06-<MAC4>`, OLED/error-LED UX, external host API.
- Main metering OLED while STA connected (hostname + OUT 1..3 V/I):
  `docs/board_main_metering_oled_architecture.md` (handoff
  `B06_MAIN_METERING_OLED`).
- Host-visible channel identity aligned with **PCB silkscreen** `OUT 1` /
  `OUT 2` / `OUT 3` (see hardware doc for silk↔schematic net mapping).

## Out of scope for this board

- Active voltage regulation or isolation between input and outputs.
- USB data / CC negotiation beyond power-only receptacles (power path only).
- Firmware involvement with passive channel LEDs `D1`/`D2`/`D4` or
  `OUT_PW_LED*` remote sense lines (not MCU-connected).
- Specialty HIL roles of other boards (RS-485, relays, logic analyzer, …).
- Redefining family WiFi, discovery, or base host transport contracts.

## Relationship to base pillars

- Uses base WiFi onboarding / discovery / host API: **yes**
- Main product OLED after STA connect: replaces base `WIFI OK` / IP / MAC
  connected screen with metering layout
  (`docs/board_main_metering_oled_architecture.md`).
- Board-specific host API extensions: per-channel power telemetry — see
  `docs/board_host_api_extensions.md` and normative
  `docs/board_host_api_metering_architecture.md` (commands `meter_read`,
  `meter_read_all`; handoff `B06_HOST_API_METERING`).

## Non-goals

- Do not redefine family WiFi, discovery, or host protocol inside this file.
- Do not place pin numbers or MCU part numbers here (see
  `docs/board_hardware_description.md`).
- Do not describe other boards' product missions here.
