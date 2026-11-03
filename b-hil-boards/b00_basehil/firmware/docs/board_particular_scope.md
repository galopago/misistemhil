# Board Particular Scope — `b00_basehil`

```text
Board ID: b00_basehil
Board number: 00
Base tree referenced: b00_basehil/firmware (this repository)
Last synced BASE-CHANGE ID: BASE-CHANGE-20261102-3
  (see docs/board_base_sync_log.md)

Pointers (required):
  - Functional description: docs/board_functional_description.md
  - Hardware description: docs/board_hardware_description.md

Base pillars consumed as-is (cite base docs):
  - WiFi onboarding: docs/wifi_provisioning_architecture.md (+ related)
  - Network detection: docs/device_discovery_mdns_architecture.md
  - External host API: docs/external_host_api_architecture.md (reserved / pending detail)

Particular additions (this repo only):
  - Acts as the family base reference + minimum human-supervised bench profile
  - Filled particular functional/hardware docs as the copyable example for bNN_hil

Explicit non-goals for this board:
  - Specialty HIL product functions (RS-485, relays, logic analyzer, …)
  - Submodule packaging of this tree into sibling boards
```

Template: `docs/hil_board_particular_spec_template.md`.
