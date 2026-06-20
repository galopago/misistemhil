# Implementer Backlog

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Pending

- `I2C_BUS_PHASE3`: implement `i2c_broker` when architect handoff is active.
- QR encoder integration when library is selected.
- SH1106 support if hardware differs from SSD1306.
- Keep the ESP-IDF project buildable.

## Completed

- `I2C_BUS_ARCHITECTURE` phase 1: shared bus, board helper, app_core startup.
- `I2C_BUS_PHASE2`: `i2c_bus_transceive`, bus mutex, SSD1306 via transceive.

## Entry Format

```text
ID:
Source:
Files to modify:
Implementation plan:
Risks:
Status:
```
