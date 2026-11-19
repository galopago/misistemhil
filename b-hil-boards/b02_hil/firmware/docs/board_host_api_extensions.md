# Board Host API Extensions — `b02_hil`

Human / implementer checklist. **Runtime discovery for agents is on-device
`capabilities`**, not this file.

Normative detail:

- Coil test: `docs/board_host_api_relay_architecture.md`
  (`B02_HOST_API_RELAY_SINGLE`)
- Path / line / config: `docs/board_uart_path_line_architecture.md`
  (`B02_UART_PATH_LINE`)
- DUT stream: `docs/board_uart_stream_architecture.md` (`B02_UART_STREAM`)

```text
Base contracts (local working copies):
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities
Extra commands:
  - relay_set / relay_get
      (channel 1..4 coil test; one coil per request; no masks)
  - uart_path_set / uart_path_get
      (path: ttl | rs485 → K1; no baud change)
  - uart_line_set / uart_line_get
      (term / pullup / pulldown → K2 / K4 / K3; partial update OK)
  - uart_config_set / uart_config_get
      (baud list + full HW framing matrix; default 9600 8N1; busy if stream open)
      see docs/board_uart_framing_architecture.md
Streams:
  - stream_id: dut_uart
    Purpose: Binary frames ↔ isolated DUT UART (TTL or RS-485 per path)
    params schema: {} or optional baud/data_bits/parity/stop_bits
    Payload semantics: opaque bytes; RS-485 write auto DE drive→TX→receive
    Hardware backend: GPIO5/6 UART + K1 mux + 485_CTR DE + line relays
Expected capabilities.summary (English):
  Isolated RS-485/TTL HIL board with dut_uart stream and path/line control
Expected info.board_id / board_number: b02_hil / 02
Expected info.streams_supported: true
Non-extensions (forbidden):
  - relay_set_all / mask / states[]
  - echo / loopback
  - User command for 485_CTR / DE
  - Replacing base discovery or WiFi onboarding protocols
  - Teaching agents to read this Markdown instead of calling capabilities
```

Firmware `capabilities` **MUST** match this checklist when the UART handoffs
are implemented.
