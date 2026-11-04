# b03_hil Firmware

ESP-IDF firmware tree for `b03_hil` (HIL board number `03`).

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Relationship to `b00_basehil`

This tree starts from a **local working copy** of the shared HIL base
architecture, philosophy, and methodology docs (copied from
`b00_basehil/firmware/docs` and related guides). **No base firmware source**
(`components/`, `main/`, build files) was copied.

- **Authoritative family contracts** remain in `b00_basehil` when the family
  base moves; absorb deltas via `docs/hil_base_spec_changelog.md` and record
  them in `docs/board_base_sync_log.md`.
- **Board-particular** files under `docs/board_*.md` are owned by this repo and
  must describe `b03_hil`, not the base bench. Several are stubs pending
  human-supplied particularities.

See:

- `docs/hil_base_platform_scope.md`
- `docs/hil_base_reuse_contract.md`
- `docs/architecture.md`
- `AGENTS.md`

## Structure (docs-first → implementer)

- `docs/`: base platform contracts (working copy) + board-particular specs.
- Particular implementation-ready architecture:
  - `docs/board_product_architecture.md`
  - `docs/board_relay_control_architecture.md`
  - `docs/board_host_api_relay_architecture.md`
- `agent-workspaces/`: architect / implementer / tester role guides.
- `tests/`: test documentation (no code yet).
- Firmware `main/` / `components/` are added by the **implementer** from the
  handoff in `agent-workspaces/implementer/handoff.md`.

## Requirements (when implementing)

- ESP-IDF installed on the development computer.
- ESP-IDF environment variables are not created globally and must not be assumed
  to exist.
- Confirm MCU target and every pin against `../pcb/` before coding board
  constants. Do not assume `b00_basehil` GPIO numbers apply.

## Hardware Note

Do not assume final pins from documentation copied from the base. Every pin
must be confirmed against the schematic in `../pcb/` and recorded in
`docs/board_hardware_description.md`.
