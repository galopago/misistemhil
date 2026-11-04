# Board Base Sync Log — `b03_hil`

Append-only log of consumed `BASE-CHANGE-…` IDs for this tree's particular docs.

Working copies of base contracts were imported from `b00_basehil/firmware/docs`
on 2026-11-03 (documentation only; no firmware code).

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261102-1, BASE-CHANGE-20261102-2, BASE-CHANGE-20261102-3,
  BASE-CHANGE-20261103-1, BASE-CHANGE-20261103-2, BASE-CHANGE-20261103-3,
  BASE-CHANGE-20261103-4, BASE-CHANGE-20261103-5, BASE-CHANGE-20261103-6
Date consumed: 2026-11-03
Agent role: (docs import for b03_hil bring-up)
Actions taken in this repo:
  - Copied base architecture / methodology docs into firmware/docs/
  - Adapted AGENTS.md and README.md for b03_hil
  - Created stub particular files:
    - docs/board_functional_description.md
    - docs/board_hardware_description.md
    - docs/board_particular_scope.md
    - docs/board_host_api_extensions.md
    - docs/board_base_sync_log.md
  - Seeded agent-workspaces/ (architect, implementer, tester) with ROLE.md
  - code adapted from base (if any): none (docs only; no components/main)
  - deferred (with reason): fill particular functional/hardware content when
    human supplies b03 particularities; confirm pins against ../pcb/
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
  - Filled docs/board_functional_description.md — six-channel relay HIL board
  - Filled docs/board_hardware_description.md from ../pcb/b03.kicad_pcb
    pinfunction/net map (MCU GPIOs, 74HCT541 /KOE, ULN2003, K1–K6, J1/J2)
  - Documented normative boot OE sequence (all KnCTR low, then /KOE low)
  - Updated docs/board_particular_scope.md and docs/board_host_api_extensions.md
  - Noted schematic label placement can disagree with PCB; PCB is authoritative
  - code adapted from base (if any): none
Open questions:
  - OLED I2C address; flash size on units; BOM R/C values; G6K coil rating
  - Strapping behavior on GPIO2/8/9
  - Frozen host API names for relay get/set
```

## Sync entry

```text
Consumed IDs: (particular architecture freeze; no new BASE-CHANGE)
Date consumed: 2026-11-03
Agent role: architect (docs)
Actions taken in this repo:
  - Added docs/board_product_architecture.md (B03_PRODUCT_BRINGUP)
  - Added docs/board_relay_control_architecture.md (B03_RELAY_CONTROL)
  - Added docs/board_host_api_relay_architecture.md (B03_HOST_API_RELAY)
  - Froze host command names relay_set / relay_set_all / relay_get /
    relay_get_all; updated board_host_api_extensions.md + particular_scope
  - Wrote implementer handoff with file list, boot order, acceptance criteria
  - code adapted from base (if any): none (architect hard stop)
Open questions:
  - OLED address / flash / BOM / strapping (hardware confirmation)
```

## Sync entry

```text
Consumed IDs: (tester RCA → architecture; no new BASE-CHANGE)
Date consumed: 2026-11-03
Agent role: architect (docs)
Actions taken in this repo:
  - Absorbed tester power-cycle relay glitch RCA
    (agent-workspaces/tester/handoff.md)
  - Extended B03_RELAY_CONTROL with B03_RELAY_OE_LEVEL_BEFORE_DIR in
    docs/board_relay_control_architecture.md (level-before-direction)
  - Updated board_hardware_description.md enable summary + product
    architecture acceptance
  - Reopened implementer handoff for relay_ctrl_init patch only
  - code adapted from base (if any): none (architect hard stop)
Open questions:
  - Retest after implementer patch; UART remux only if chatter remains
```

## Sync entry

```text
Consumed IDs: (particular OLED architecture; no new BASE-CHANGE)
Date consumed: 2026-11-03
Agent role: architect (docs)
Actions taken in this repo:
  - Added docs/board_main_relay_oled_architecture.md (B03_MAIN_RELAY_OLED)
  - Froze aesthetic N:[X] with square brackets; 2 channels/line; tokens
    0/1/P/T/E (mode-stable, no pulse flicker)
  - Updated particular_scope, product architecture, functional description,
    implementer/architect handoffs
  - code adapted from base (if any): none (architect hard stop)
Open questions:
  - OLED I2C address; pulse/train/encoder driver handoffs later
```

## Sync entry

```text
Consumed IDs: (tester OLED dynamic token gap → architecture)
Date consumed: 2026-11-03
Agent role: architect (docs)
Actions taken in this repo:
  - Added docs/board_relay_oled_view_mode_architecture.md
    (B03_RELAY_OLED_VIEW_MODE)
  - Superseded "P/T/E reserved only" in board_main_relay_oled_architecture.md
  - Binding: pulse/hold→P, train→T, encoder_step→E; drive view-inert;
    set_view_mode + callback for OLED refresh
  - Reopened implementer handoff for relay_ctrl + relay_sched wiring
  - code: none (architect hard stop)
Open questions:
  - none for token mapping; optional host JSON mode field later
```
