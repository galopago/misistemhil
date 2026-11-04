# Board Host API Extensions — `b03_hil`

Human / implementer checklist. **Runtime discovery for agents is on-device
`capabilities`**, not this file.

Normative detail:

- v1 state commands: `docs/board_host_api_relay_architecture.md`
  (`B03_HOST_API_RELAY`)
- v2 timed / encoder (pending firmware): `docs/board_host_api_relay_v2_architecture.md`
  (`B03_HOST_API_RELAY_V2_LLM`)

```text
Base contracts:
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities
Extra commands (board product) — v1 FROZEN / implemented:
  - relay_set
  - relay_set_all
  - relay_get
  - relay_get_all
Extra commands (board product) — v2 PENDING implementer:
  - relay_pulse
  - relay_hold
  - relay_hold_many
  - relay_hold_cancel
  - relay_release_all
  - encoder_step          (exactly 2 channels + direction + period_ms)
  - encoder_pulse_train   (exactly 1 channel; no direction)
  - encoder_cancel
Streams (board product):
  - none
Expected capabilities.summary (English):
  Six-channel relay HIL board with boot-safe buffered GPIO drive
Expected info.board_id / board_number: b03_hil / 03
Expected info.streams_supported: false
Expected capabilities.timing_limits: see relay v2 architecture (min 20 ms, etc.)
Non-extensions (forbidden):
  - echo / loopback on this board
  - Blocking vTaskDelay for pulse/hold/encoder durations
  - Replacing base discovery or WiFi onboarding protocols
  - Shadow APIs that duplicate base host semantics under new names without cause
  - Teaching agents to read this Markdown instead of calling capabilities
```

Embed the same catalog in firmware `capabilities` when implementing v2.
