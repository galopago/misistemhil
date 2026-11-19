# Architect Handoff — `b02_hil`

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting any task in this file, run the **Role Boundary Check**:

- Read `ROLE.md` in this folder, `../../AGENTS.md`, and `../../docs/methodology.md`.
- Confirm the request is **architecture and documentation**, not firmware
  implementation, builds, or formal validation.
- If the task includes code, tests, or flash steps, stop and tell the human that
  the implementer or tester must execute those parts unless explicitly
  redirected.

## Current Status

Active / recent handoffs:

- `B02_UART_FRAMING_HW_FULL` (**docs + code + tester PASS / CLOSED** 2026-11-18;
  full HW framing matrix incl. 8E1/8O1 + 5–8 data)

- `B02_MAIN_UART_OLED_FRAMING` (**docs + code**; tester PASS)
- `B02_MAIN_UART_OLED` (**docs + code**; tester PASS; framing follow-up above)
- `B02_UART_PATH_LINE` + `B02_UART_STREAM` (**docs complete** 2026-11-18 —
  firmware **pending** implementer; see
  `docs/board_uart_path_line_architecture.md`,
  `docs/board_uart_stream_architecture.md`,
  `../implementer/handoff.md`)
- `B02_HOST_API_RELAY_SINGLE` (**docs complete**; implementer reported code
  done 2026-11-18 — build/flash by tester)
- `B02_DAUGHTER_BOOTSTRAP` (**docs complete** 2026-11-17)
- `B02_GPIO_BRINGUP_SAFE` (**docs complete** 2026-11-17)
- `B02_FIRMWARE_ADAPT_COPY` + GPIO slice (**implementer reported implemented**
  2026-11-17 — build/flash by tester)

## B02_DAUGHTER_BOOTSTRAP

```text
Role boundary check:
  Architect-owned documentation only. No components/, main/, sdkconfig, or
  idf.py. Firmware file copy is pending for the implementer.
ID: B02_DAUGHTER_BOOTSTRAP
Date: 2026-11-17
Source:
  - Human request: act as architect; create b02_hil daughter tree from
    b00_basehil without dependencies or links to the base codebase so this
    board can evolve independently; follow base reuse documentation.
Objective:
  Bootstrap b02_hil/firmware as an independent particular board:
  (1) local working copy of family contracts and methodology;
  (2) particular docs rewritten for b02 (RS-485/TTL isolated HIL);
  (3) PCB-authoritative hardware map;
  (4) implementation-ready pending task for adapt-copy of reference firmware
      with zero build-time coupling to b00_basehil.
Reason:
  Family reuse is contracts-first, no submodule
  (docs/hil_base_reuse_contract.md). Sibling boards own adapted code copies.
  b02_hil/firmware was empty; PCB title HIL-02 already exists under ../pcb/.
Authorized files (architect):
  - docs/** (working-copy contracts + particular board_* docs)
  - AGENTS.md, README.md
  - agent-workspaces/architect/**
  - agent-workspaces/implementer/handoff.md (pending tasks only)
  - agent-workspaces/tester/* stubs as needed
  - tests/README.md
Expected changes (done by architect):
  - Snapshot family docs from b00_basehil into this docs/ tree.
  - Create board_functional_description.md, board_hardware_description.md,
    board_particular_scope.md, board_host_api_extensions.md,
    board_base_sync_log.md for board 02.
  - Consume BASE-CHANGE-20261102-1 … BASE-CHANGE-20261103-6 in sync log.
  - Record B02_FIRMWARE_ADAPT_COPY for implementer with deterministic copy
    rules and identity/pin constraints.
Module boundaries and contracts:
  - Family pillars remain those in local docs/ working copies.
  - Board number / identity: "02" / HIL-02-<MAC4>.
  - Pins: docs/board_hardware_description.md (PCB authoritative).
  - No EXTRA_COMPONENT_DIRS, submodule, or relative path into b00_basehil
    after adapt-copy completes.
  - Product RS-485/relay host API is OUT OF SCOPE for bootstrap; do not invent.
Non-goals:
  - Architect writing/editing components/ or main/.
  - Building or flashing.
  - Completing RS-485 product feature architecture in this handoff.
  - Maintaining a live link to b00_basehil source.
Acceptance criteria:
  - Particular docs exist and describe b02, not the base bench mission.
  - Hardware pin table matches ../pcb/b02.kicad_pcb pad pinfunction for U4.
  - README/AGENTS state no submodule / no live base path dependency.
  - Implementer handoff B02_FIRMWARE_ADAPT_COPY lists exact allowed paths,
    renames, board identity, pin constants, and non-goals.
  - Sync log records consumed BASE-CHANGE IDs through BASE-CHANGE-20261103-6.
Open questions (block product firmware, not bootstrap adapt-copy):
  1. Relay truth table: energized K1 = TTL or RS-485 path?
  2. K2/K3/K4 termination/bias combinations (which coils = failsafe / on-bus)?
  3. Active level of 485_CTR after isolator for MAX485 DE/~RE (high = drive?).
  4. Host API surface for UART (stream vs commands) and relay mode control.
  5. Whether TTL_IN (GPIO10) is required in v1 product behavior.
  6. STA OLED: keep base WIFI OK screen vs particular serial status screen.
```

## B02_GPIO_BRINGUP_SAFE

```text
Role boundary check:
  Architect documentation only. Firmware remains pending for implementer.
ID: B02_GPIO_BRINGUP_SAFE
Date: 2026-11-17
Source:
  - Human description: galvanically isolated UART; relay mux TTL/RS-485;
    relay-switched 485 pull-up, pull-down, termination; ULN coil driver;
    buffer to prevent spurious relay activation during ESP32 GPIO init;
    first implementer scope = base onboarding + GPIO init without chatter
    (same fix class as b03_hil).
Objective:
  Record correct functional pin/relay map and normative board_ctrl bring-up
  so adapt-copy firmware ends in a cold-safe actuator state before WiFi
  onboarding continues.
Authorized files:
  - docs/board_gpio_bringup_architecture.md (new)
  - docs/board_hardware_description.md
  - docs/board_functional_description.md
  - docs/board_particular_scope.md
  - agent-workspaces/architect/handoff.md, decisions.md, backlog.md
  - agent-workspaces/implementer/handoff.md (pending tasks)
Acceptance criteria:
  - K1=mux, K2=term, K3=pull-down, K4=pull-up documented with GPIOs
  - Level-before-direction /KOE sequence mandatory; OE enabled only with
    all CTR/DE inactive
  - Implementer handoff lists Slice A (adapt-copy) + Slice B (board_ctrl)
  - No runtime mux/term/DE API authorized yet
Open questions:
  - Confirm on bench that de-energized K1 is TTL (NC assumption)
  - Isolator DE polarity when enabling TX later
```

## B02_HOST_API_RELAY_SINGLE

```text
Role boundary check:
  Architect documentation only. Firmware pending for implementer.
ID: B02_HOST_API_RELAY_SINGLE
Date: 2026-11-18
Source:
  - Human: plan a minimal REST interface like the relay sister board, to
    electrically test coils — turn on one relay, turn off one relay; one relay
    at a time (no masks). Write implementer instructions; human will test as
    tester after code lands.
Objective:
  Authorize only relay_set / relay_get for channels 1..4 (K1..K4), each request
  touching exactly one coil. Mirror b03 single-channel command shape; omit
  set_all/mask/states.
Authorized files:
  - docs/board_host_api_relay_architecture.md (new)
  - docs/board_host_api_extensions.md
  - docs/board_gpio_bringup_architecture.md (cross-link)
  - docs/board_particular_scope.md
  - agent-workspaces/architect/handoff.md, decisions.md, backlog.md
  - agent-workspaces/implementer/handoff.md (pending)
  - agent-workspaces/tester/handoff.md (human curl checklist)
Acceptance criteria:
  - Normative docs freeze channel map, JSON args, errors, capabilities string
  - Explicit non-goals: masks / set_all / DE-as-channel
  - Implementer has file list + board_ctrl_relay_set/get + host_api wiring
  - Tester handoff has curl on/off examples for channels 1..4
Open questions:
  - None for this minimal test API
```

## B02_UART_FRAMING_HW_FULL

```text
Role boundary check: architect docs only.
ID: B02_UART_FRAMING_HW_FULL
Date: 2026-11-18
Source:
  - tester/feedback.md: unlock 8E1/8O1 (habitual); baud list OK
  - human: list and support all combinations present hardware can do
Objective:
  Normative matrix = ESP32-C3 UART capabilities (isolator/MAX485 transparent):
  data 5–8, parity none|even|odd, stop 1|2|1.5(with 5 data), × existing bauds.
Authorized files:
  - docs/board_uart_framing_architecture.md (new)
  - docs/board_uart_path_line_architecture.md
  - docs/board_main_uart_oled_architecture.md
  - docs/board_host_api_extensions.md, board_particular_scope.md
  - agent-workspaces/architect/*, implementer/handoff.md, tester/handoff.md
Acceptance:
  - 27 framing profiles tabulated; rules for validation
  - Implementer handoff points at dut_uart framing_valid + IDF mapping
  - OLED stop token 15 for 1.5
Open questions: none
```

## B02_MAIN_UART_OLED_FRAMING

```text
Role boundary check: architect docs only.
ID: B02_MAIN_UART_OLED_FRAMING
Date: 2026-11-18
Source: tester/handoff.md after B02_MAIN_UART_OLED PASS — show data bits +
  parity (and stop) on OLED line 3, spirit of 9600 8N1.
Objective: Freeze line 3 as BAUD <n> <d><p><s>; hand to implementer.
Authorized files:
  - docs/board_main_uart_oled_architecture.md
  - agent-workspaces/architect/handoff.md, decisions.md
  - agent-workspaces/implementer/handoff.md (pending)
  - agent-workspaces/tester/handoff.md
Acceptance:
  - Normative examples BAUD 9600 8N1 / BAUD 115200 8N1
  - Implementer told exact snprintf intent in app_core_uart_oled.c
Open questions: none
```

## B02_MAIN_UART_OLED

```text
Role boundary check:
  Architect documentation only. No components/ or idf.py.
ID: B02_MAIN_UART_OLED
Date: 2026-11-18
Source:
  - Human: after TX/RX and 485↔TTL mux tests OK, want graphical HMI like
    relay board — once IP acquired show board id and relevant UART state
    (PATH RS485|TTL; if 485 show pullup/pulldown/term; baud). Clear docs for
    implementer; minimal human-readable HMI.
Objective:
  Freeze FULL_FOUR_LINES STA screen grammar and refresh rules.
Authorized files:
  - docs/board_main_uart_oled_architecture.md (new)
  - docs/board_particular_scope.md
  - agent-workspaces/architect/handoff.md, decisions.md, backlog.md
  - agent-workspaces/implementer/handoff.md (pending)
  - agent-workspaces/tester/handoff.md
Acceptance criteria:
  - Exact line strings documented (PATH / T:U:D / BAUD)
  - Replaces base WIFI OK while STA connected only
  - Implementer has file list and data sources
Open questions:
  - None for v1 layout
```

## B02_UART_PATH_LINE + B02_UART_STREAM

```text
Role boundary check:
  Architect documentation only. No components/, main/, sdkconfig, or idf.py.
ID: B02_UART_PATH_LINE / B02_UART_STREAM
Date: 2026-11-18
Source:
  - Human: plan UART streaming TTL|RS485 (one path via relay); RS-485 DE
    assert on TX then return to RX; mechanism for term/PU/PD; choose semantic
    POST commands (A) so relays toggle without reconfiguring UART; default
    9600 8N1.
Objective:
  Publish implementation-ready particular architectures for path/line/config
  commands and dut_uart stream (family HTTP stream plane + automatic DE).
Authorized files:
  - docs/board_uart_path_line_architecture.md (new)
  - docs/board_uart_stream_architecture.md (new)
  - docs/board_host_api_extensions.md
  - docs/board_particular_scope.md
  - agent-workspaces/architect/handoff.md, decisions.md, backlog.md
  - agent-workspaces/implementer/handoff.md (pending firmware)
  - agent-workspaces/tester/handoff.md (checklist)
Acceptance criteria:
  - Path/line = POST commands; independent of baud; default 9600 8N1
  - Stream dut_uart; DE sequence normative for rs485 writes
  - Implementer handoff lists modules, APIs, forbidden items
Open questions:
  - Bench-confirm 485_CTR=1 is MAX485 drive
```

## B02_FIRMWARE_ADAPT_COPY (summary for cross-link)

Baseline adapt-copy + `board_ctrl_init` in `../implementer/handoff.md`
(implemented 2026-11-17). Relay single-channel code reported 2026-11-18.
**Closed (UART framing):** `B02_UART_FRAMING_HW_FULL` — tester PASS / CLOSED
2026-11-18. Earlier UART path/stream/OLED handoffs also tester PASS.
