# Implementer Backlog

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Pending (authorized now)

Execute in order — details in `handoff.md` → **Next Tasks**:

1. **setup_url** — `components/setup_url/` per `docs/qr_encoder_interface.md`.
2. **DISPLAY_DELIVERY** — `app_core_display_show_*` callbacks; sole display caller.
3. **DISPLAY_FIX** — remove hardcoded URL in `display_controller_show_template` QR branch.
4. **QR_ENCODER** — Nayuki vendor + real `display_qr_generate`.
5. **QR_INTEGRATION** — build, flash/RAM note, hardware QR scan test for tester.

## Pending (not authorized yet)

- `I2C_BUS_PHASE3`: `i2c_broker` when architect activates phase 3 handoff.
- SH1106 driver: only if hardware changes (v1 target is SSD1306).
- INA219 driver: separate future handoff.

## Completed

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
