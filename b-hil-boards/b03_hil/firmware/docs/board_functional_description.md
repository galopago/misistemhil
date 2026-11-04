# Board Functional Description — `b03_hil`

Particular product/functional role for **this** repository only.

Template: `docs/hil_board_particular_spec_template.md`.

Authoritative hardware pins and connector mapping:
`docs/board_hardware_description.md` (from `../pcb/b03.kicad_pcb`).

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b03_hil
Board number: 03
One-line role:
  Six-channel relay HIL board with boot-safe buffered GPIO drive and shared
  family WiFi / discovery / host API.
```

## Functional mission

`b03_hil` provides **six independently controlled electromechanical relays** for
HIL switching of DUT signals or loads:

- An ESP32-C3 SuperMini runs the shared HIL base pillars (WiFi onboarding, mDNS
  `HIL-03-<MAC4>`, external host API) and the relay application logic.
- Relay command GPIOs pass through a **74HCT541** enable/buffer so ESP32 boot
  pin chatter cannot chatter the coils. Firmware enables the buffer only after
  all relay controls are outputs driven low.
- A **ULN2003** sinks the relay coils; contacts are brought out on connector
  `J2` for external wiring.
- On-board OLED and error LED provide the shared base provisioning/status UX;
  channel LEDs (`D1`…`D6`) show drive assertion per relay.
- Optional remote connector `J1` mirrors I2C, factory-reset, error LED, and
  per-channel LED sense lines.

## In scope for this board

- Safe power-up / enable sequencing for the six relay channels
- Local and remote indication of relay drive / error / reset
- Host API extensions to read/set relay states (`relay_set`,
  `relay_set_all`, `relay_get`, `relay_get_all` — see
  `docs/board_host_api_relay_architecture.md`)
- Shared family onboarding, discovery, and base host commands

## Out of scope for this board

- Continuous analog metering (belongs to boards such as `b06_hil`)
- Redefining family WiFi onboarding, mDNS discovery, or base host transport
- Assuming `b00_basehil` GPIO numbers without the map in
  `docs/board_hardware_description.md`

## Relationship to base pillars

- Uses base WiFi onboarding / discovery / host API: **yes**
- Board-specific functions via host API extensions: relay channel commands
  (see `docs/board_host_api_extensions.md`)

## Non-goals

- Do not redefine family WiFi, discovery, or base host protocol here
- Do not place pin numbers or MCU part numbers here (hardware description)
