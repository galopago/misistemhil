# HIL Board Particular Specification Template

Template and rules for **board-owned** documentation in each `bNN_hil`
repository. Particular specs MUST live in **different files** (and the board's
own repo) from base pillar docs in `b00_basehil`.

Source handoff: `agent-workspaces/architect/handoff.md`, `HIL_BASE_REUSE_CONTRACT`
(extension: platform pillars and particular separation).

Base delimiters:

- `docs/hil_base_platform_scope.md`
- `docs/hil_base_reuse_contract.md`
- `docs/hil_base_spec_changelog.md`

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Intent

Keep board-unique behavior easy to find and keep base changes easy to merge:

- Base docs describe family platform (WiFi onboarding, discovery, host API).
- Particular docs describe the **board's product role**, **hardware**, and only
  deltas relative to base pillars.
- When base changes, the LLM reads `hil_base_spec_changelog.md` and updates
  particular files + sync log — without forking base contracts.

Two particular concerns **always** vary by board and MUST be separate files
from each other and from base pillars:

1. **Functional description** — what the board is for (for example RS-485
   interface, relay card, logic analyzer, power monitor).
2. **Hardware description** — MCU/module choice, peripherals, and pin map
   (processor and pinout MAY differ from `b00_basehil`).

## Recommended files in a particular board repo

Create these under the board firmware tree (paths may be adjusted, names SHOULD
stay recognizable):

| File | Purpose | Required? |
| --- | --- | --- |
| `docs/board_functional_description.md` | Product/functional role of this board | **Yes** |
| `docs/board_hardware_description.md` | MCU, modules, pin→function map, power | **Yes** |
| `docs/board_particular_scope.md` | What this board adds beyond base; non-goals | **Yes** |
| `docs/board_product_architecture.md` | Deeper HIL channels, UX, state machines | As needed |
| `docs/board_host_api_extensions.md` | Extensions to base host API only | Yes (may state “none”) |
| `docs/board_base_sync_log.md` | Consumed `BASE-CHANGE-…` IDs and local notes | **Yes** |

Do **not** merge functional and hardware descriptions into one file when both
are non-trivial: an LLM must be able to update pin maps without rewriting the
product mission, and vice versa.

Do **not** copy full base pillar documents into the board repo as a maintained
fork. Link/cite base paths instead.

Do **not** assume the base MCU (ESP32-C3 SuperMini in `b00_basehil`) or base
GPIO numbers apply. Hardware is board-owned.

**Filled example in this repository:** `b00_basehil` itself carries concrete
particular files (`docs/board_functional_description.md`,
`docs/board_hardware_description.md`, and siblings). When starting a new
`bNN_hil` tree, **copy the file set and philosophy**, then rewrite the content
for that board's mission and hardware — do not keep the base bench story or
GPIO map unchanged.

## Required sections: `board_functional_description.md`

Normative **product intent** for this board only. Examples of board classes:
RS-485 / industrial bus interface, relay output card, logic analyzer, multi-
channel current sense, mixed I/O HIL front-end.

```text
Board ID: bNN_hil
Board number: NN
One-line role:
  (e.g. "Galvanically isolated RS-485 HIL interface for DUT serial traffic.")

Functional mission:
  - What the board does in the HIL system
  - Who/what it connects to (DUT, host, other HIL boards)
  - Primary user-visible capabilities

In scope for this board:
  - …

Out of scope for this board:
  - … (including features that belong to other family boards)

Relationship to base pillars:
  - Uses base WiFi onboarding / discovery / host API: yes/partial (cite)
  - Board-specific functions exposed via host API extensions: cite
    board_host_api_extensions.md

Non-goals:
  - Do not redefine family WiFi, discovery, or base host protocol here
  - Do not place pin numbers or MCU part numbers here (those belong in
    board_hardware_description.md)
```

## Required sections: `board_hardware_description.md`

Normative **hardware profile** for this board. Processor, modules, and pin
routing MAY differ from `b00_basehil` and from every other `bNN_hil`.

```text
Board ID: bNN_hil
Schematic / PCB references:
  - path or doc ID for authoritative schematic
MCU / module:
  - Part / module name (e.g. ESP32-C3 SuperMini, other SoC, other module)
  - Target toolchain profile (e.g. esp32c3) if applicable
  - Clock, flash, PSRAM notes as needed
On-board peripherals (non-exhaustive categories):
  - Display, LEDs, buttons/reset, buses (I2C/SPI/UART/RS-485), relays, ADCs, …
  - Extra I2C product devices beyond base OLED (e.g. INA219 @ addresses …),
    if any — base test profile does not require them
Power:
  - Input rails, regulation, isolation notes
Pin map (schematic-confirmed; mark pending with TODO(bNN-hil): ):
  | Signal name | MCU pin / pad | Direction | Notes |
  | --- | --- | --- | --- |
  | … | … | … | … |
Differences from b00_basehil hardware profile:
  - … (MCU change, missing OLED, extra transceiver, different reset GPIO, …)
Firmware board ownership:
  - components/board (or equivalent) owns these constants; do not scatter pins
```

Rules:

- Pin values MUST be confirmed against **this board's** schematic before they
  are treated as final.
- Pending pins use `TODO(bNN-hil):` (or the board's agreed TODO tag), not silent
  guesses.
- Base file `docs/esp32_c3_supermini_connections.md` is **not** normative for
  other boards; it only describes the base hardware profile.

## Required sections: `board_particular_scope.md`

```text
Board ID: bNN_hil
Board number: NN
Base tree referenced: b00_basehil/firmware
Last synced BASE-CHANGE ID: (from board_base_sync_log.md)

Pointers (required):
  - Functional description: docs/board_functional_description.md
  - Hardware description: docs/board_hardware_description.md

Base pillars consumed as-is (cite base docs):
  - WiFi onboarding: …
  - Network detection: …
  - External host API: …

Particular additions (this repo only):
  - …

Explicit non-goals for this board:
  - …
```

## Required sections: `board_host_api_extensions.md`

```text
Base contracts:
  - b00_basehil/firmware/docs/external_host_api_architecture.md
  - b00_basehil/firmware/docs/external_host_command_api.md
  - b00_basehil/firmware/docs/external_host_stream_api.md
Compatibility: must not change /v1 paths, Base64 frame limits, or mandatory
  ping / info / capabilities
Mandatory base commands on this board: ping, info, capabilities (inherited)
Note: agents discover commands/streams via on-device capabilities, not this file.
Extra commands:
  - name:
    Purpose:
    Args:
    Result:
    Errors:
Streams:
  - stream_id: (or "none")
    Purpose:
    params schema:
    Payload semantics (host→board / board→host):
    Hardware backend:
Non-extensions (forbidden):
  - Replacing base discovery, WiFi onboarding, or host transport/framing
  - Shadow APIs that duplicate base host semantics under new names without cause
  - Expecting agents to read repo Markdown instead of POST capabilities
```

If the board has no extra commands and no streams, state that explicitly
(`Extra commands: none`, `Streams: none`). Firmware `capabilities` MUST match.

## Required sections: `board_base_sync_log.md`

Append-only. Newest entry first or last is acceptable if order is consistent;
prefer chronological ascending.

```text
## Sync entry

Consumed IDs: BASE-CHANGE-YYYYMMDD-N[, …]
Date consumed:
Agent role:
Actions taken in this repo:
  - files modified:
  - code adapted from base (if any):
  - deferred (with reason):
Open questions:
```

## LLM checklist when base changes

1. Read new entries in `b00_basehil/firmware/docs/hil_base_spec_changelog.md`
   after the last consumed ID.
2. For each ID, open the listed affected **base** docs and understand the delta.
3. Update only **particular** files in this board repo (functional description,
   hardware description, scope, product, API extensions, sync log).
4. If base host API gained mandatory behavior, update board firmware and
   `board_host_api_extensions.md` so extensions remain compatible.
5. If a hardware or MCU assumption in base support docs does not apply, record
   the difference in `board_hardware_description.md`; do not rewrite base docs.
6. If a change cannot be applied, record it under deferred with a concrete
   reason; do not silently ignore a changelog ID.

## Non-Goals

- Maintaining a full duplicate of base `docs/` inside the board repo.
- Editing base normative files from a board-only task without a base architect
  handoff.
- Treating pin or MCU differences as a reason to fork WiFi/discovery/host API
  contracts.
- Putting functional mission and pin maps in the same particular file when both
  need independent revision.
