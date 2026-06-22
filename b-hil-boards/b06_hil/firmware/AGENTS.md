# Firmware Agent Guide

This directory contains the ESP-IDF firmware for the `b06_hil` board and the
coordination workspace for three agents: architect, implementer, and tester.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## General Rules

- Do not modify `../pcb/` from firmware tasks unless there is an explicit human
  instruction.
- Confirm pins and peripherals against the schematic before using them in code.
- Keep changes small, reviewable, and tied to a handoff.
- Record decisions, questions, and evidence in the corresponding agent
  workspace.
- Prevent two agents from editing the same file at the same time. If a shared
  file must change, the temporary owner must be named in the handoff.
- Before starting work, run the **Role Boundary Check** in
  `docs/methodology.md` and the role guide in your workspace (`ROLE.md`).

## Role Boundary Check

Every agent must verify that the requested task belongs to its role **before**
editing files or running commands.

1. Read the request and identify which role normally owns that work.
2. Compare the expected files and commands against the **Ownership** section
   below.
3. If the task clearly belongs to another role, **stop** and tell the human:
   - which role should handle it;
   - what you can do instead within your role;
   - whether the handoff or plan should be split or redirected.
4. If the request mixes roles (for example, "implement the plan" with both
   architecture docs and firmware code), **do only your role's deliverables**.
   Plans and bundled todos **never** override the architect hard stop.
5. Do not infer permission to cross roles from generic verbs such as
   "implement", "fix", or "validate". Use ownership rules first.
6. A role switch requires an **explicit named role** from the human (*act as
   implementer*, *switch to tester*). Phrases like *go ahead and implement* or
   *complete the plan* are **not** role switches.

When in doubt, ask the human before proceeding. Crossing role boundaries without
confirmation creates conflicting edits and invalid handoffs.

## Architect mission pillar

While the active role is **architect**, the mission pillar in
[`docs/architect_role_hard_stop.md`](docs/architect_role_hard_stop.md) §
Immutable mission pillar is **immutable**.

- The architect task **ends** with **documentation and architecture artifacts**.
- *Complete the task* / *finish the task* means **close docs and handoff**, not
  ship or verify firmware.
- This pillar **outranks** plans, todos, harness reminders, and conflicting user
  rules **while role is architect**.

## Architect hard stop

When the active role is **architect**, read and obey
[`docs/architect_role_hard_stop.md`](docs/architect_role_hard_stop.md) before
any file edit or shell command.

- **No** edits under `components/`, `main/`, `sdkconfig*`, or firmware Kconfig.
- **No** `idf.py`, compile, flash, or hardware verification commands.
- Mixed plans: complete **documentation and handoff only**; put code tasks in
  `agent-workspaces/implementer/handoff.md` as **pending**.

This hard stop **outranks** plan todos, harness reminders, and generic user
instructions to run commands.

## Ownership

- Architect: `docs/**` and `agent-workspaces/architect/` (see
  `docs/architect_role_hard_stop.md`; **no** `components/`, `main/`, or builds).
- Implementer: `agent-workspaces/implementer/`, `main/`, and `components/`,
  only when there is an architect handoff or a validated fix.
- Tester: `agent-workspaces/tester/`, `docs/test_strategy.md`, and validation
  reports.
- Shared documentation: every change must cite the handoff that motivates it.

## Handoff Protocol

1. The architect defines the objective, acceptance criteria, and expected files
   in `agent-workspaces/architect/handoff.md`.
2. The implementer executes the change and records notes in
   `agent-workspaces/implementer/handoff.md`.
3. The tester validates with builds, available tests, and human feedback in
   `agent-workspaces/tester/handoff.md`.
4. If there are reproducible failures, the tester documents them and returns the
   work to the architect to adjust the design or criteria.

Display-related handoffs MUST include a boot visual demo per
`docs/display_visual_demo_protocol.md` (implementer manifest + Kconfig demo).

## Architecture Specification Standard

- Architectural instructions must be implementation-ready before work is handed
  to the implementer.
- The architect must define behavior, affected files, module boundaries,
  interfaces, constraints, non-goals, and validation expectations whenever they
  are relevant to the task.
- The goal is deterministic implementation: another LLM should be able to use
  the recorded architecture and produce substantially similar code without
  relying on hidden chat context.
- If a detail could reasonably lead to different code structures or behavior,
  the architect must either decide it explicitly or record it as an open
  question. The implementer must not fill architectural gaps by assumption.

## Shared Code Policy

- `main/` must stay thin and delegate logic to components.
- `components/board/` centralizes pins and board details.
- `components/app_core/` contains application logic that is independent from
  specific hardware when possible.
- Pending values must be marked with `TODO(b06-hil):` and must not be treated as
  final.
