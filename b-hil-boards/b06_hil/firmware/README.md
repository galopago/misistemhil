# b06_hil Firmware

ESP-IDF firmware for the `b06_hil` board, based on ESP32-C3.

All agentic development methodology documentation in this firmware tree is
maintained in English.

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
