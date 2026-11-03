# Board Base Sync Log — `b00_basehil`

Append-only log of consumed `BASE-CHANGE-…` IDs for this tree's particular docs.

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261102-1, BASE-CHANGE-20261102-2, BASE-CHANGE-20261102-3
Date consumed: 2026-11-02
Agent role: architect
Actions taken in this repo:
  - files modified / created:
    - docs/board_functional_description.md
    - docs/board_hardware_description.md
    - docs/board_particular_scope.md
    - docs/board_host_api_extensions.md
    - docs/board_base_sync_log.md
  - code adapted from base (if any): none (docs only)
  - deferred (with reason): none
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: (doc-only particular correction; no new BASE-CHANGE)
Date consumed: 2026-11-02
Agent role: architect
Actions taken in this repo:
  - Removed INA219 from docs/board_hardware_description.md and
    docs/board_functional_description.md for the minimum test bench profile.
  - Drivers and board_pins.h left unchanged (other boards may still use INA219
    or other I2C devices in their particular hardware docs).
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-1
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Clarified I2C ownership: shared bus is base; extra slaves (INA219, etc.)
    are daughter-board particular. Updated connections, i2c_bus architecture,
    platform scope, architecture, reuse contract, particular template.
  - No driver or board_pins.h changes.
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-2
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Published external host API v1 concept (overview + command + stream docs).
  - Updated board_host_api_extensions.md and family pointers.
  - Firmware for host API left pending for implementer.
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-3
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Revised host stream plane to agent-primary HTTP write/read (dropped TCP 9000).
  - Updated family pointers, extensions compatibility line, particular scope.
  - Firmware still pending EXTERNAL_HOST_API_V1_CONCEPT.
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-4
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Documented mandatory capabilities (ask-the-board) and b00 demos echo/loopback.
  - Updated board_host_api_extensions.md checklist to match planned manifest.
  - Firmware still pending HOST_API_CAPABILITIES_ECHO_LOOPBACK.
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-5
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Documented port-80 exclusivity and PORTAL_TEARDOWN_COMPLETE → host_api_start.
  - Closed TESTER_RETURN_HOST_API_AFTER_PROVISION as HOST_API_AFTER_PORTAL_TEARDOWN.
  - Firmware pending implementer.
Open questions:
  - None
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-6
Date consumed: 2026-11-03
Agent role: architect
Actions taken in this repo:
  - Added docs/flash_partition_architecture.md; linked from architecture,
    hardware description, platform scope, reuse contract.
  - Recorded FLASH_4MB_LARGE_FACTORY decision (4MB + large factory, no OTA).
Open questions:
  - None
```
