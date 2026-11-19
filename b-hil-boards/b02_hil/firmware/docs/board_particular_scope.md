# Board Particular Scope — `b02_hil`

```text
Board ID: b02_hil
Board number: 02
Base tree referenced at bootstrap: b00_basehil/firmware
  (local working copies under docs/; no build-time path dependency)
Last synced BASE-CHANGE ID: BASE-CHANGE-20261103-6
  (see docs/board_base_sync_log.md)

Pointers (required):
  - Functional description: docs/board_functional_description.md
  - Hardware description: docs/board_hardware_description.md
  - GPIO bring-up (boot-safe): docs/board_gpio_bringup_architecture.md
  - UART path/line/config: docs/board_uart_path_line_architecture.md
  - UART framing matrix: docs/board_uart_framing_architecture.md
  - UART stream: docs/board_uart_stream_architecture.md
  - STA main OLED: docs/board_main_uart_oled_architecture.md

Base pillars consumed as working-copy contracts (cite docs/):
  - WiFi onboarding: docs/wifi_provisioning_architecture.md (+ related)
  - Network detection: docs/device_discovery_mdns_architecture.md
  - External host API: docs/external_host_api_architecture.md (+ command/stream
    siblings)

Particular additions (this repo only):
  - Galvanically isolated UART (ADuM1412 + PS1) muxed TTL/RS-485 via K1
  - Relay-switched RS-485 termination (K2), pull-down (K3), pull-up (K4)
  - ULN2003 coil driver + 74HCT541 /KOE anti-glitch buffer
  - board_ctrl cold init (level-before-direction) before platform onboarding
  - relay_set / relay_get (coil test) — board_host_api_relay_architecture.md
  - uart_path_* / uart_line_* / uart_config_* (semantic; default 9600 8N1)
  - Stream dut_uart with automatic RS-485 DE around TX
  - STA OLED: identity + PATH + T/U/D + BAUD (minimal HMI)

Explicit non-goals:
  - relay_set_all / masks / states[] / timed relay actions
  - loopback / echo demos
  - User-facing DE command
  - Git submodule / EXTRA_COMPONENT_DIRS / live path dependency on b00_basehil
  - Forking family WiFi / discovery / host API contracts
```

Template: `docs/hil_board_particular_spec_template.md`.
