# Board Host API Extensions — `b06_hil`

Human / implementer checklist. **Runtime discovery for agents is on-device
`capabilities`**, not this file.

Normative metering command contract:
`docs/board_host_api_metering_architecture.md` (handoff `B06_HOST_API_METERING`).

```text
Base contracts:
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities

Extra commands (particular — FROZEN by B06_HOST_API_METERING):
  - meter_read
      args: { "channel": 1|2|3 }   # silk OUT n
      result: { channel, ok, voltage_v?, current_a?, power_w? }
  - meter_read_all
      args: {}
      result: { "channels": [ <three channel objects in order 1..3> ] }
  - Units: voltage_v (V), current_a (A, signed), power_w (W = V*I)
  - Channel map:
      1 → INA219 0x42 (U3 / J2) silk OUT 1
      2 → INA219 0x41 (U2 / J3) silk OUT 2
      3 → INA219 0x40 (U1 / J4) silk OUT 3
  - Full schemas, errors, capabilities descriptors, concurrency:
      docs/board_host_api_metering_architecture.md

Streams (particular):
  - none for v1 metering (polling commands only)
  - info.streams_supported: false

Expected capabilities.summary (English):
  Passive USB 5V 1-to-3 splitter with per-output INA219 metering

Not on this board (unless a future handoff adds them):
  - echo, loopback (base demos)

Non-extensions (forbidden):
  - Replacing base discovery or WiFi onboarding protocols
  - Shadow APIs that duplicate base host semantics under new names without cause
  - Teaching agents to read this Markdown instead of calling capabilities
  - Exposing channels by schematic OUT_PW_LED numeral instead of silk OUT n
```
