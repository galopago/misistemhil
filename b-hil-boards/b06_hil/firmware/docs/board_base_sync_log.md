# Board Base Sync Log — `b06_hil`

Append-only log of consumed `BASE-CHANGE-…` IDs for this tree's particular docs.

Working copies of base contracts were imported from `b00_basehil/firmware/docs`
on 2026-11-03 (documentation only; no firmware code).

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261102-1, BASE-CHANGE-20261102-2, BASE-CHANGE-20261102-3,
  BASE-CHANGE-20261103-1, BASE-CHANGE-20261103-2, BASE-CHANGE-20261103-3,
  BASE-CHANGE-20261103-4, BASE-CHANGE-20261103-5
Date consumed: 2026-11-03
Agent role: (docs import for b06_hil bring-up)
Actions taken in this repo:
  - Copied base architecture / methodology docs into firmware/docs/
  - Adapted AGENTS.md and README.md for b06_hil
  - Created stub particular files:
    - docs/board_functional_description.md
    - docs/board_hardware_description.md
    - docs/board_particular_scope.md
    - docs/board_host_api_extensions.md
    - docs/board_base_sync_log.md
  - code adapted from base (if any): none (docs only; no components/main)
  - deferred (with reason): fill particular functional/hardware content when
    human supplies b06 particularities; confirm pins against ../pcb/
Open questions:
  - Board functional mission (TBD)
  - MCU / pin map / peripherals (TBD from schematic + human)
```

## Sync entry

```text
Consumed IDs: (particular fill from PCB; no new BASE-CHANGE)
Date consumed: 2026-11-03
Agent role: architect (docs)
Actions taken in this repo:
  - Filled docs/board_functional_description.md — passive USB 5V 1→3 splitter
    with per-output INA219 metering
  - Filled docs/board_hardware_description.md from ../pcb KiCad 7 schematic/PCB
    (MCU GPIO map, J1–J5, U1–U5, silk OUT↔I2C address table, J5 pinout)
  - Updated docs/board_particular_scope.md and docs/board_host_api_extensions.md
  - Recorded open questions: shunt/BOM values, OLED address, JP1 default,
    host API command names
  - code adapted from base (if any): none
Open questions:
  - Shunt resistor ohms / polyfuse ratings (schematic Values unset)
  - OLED I2C address on hardware
  - JP1 production default (open vs closed)
  - Frozen host API names for channel telemetry
```

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261103-6
Date consumed: 2026-11-03
Agent role: architect (docs only; no compile)
Actions taken in this repo:
  - Imported docs/flash_partition_architecture.md from b00_basehil
  - Updated docs/architecture.md, hil_base_platform_scope.md,
    hil_base_reuse_contract.md, hil_base_spec_changelog.md
  - Documented 4MB flash + base partition adoption in
    docs/board_hardware_description.md
  - Bumped docs/board_particular_scope.md last synced ID to
    BASE-CHANGE-20261103-6
  - code adapted from base (if any): none in this sync (architect hard stop —
    partitions.csv / sdkconfig.defaults left for implementer)
  - deferred (with reason): create firmware/partitions.csv and set
    CONFIG_ESPTOOLPY_FLASHSIZE_4MB + custom partition table in
    sdkconfig.defaults when acting as implementer
Open questions:
  - Confirm physical SuperMini flash on built b06 units is 4MB (expected)
```
