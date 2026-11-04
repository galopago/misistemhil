# Flash and Partition Architecture — `b00_basehil`

Normative decision for **on-device flash sizing and the ESP-IDF partition
table** used by this base firmware tree.

Source handoff: `agent-workspaces/architect/handoff.md`,
`FLASH_4MB_LARGE_FACTORY`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Status

**Normative for `b00_basehil` reference builds.** Sibling boards MAY choose
different flash sizes and tables; they MUST document their own layout in their
hardware / build docs and MUST NOT assume the old 1 MB factory default from
early base experiments.

## Decision (2026-11-03)

| Item | Value |
| --- | --- |
| Assumed module flash | **4 MB** (ESP32-C3 SuperMini reference used on this bench) |
| `CONFIG_ESPTOOLPY_FLASHSIZE` | **4MB** |
| Partition scheme | **Custom** `partitions.csv` (single factory app, **no OTA**) |
| `nvs` | offset `0x9000`, size `0x6000` (unchanged; matches erase procedures) |
| `phy_init` | offset `0xf000`, size `0x1000` |
| `factory` (app) | offset `0x10000`, size **`0x3F0000`** (~4032 KiB) |

Authoritative files in this tree:

- [`partitions.csv`](../partitions.csv)
- [`sdkconfig.defaults`](../sdkconfig.defaults) (flash size + custom table)

## Why

Early builds used IDF default **Single factory app** (`factory` = **1 MB**) with
sdkconfig flash **2 MB**. The application (WiFi + HTTP portal + mDNS + OLED +
host API) already occupied most of that 1 MB partition (~9 % free), while the
physical module has **4 MB**. The build warning compared **app vs app
partition**, not vs total chip flash.

Growing the **factory** partition (and declaring **4 MB** flash) is the
lowest-risk way to keep headroom for family base features without enabling
dual-OTA yet.

## Non-goals (for now)

- Dual OTA slots (`partitions_two_ota*`) — halves usable app size on 4 MB; defer
  until an explicit OTA product requirement exists.
- Changing NVS offset/size (would break documented `erase_region 0x9000 0x6000`
  procedures unless those docs are updated in the same change).
- Claiming every `bNN_hil` board has 4 MB flash (particular hardware may differ).

## Operational notes

- After switching to this table, a **fullclean / reconfigure** and flash are
  required; a one-time **erase-flash** (or at least rewriting the partition
  table) may be needed on devices that still have the old 1 MB layout.
- Tester NVS erase helpers that use `0x9000` / `0x6000` remain valid.
- IDF preset `partitions_singleapp_large.csv` (factory **1500 K**) was
  **not** chosen: too little headroom relative to a 4 MB module once host API
  and further base features land.

## Future OTA

If OTA is required later, redesign the partition table deliberately (two app
slots + sizes that still fit a budgeted max app image), update this document,
and add a `BASE-CHANGE-…` entry. Do not silently re-enable two-OTA defaults.

## Related

- Hardware module: `docs/board_hardware_description.md`
- Factory-reset / NVS erase: `docs/wifi_factory_reset_architecture.md`,
  `agent-workspaces/tester/procedures.md`
- Overview pointer: `docs/architecture.md`
