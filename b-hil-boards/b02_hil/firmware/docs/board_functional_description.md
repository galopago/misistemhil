# Board Functional Description — `b02_hil`

Particular product/functional role for **this** repository only.

Template: `docs/hil_board_particular_spec_template.md`.

Authoritative hardware pins and connector mapping:
`docs/board_hardware_description.md` (from `../pcb/b02.kicad_pcb`, title
`HIL-02`).

Boot-safe GPIO bring-up: `docs/board_gpio_bringup_architecture.md`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b02_hil
Board number: 02
One-line role:
  Galvanically isolated UART HIL interface, relay-muxed TTL/RS-485 with
  relay-switched bias/termination, plus shared family WiFi onboarding.
```

## Functional mission

`b02_hil` presents an **isolated serial HIL port** to a DUT:

- An ESP32-C3 SuperMini runs shared HIL platform pillars (WiFi onboarding,
  mDNS `HIL-02-<MAC4>`, external host API) and board control logic.
- MCU UART is **galvanically isolated** (ADuM1412 + isolated DC-DC) before it
  reaches the DUT connector domain.
- One DPDT relay (**K1**) multiplexes that isolated UART between **TTL** and
  **RS-485** (MAX485E).
- Three further relays enable RS-485 line network: **termination** (**K2**),
  **pull-down** (**K3**), and **pull-up** (**K4**).
- **ULN2003** drives the relay coils; a **74HCT541** with active-low `/KOE`
  buffers MCU GPIOs so ESP32 boot/GPIO claim cannot spuriously energize
  coils or DE.
- OLED + error LED follow shared onboarding / status UX until particular
  product screens are authorized.

## In scope for this board

- Safe `/KOE` + relay CTR + DE GPIO bring-up (no boot chatter)
- Isolated UART path with relay-selected TTL vs RS-485
- Relay-enabled RS-485 pull-up / pull-down / termination
- Shared family WiFi onboarding, discovery, and base host commands
- Later: host API / UART product control (separate architect handoffs)

## Out of scope for this board

- General-purpose multi-channel relay matrix product (`b03_hil`)
- Analog metering product (`b06_hil`)
- Redefining family WiFi / mDNS / base host transport
- Assuming base-bench GPIO numbers beyond this board's hardware map

## Relationship to base pillars

- Uses base WiFi onboarding / discovery / host API: **yes**
- Board-specific host extensions: **none** in the current bootstrap handoff
  (see `docs/board_host_api_extensions.md`)

## Non-goals

- Do not redefine family WiFi, discovery, or base host protocol here
- Do not place pin numbers here (hardware description / GPIO bring-up docs)
