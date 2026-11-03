# Board Functional Description — `b00_basehil`

Particular product/functional role for **this** repository only. Other family
boards replace this file with their own mission (RS-485, relays, logic
analyzer, etc.) while keeping the same documentation philosophy.

Source handoff: `agent-workspaces/architect/handoff.md`, `HIL_BASE_REUSE_CONTRACT`
(particular filled example for the base tree).

Template: `docs/hil_board_particular_spec_template.md`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b00_basehil
Board number: 00
One-line role:
  Shared HIL family base firmware and documentation reference for human-
  supervised minimum bring-up tests.
```

## Functional mission

`b00_basehil` is **not** a product HIL specialty card (it is not an RS-485
front-end, relay card, or logic analyzer). It is the **family base example**:

- Own and evolve the shared platform pillars: WiFi onboarding, network board
  detection, and external host communication methodology.
- Provide a flashable reference implementation that proves those pillars on a
  **minimum hardware bench**.
- Support **minimum tests supervised by a human** (operator confirms OLED,
  SoftAP/QR, portal, LED patterns, factory reset, mDNS, etc.).
- Serve as the contracts-first source that LLM agents on `bNN_hil` boards read
  and adapt.

## In scope for this board

- Reference WiFi provisioning SoftAP/portal, credential NVS, STA connect /
  failure / reconnect / factory-reset behavior.
- Reference OLED provisioning/status UX and error-LED WiFi link patterns.
- Reference LAN identity / mDNS hostname `HIL-00-<MAC4>`.
- Reserved family host API contract (detail may still be pending in
  `docs/external_host_api_architecture.md`).
- Agentic methodology and base vs particular documentation model.
- Minimum I2C bench device for shared display/bus paths: OLED only on this
  test profile.

## Out of scope for this board

- INA219 (or other extra I2C sensors) on the minimum human-supervised test
  bench; specialty boards document their own I2C devices separately.
- Product-specific HIL channels unique to a specialty card (RS-485 framing,
  relay matrices, high-speed logic capture, etc.).
- Claiming this tree is a packaged library consumed via submodule.
- Replacing human-supervised acceptance with fully automated lab certification
  (tester + operator remain part of the workflow).

## Relationship to base pillars

- This repo **is** the base: pillars live in the normative `docs/` contracts and
  the reference firmware here.
- Uses base WiFi onboarding / discovery / host API: **yes** (authoritative).
- Board-specific host API extensions: **none** for v1 beyond the family host
  API home (`docs/board_host_api_extensions.md`).

## Non-goals

- Do not redefine family WiFi, discovery, or host protocol inside this file.
- Do not place pin numbers or MCU part numbers here (see
  `docs/board_hardware_description.md`).
- Do not describe other boards' product missions here.
