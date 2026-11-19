# HIL Base Specification Changelog

Append-only log of **normative base documentation changes** that derivative
board repositories may need to absorb.

Source handoff: `agent-workspaces/architect/handoff.md`, `HIL_BASE_REUSE_CONTRACT`
(extension: platform pillars and particular separation).

Rules:

- Every material change to a base pillar or support contract that can affect
  `bNN_hil` particulars MUST add an entry here.
- Entries use a stable ID: `BASE-CHANGE-YYYYMMDD-N` (N starts at 1 per day).
- Entries MUST list affected base docs and the **action required** in particular
  repos (read / adapt code / update particular delta / none).
- Do not rewrite history; add correcting entries if needed.
- Editorial-only typos that cannot change behavior MAY omit a changelog entry.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## How an LLM uses this file

1. Open the particular board's sync log (see
   `docs/hil_board_particular_spec_template.md`).
2. Find the last consumed `BASE-CHANGE-…` ID.
3. Read every newer entry in this file in order.
4. For each entry, apply the listed particular actions in the **board repo**
   files (never by editing base normative docs from the board session).
5. Append the consumed IDs to the board sync log.

## Entries

### BASE-CHANGE-20261102-1

```text
ID: BASE-CHANGE-20261102-1
Date: 2026-11-02
Summary: Establish family base pillars and file separation for base vs particular.
Pillars affected:
  - WiFi onboarding (ownership clarified; existing WiFi docs remain authoritative)
  - Network board detection (ownership clarified; mDNS doc remains authoritative)
  - External host communication (new reserved base doc)
Affected base docs:
  - docs/hil_base_platform_scope.md (new)
  - docs/hil_base_spec_changelog.md (new)
  - docs/hil_board_particular_spec_template.md (new)
  - docs/external_host_api_architecture.md (new; reserved / pending detail)
  - docs/hil_base_reuse_contract.md (updated)
  - docs/architecture.md (updated)
Required particular-repo actions:
  - Read docs/hil_base_platform_scope.md and docs/hil_base_reuse_contract.md.
  - Create board particular docs using docs/hil_board_particular_spec_template.md
    if the board firmware tree exists.
  - Do not invent a parallel family host API; wait for or extend
    docs/external_host_api_architecture.md per particular template rules.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none for this documentation change.
```

### BASE-CHANGE-20261102-2

```text
ID: BASE-CHANGE-20261102-2
Date: 2026-11-02
Summary: Require separate particular docs for board functional role and
  hardware (MCU/pins); both vary per board and must not be mixed with base
  pillars or with each other.
Pillars affected:
  - None directly (particular-surface clarification)
Affected base docs:
  - docs/hil_board_particular_spec_template.md (updated)
  - docs/hil_base_platform_scope.md (updated)
  - docs/hil_base_reuse_contract.md (updated)
  - docs/architecture.md (updated)
Required particular-repo actions:
  - Create docs/board_functional_description.md (product mission of the board).
  - Create docs/board_hardware_description.md (MCU/module + pin→function map).
  - Point board_particular_scope.md at those two files.
  - Do not assume b00_basehil ESP32-C3 pin profile; confirm against local PCB.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none for this documentation change.
```

### BASE-CHANGE-20261102-3

```text
ID: BASE-CHANGE-20261102-3
Date: 2026-11-02
Summary: Fill b00_basehil particular functional and hardware docs as the
  concrete example (minimum human-supervised base bench on ESP32-C3 SuperMini).
Pillars affected:
  - None (particular example for this tree)
Affected base docs:
  - docs/board_functional_description.md (new, filled)
  - docs/board_hardware_description.md (new, filled)
  - docs/board_particular_scope.md (new)
  - docs/board_host_api_extensions.md (new; none)
  - docs/board_base_sync_log.md (new)
  - docs/hil_board_particular_spec_template.md (example pointer)
  - docs/architecture.md (example pointer)
Required particular-repo actions:
  - When creating a new bNN_hil firmware docs set, copy the particular file
    philosophy from this example, then rewrite functional + hardware content
    for that board (do not keep b00_basehil GPIO map or "base bench" mission).
  - Record this ID as consumed in the board sync log when the board docs are
    created or updated accordingly.
Code impact in base tree: none for this documentation change.
```

### BASE-CHANGE-20261103-1

```text
ID: BASE-CHANGE-20261103-1
Date: 2026-11-03
Summary: Clarify that extra I2C product devices (INA219 example) belong on
  daughter boards; b00_basehil minimum test profile is OLED-only on the shared
  bus. i2c_bus remains device-agnostic and unchanged.
Pillars affected:
  - Support contract: I2C bus ownership split (bus base / slaves particular)
Affected base docs:
  - docs/i2c_bus_architecture.md
  - docs/esp32_c3_supermini_connections.md
  - docs/architecture.md
  - docs/hil_base_platform_scope.md
  - docs/hil_base_reuse_contract.md
  - docs/hil_board_particular_spec_template.md
Required particular-repo actions:
  - If the board has INA219 or other extra I2C slaves, list them only in
    board_hardware_description.md (and board component macros).
  - Do not treat INA219 as required because early base experiments mentioned it.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none (documentation only; drivers untouched).
```

### BASE-CHANGE-20261103-2

```text
ID: BASE-CHANGE-20261103-2
Date: 2026-11-03
Summary: Specify family external host API v1 concept — HTTP JSON slow commands
  and TCP length-prefixed binary streams (control vs data planes).
Pillars affected:
  - External host communication (pillar 3)
Affected base docs:
  - docs/external_host_api_architecture.md (filled)
  - docs/external_host_command_api.md (new)
  - docs/external_host_stream_api.md (new)
  - docs/hil_base_platform_scope.md
  - docs/hil_base_reuse_contract.md
  - docs/architecture.md
  - docs/hil_board_particular_spec_template.md
  - docs/board_host_api_extensions.md
Required particular-repo actions:
  - Read the three host API docs; do not invent a parallel protocol.
  - Update board_host_api_extensions.md using the new template sections
    (extra commands / streams).
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none yet (docs only; firmware pending
  EXTERNAL_HOST_API_V1_CONCEPT).
```

### BASE-CHANGE-20261103-3

```text
ID: BASE-CHANGE-20261103-3
Date: 2026-11-03
Summary: Revise host API data plane for agent-primary client — replace raw TCP
  :9000 with HTTP JSON stream write/read (Base64, bounded timeouts).
Pillars affected:
  - External host communication (pillar 3)
Affected base docs:
  - docs/external_host_api_architecture.md
  - docs/external_host_stream_api.md
  - docs/external_host_command_api.md
  - docs/hil_base_platform_scope.md
  - docs/hil_base_reuse_contract.md
  - docs/architecture.md
  - docs/hil_board_particular_spec_template.md
  - docs/board_host_api_extensions.md
Required particular-repo actions:
  - Re-read stream API; drop any assumption of TCP 9000 / length-prefix framing.
  - Keep board_host_api_extensions.md aligned with open/close/write/read +
    Base64 frame limits.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none yet (docs only; firmware still pending).
```

### BASE-CHANGE-20261103-4

```text
ID: BASE-CHANGE-20261103-4
Date: 2026-11-03
Summary: Mandatory capabilities (ask-the-board manifest); b00_basehil reference
  demos echo command + loopback stream.
Pillars affected:
  - External host communication (pillar 3)
Affected base docs:
  - docs/external_host_api_architecture.md
  - docs/external_host_command_api.md
  - docs/external_host_stream_api.md
  - docs/hil_base_platform_scope.md
  - docs/architecture.md
  - docs/hil_board_particular_spec_template.md
  - docs/board_host_api_extensions.md
Required particular-repo actions:
  - Implement or stub capabilities in firmware when adopting host API; agents
    must not rely on reading repo Markdown for discovery.
  - Do not assume echo/loopback exist unless listed in that board's on-device
    capabilities (they are b00_basehil demos, not family-mandatory).
  - Update board_host_api_extensions.md checklist to match firmware manifest.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: none yet (docs only; firmware pending
  HOST_API_CAPABILITIES_ECHO_LOOPBACK).
```

### BASE-CHANGE-20261103-5

```text
ID: BASE-CHANGE-20261103-5
Date: 2026-11-03
Summary: Fix host API bring-up after POST provision — port 80 exclusivity;
  emit WIFI_PROV_EVENT_PORTAL_TEARDOWN_COMPLETE; start host_api after SoftAP
  httpd stop (Run 029 / TESTER_RETURN_HOST_API_AFTER_PROVISION).
Pillars affected:
  - External host communication (pillar 3)
  - WiFi onboarding (portal teardown signal)
Affected base docs:
  - docs/external_host_api_architecture.md
  - docs/wifi_provisioning_architecture.md
Required particular-repo actions:
  - When adopting host API + SoftAP portal on the same port 80 pattern, absorb
    PORTAL_TEARDOWN_COMPLETE → host_api_start ordering.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: pending implementer HOST_API_AFTER_PORTAL_TEARDOWN.
```

### BASE-CHANGE-20261103-6

```text
ID: BASE-CHANGE-20261103-6
Date: 2026-11-03
Summary: Document b00 flash/partition baseline — 4MB flash, custom
  partitions.csv with large single factory app (0x3F0000), no dual-OTA yet.
Pillars affected:
  - None directly (build/hardware support contract)
Affected base docs:
  - docs/flash_partition_architecture.md (new)
  - docs/architecture.md
  - docs/board_hardware_description.md
  - docs/hil_base_platform_scope.md
  - docs/hil_base_reuse_contract.md
Required particular-repo actions:
  - Do not assume 1MB factory or 2MB flash from older base builds.
  - If reusing this tree's partitions.csv, confirm the board module is 4MB
    (or adapt the table) and note NVS remains at 0x9000.
  - Record this ID as consumed in the board sync log.
Code impact in base tree: partitions.csv + sdkconfig.defaults already set;
  validate with fullclean/build outside architect role.
```
