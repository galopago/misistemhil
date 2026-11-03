# Board Particular Scope — `b06_hil`

```text
Board ID: b06_hil
Board number: 06
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

Particular additions (this repo only):
  - Passive USB-C 5 V 1→3 power splitter
  - Per-output INA219 voltage/current measurement (silk OUT 1/2/3)
  - Main metering OLED while STA connected (hostname + OUT V/I):
      docs/board_main_metering_oled_architecture.md
  - Remote host commands meter_read / meter_read_all:
      docs/board_host_api_metering_architecture.md
  - Remote 1x16 connector J5 for display/controls/LED sense
  - Channel identity MUST follow silkscreen (see hardware doc mapping table)

Explicit non-goals for this board:
  - Active regulation / isolation of the 5 V split
  - USB data-path HIL functions
  - Submodule packaging / build-time path dependency on b00_basehil
  - Forking or weakening family WiFi / discovery / host API contracts
```

Template: `docs/hil_board_particular_spec_template.md`.
