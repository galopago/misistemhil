# External Host API Architecture

Normative home for the **family-wide communication methodology** between an
external entity (LLM agent, application, or test harness) and each HIL board.

Source handoff: `agent-workspaces/architect/handoff.md`, `HIL_BASE_REUSE_CONTRACT`
(extension: platform pillars and particular separation).

Pillar ownership: **Base pillar 3** in `docs/hil_base_platform_scope.md`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Status

**Reserved base contract — behavioral detail pending.**

This file is the authoritative location for the host↔board API/protocol once
specified. Until request/response schemas, transport binding, and versioning
are filled by a future architect handoff in `b00_basehil`:

- Derivative boards MUST NOT invent a competing family-wide host protocol
  document.
- Derivative boards MAY draft **board-only extensions** under
  `board_host_api_extensions.md` in their own repo, but those extensions remain
  provisional until this base contract defines the shared surface they extend.
- Implementers MUST NOT treat informal chat descriptions as the host API.

When detail is added, append a `BASE-CHANGE-…` entry to
`docs/hil_base_spec_changelog.md`.

## Mission

After WiFi onboarding and network detection succeed, an external host must be
able to address a board (for example via its discovery identity) and exchange
commands/data using a **single family methodology** owned by this base tree.

Board-particular product features (extra sensors, HIL channels, unique
commands) are documented and implemented in each `bNN_hil` repo as **extensions**
of this contract, not as replacements.

## Boundaries

| In this base doc | Not in this base doc |
| --- | --- |
| Transport and protocol choice for family host access | Board pin maps |
| Shared API surface, versioning, auth (if any), error model | Board-unique command semantics beyond declared extensions |
| How hosts locate the API relative to discovery identity | PCB-specific electrical behavior |
| Compatibility rules for particular extensions | Per-board validation evidence |

Related base pillars:

- WiFi onboarding: `docs/wifi_provisioning_architecture.md` and related docs
- Network detection: `docs/device_discovery_mdns_architecture.md`

## Normative rules already in force

1. This file is the only family-wide normative host API document.
2. Particular repos record extensions only in their own
   `board_host_api_extensions.md` (see
   `docs/hil_board_particular_spec_template.md`).
3. Extensions MUST NOT break or shadow mandatory base host behavior once that
   behavior is specified here.
4. Discovery hostname format and WiFi onboarding remain separate pillars; the
   host API consumes their outcomes (board online on LAN with known identity)
   rather than redefining them.

## Pending architect decisions (open)

Recorded so implementers do not guess:

- Transport (for example HTTP on STA, other) and port/path conventions
- Message schema and versioning strategy
- Authentication/authorization profile for v1 (if any)
- Mapping from discovery identity to host endpoint
- Which operations are mandatory for every family board versus optional
  particular extensions

These open items block a complete implementation-ready host API but do **not**
block documenting pillar ownership or particular-file separation.

## Non-Goals (until a future handoff expands them)

- Defining board-specific HIL measurement protocols inside this file
- Replacing mDNS discovery with a custom host-only discovery mechanism
- Coupling host API headers into display or WiFi provisioning internals without
  an explicit base architecture decision
