# Architect Backlog

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Pending

- Confirm the pin map against the schematic.
- Define the functional scope of the first firmware.
- Decide the initial peripherals and acceptance criteria.
- Identify hardware risks before implementing drivers.

## Ready For Handoff

```text
ID: OLED_TEXT_DISPLAY_INTERFACE
Objective: Define the initial architecture contract for a 0.96 inch I2C OLED
  text display interface with runtime region layouts and constrained QR support.
Context: The human-provided technical specification has been normalized into
  `docs/oled_text_display_interface.md` and linked from the firmware
  architecture and test strategy.
Expected files:
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/implementer/handoff.md
Acceptance criteria:
  - The display contract is implementation-ready and independent from a concrete
    I2C driver.
  - The implementer can derive module boundaries, APIs, rendering behavior, QR
    behavior, invalid input handling, and validation cases without hidden chat
    context.
  - Physical I2C enablement remains blocked until pins, address, and driver are
    confirmed.
Risks:
  - QR support depends on selecting a standards-compliant encoder suitable for
    the target footprint.
  - Font availability can vary by graphics stack, so tests must assert
    deterministic geometry separately from font-specific pixels.
Status: Ready for implementer handoff.
```

## Task Format

```text
ID:
Objective:
Context:
Expected files:
Acceptance criteria:
Risks:
Status:
```
