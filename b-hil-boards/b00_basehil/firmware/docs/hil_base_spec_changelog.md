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
