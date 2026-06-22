# Implementer Backlog

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Pending (authorized now)

- `DISPLAY_VISUAL_DEMO_PROTOCOL`: Kconfig + `app_core_display_demo.c` per
  `docs/display_visual_demo_protocol.md` and architect handoff.

## Pending (not authorized yet)

- `I2C_BUS_PHASE3`: `i2c_broker` when architect activates phase 3 handoff.
- SH1106 driver: only if hardware changes (v1 target is SSD1306).
- INA219 driver: separate future handoff.

## Completed

- `DISPLAY_DELIVERY_CONTRACT`: app_core display callback API.
- `QR_ENCODER_INTERFACE`: setup_url, Nayuki qrcodegen, display_qr_generate.
- `I2C_BUS_ARCHITECTURE` phase 1: shared bus, board helper, app_core startup.
- `I2C_BUS_PHASE2`: `i2c_bus_transceive`, bus mutex, SSD1306 via transceive.
- `OLED_TEXT_DISPLAY_INTERFACE` base stack (renderer, task, SSD1306 when OLED present).

## Entry Format

```text
ID:
Source:
Files to modify:
Implementation plan:
Risks:
Status:
```
