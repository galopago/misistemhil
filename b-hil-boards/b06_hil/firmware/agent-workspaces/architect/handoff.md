# Architect Handoff — `b06_hil`

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting any task in this file, run the **Role Boundary Check**:

- Read `ROLE.md` in this folder, `../../AGENTS.md`, and `../../docs/methodology.md`.
- Confirm the request is **architecture and documentation**, not firmware
  implementation, builds, or formal validation.

## Current Status

```text
Board: b06_hil (06)
Docs import from b00_basehil: done (2026-11-03)
Particular functional/hardware content: filled from PCB (2026-11-03)
Last synced BASE-CHANGE: BASE-CHANGE-20261103-6 (4MB flash / large factory)
B06_MAIN_METERING_OLED: docs + local implementer firmware (tester pending)
B06_HOST_API_METERING: docs + local implementer source (tester pending)
Pending implementer (build config): partitions.csv + sdkconfig.defaults 4MB
```

## Active handoff: `B06_HOST_API_METERING`

### Objective

Freeze particular remote metering commands on the family host API so an LLM
agent can read silk OUT 1..3 voltage, current, and power after
`ping` → `info` → `capabilities`.

### Normative contract

`docs/board_host_api_metering_architecture.md`

### Related docs

- `docs/board_host_api_extensions.md` (frozen checklist)
- `docs/external_host_api_architecture.md`
- `docs/external_host_command_api.md`
- `docs/board_hardware_description.md`
- `docs/board_main_metering_oled_architecture.md` (shared INA219; separate UX)

### Frozen surface

| Command | Args | Result |
| --- | --- | --- |
| `meter_read` | `{ "channel": 1\|2\|3 }` | one channel object |
| `meter_read_all` | `{}` | `{ "channels": [3 objects] }` |

Units: `voltage_v`, `current_a` (signed), `power_w = V*I`.  
Streams: none. No `echo`/`loopback` on this board.

### Constraints

- Architect deliverable is documentation only; no `components/` / `main/` edits
  in this handoff closeout.
- Do not change family `/v1` paths or mandatory `ping` / `info` / `capabilities`.
- Channel index = silkscreen only.
- Host samples MUST be fresh I2C reads, serialized with OLED metering loop.
- `host_api` MUST NOT drive I2C; `ina219` MUST NOT include host API headers.

### Acceptance criteria

See § Acceptance criteria in
`docs/board_host_api_metering_architecture.md` (items 1–9).

### Open questions

1. BOM shunt ohms (provisional `0.1f`) — accuracy only.
2. Future metering stream — not authorized.

### Implementer package

Pending tasks in `../implementer/handoff.md` under `B06_HOST_API_METERING`.

## Prior handoff (still relevant)

`B06_MAIN_METERING_OLED` — OLED main screen + INA219 loop; see
`docs/board_main_metering_oled_architecture.md`.
