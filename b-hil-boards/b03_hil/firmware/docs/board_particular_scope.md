# Board Particular Scope — `b03_hil`

```text
Board ID: b03_hil
Board number: 03
Base tree referenced: b00_basehil/firmware
Last synced BASE-CHANGE ID: BASE-CHANGE-20261103-6
  (see docs/board_base_sync_log.md)

Pointers (required):
  - Functional description: docs/board_functional_description.md
  - Hardware description: docs/board_hardware_description.md

Base pillars consumed as working-copy contracts (cite docs/):
  - WiFi onboarding: docs/wifi_provisioning_architecture.md (+ related)
  - Network detection: docs/device_discovery_mdns_architecture.md
  - External host API: docs/external_host_api_architecture.md (+ command/stream
    siblings)

Particular architecture (implementation-ready):
  - docs/board_product_architecture.md          (B03_PRODUCT_BRINGUP)
  - docs/board_relay_control_architecture.md    (B03_RELAY_CONTROL +
      B03_RELAY_OE_LEVEL_BEFORE_DIR)
  - docs/board_host_api_relay_architecture.md   (B03_HOST_API_RELAY)
  - docs/board_main_relay_oled_architecture.md  (B03_MAIN_RELAY_OLED)
  - docs/board_relay_oled_view_mode_architecture.md (B03_RELAY_OLED_VIEW_MODE)
  - docs/board_host_api_relay_v2_architecture.md (B03_HOST_API_RELAY_V2_LLM)

Particular additions (this repo only):
  - Six DPDT relays (K1–K6) with contact connector J2
  - Boot-safe drive path: MCU KnCTR → 74HCT541 (/KOE) → KnDRV → ULN2003 → coil
  - Component relay_ctrl + host commands relay_set / relay_set_all /
    relay_get / relay_get_all
  - relay_sched timed pulse/hold + encoder step/train (v2 host API)
  - Main STA OLED: hostname + N:[X] view tokens (0/1/P/T/E) bound to sched
  - Channel LEDs D1–D6 + J1 LED_EXT mirrors (hardware; no firmware PWM)

Explicit non-goals for this board (current handoffs):
  - NVS persistence of relay state
  - echo / loopback demos
  - Submodule packaging / build-time path dependency on b00_basehil
  - Forking or weakening family WiFi / discovery / host API contracts
```

Template: `docs/hil_board_particular_spec_template.md`.
