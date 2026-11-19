# b02_hil Firmware

ESP-IDF firmware tree for `b02_hil` (HIL board number `02`).

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Relationship to `b00_basehil`

This tree starts from a **local working copy** of the shared HIL base
architecture, philosophy, and methodology docs (copied from
`b00_basehil/firmware/docs` and related guides on 2026-11-17).

**No live dependency** on the base repository:

- Do **not** use git submodules, nested repos, or CMake
  `EXTRA_COMPONENT_DIRS` pointing at `b00_basehil`.
- Firmware source (`main/`, `components/`, build config) is added by the
  **implementer** as an **adapt-copy** owned by this tree (see
  `agent-workspaces/implementer/handoff.md`, `B02_FIRMWARE_ADAPT_COPY`).
- After bootstrap, this board evolves **independently**. Optional re-sync with
  family contracts uses `docs/hil_base_spec_changelog.md` +
  `docs/board_base_sync_log.md`.

See:

- `docs/hil_base_platform_scope.md`
- `docs/hil_base_reuse_contract.md`
- `docs/architecture.md`
- `AGENTS.md`
- `docs/board_particular_scope.md`

## Structure

- `docs/`: family platform contracts (working copy) + board-particular specs.
- Particular bring-up: `docs/board_gpio_bringup_architecture.md` (boot-safe
  `/KOE` + relay/DE cold init; same hazard class as `b03_hil`).
- `agent-workspaces/`: architect / implementer / tester role guides and handoffs.
- `tests/`: test documentation.
- Firmware `main/` / `components/` appear after implementer adapt-copy.

## Board identity

- Board number: **`02`**
- SoftAP / mDNS hostname form: `HIL-02-<MAC4>`
- Hardware authority: `../pcb/b02.kicad_pcb` via
  `docs/board_hardware_description.md`

## Requirements (when implementing / testing)

- ESP-IDF installed on the development computer.
- ESP-IDF environment variables are not created globally and must not be assumed
  to exist.
- Confirm MCU target and every pin against `../pcb/` before coding board
  constants. Do not assume `b00_basehil` GPIO numbers apply beyond what this
  board's hardware description records.
