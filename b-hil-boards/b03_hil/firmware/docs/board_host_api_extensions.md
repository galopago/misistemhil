# Board Host API Extensions — `b03_hil`

Human / implementer checklist. **Runtime discovery for agents is on-device
`capabilities`**, not this file.

Normative detail: `docs/board_host_api_relay_architecture.md`
(`B03_HOST_API_RELAY`).

```text
Base contracts:
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities
Extra commands (board product) — FROZEN names:
  - relay_set
  - relay_set_all
  - relay_get
  - relay_get_all
Streams (board product):
  - none
Expected capabilities.summary (English):
  Six-channel relay HIL board with boot-safe buffered GPIO drive
Expected info.board_id / board_number: b03_hil / 03
Expected info.streams_supported: false
Non-extensions (forbidden):
  - echo / loopback on this board
  - Replacing base discovery or WiFi onboarding protocols
  - Shadow APIs that duplicate base host semantics under new names without cause
  - Teaching agents to read this Markdown instead of calling capabilities
```

Embed the same catalog in firmware `capabilities`.
