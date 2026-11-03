# b00_basehil Firmware

ESP-IDF base firmware for `b00_basehil` (shared HIL base), based on ESP32-C3.

All agentic development methodology documentation in this firmware tree is
maintained in English.

This project is the **shared base** for the HIL board family: WiFi onboarding,
network board detection, and external host API/protocol methodology live here.
Each board repo keeps separate particular docs for its **functional role** and
**hardware** (MCU/pins may differ). Other boards reuse `docs/` contracts and
may adapt this code; they do not link it as a submodule. See
`docs/hil_base_platform_scope.md`, `docs/hil_base_reuse_contract.md`, and
`docs/architecture.md`.

## Requirements

- ESP-IDF installed on the development computer.
- ESP-IDF environment variables are not created globally and must not be assumed
  to exist.
- ESP32-C3 target configured with the locally available ESP-IDF tools.

## Initial Use

Locate the local ESP-IDF tools at command execution time. Do not write the
workstation-specific absolute ESP-IDF path into committed files.

```bash
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Structure

- `main/`: firmware entry point.
- `components/board/`: board details and pin map.
- `components/app_core/`: main application logic.
- `docs/`: methodology, architecture, and test strategy.
- `agent-workspaces/`: separate work area for the architect, implementer, and
  tester.

## Hardware Note

Do not assume final pins from firmware. Every pin must be confirmed against the
schematic in `../pcb/` and recorded in the corresponding handoff.
