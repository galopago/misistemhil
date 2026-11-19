# Board Base Sync Log — `b02_hil`

Append-only log of consumed `BASE-CHANGE-…` IDs for this tree's particular docs
and local working copies of family contracts.

## Sync entry

```text
Consumed IDs: BASE-CHANGE-20261102-1, BASE-CHANGE-20261102-2,
  BASE-CHANGE-20261102-3, BASE-CHANGE-20261103-1, BASE-CHANGE-20261103-2,
  BASE-CHANGE-20261103-3, BASE-CHANGE-20261103-4, BASE-CHANGE-20261103-5,
  BASE-CHANGE-20261103-6
Date consumed: 2026-11-17
Agent role: architect
Actions taken in this repo:
  - files modified / created:
    - Local working copy of family docs/ contracts from b00_basehil
      (excluding b00 particular board_* content)
    - docs/board_functional_description.md (new for b02)
    - docs/board_hardware_description.md (new; PCB-authoritative pin map)
    - docs/board_particular_scope.md
    - docs/board_host_api_extensions.md (none for bootstrap)
    - docs/board_base_sync_log.md
    - AGENTS.md, README.md, agent-workspaces scaffolding
  - code adapted from base (if any): none yet (architect bootstrap docs-only;
    firmware adapt-copy pending implementer handoff B02_FIRMWARE_ADAPT_COPY)
  - deferred (with reason):
    - RS-485/TTL product host API and relay truth table (needs product
      architecture handoff; hardware map recorded only)
Open questions:
  - See agent-workspaces/architect/handoff.md B02_DAUGHTER_BOOTSTRAP
```

## Sync entry

```text
Consumed IDs: (no new BASE-CHANGE; particular clarification)
Date consumed: 2026-11-17
Agent role: architect
Actions taken in this repo:
  - Human product description absorbed: isolated UART; K1 TTL/RS-485 mux;
    K2 term; K3 pull-down; K4 pull-up; ULN + /KOE buffer.
  - Added docs/board_gpio_bringup_architecture.md (B02_GPIO_BRINGUP_SAFE).
  - Updated functional/hardware/particular docs + implementer handoff
    (adapt-copy onboarding + level-before-direction board_ctrl_init).
  - code adapted from base: still pending implementer
  - deferred: runtime mux/term/DE/UART product APIs
Open questions:
  - Bench-confirm de-energized K1 = TTL; DE polarity through isolator
```

## Sync entry

```text
Consumed IDs: (no new BASE-CHANGE; particular UART architecture)
Date consumed: 2026-11-18
Agent role: architect
Actions taken in this repo:
  - docs/board_uart_path_line_architecture.md (new)
  - docs/board_uart_stream_architecture.md (new)
  - Updated board_host_api_extensions.md, board_particular_scope.md
  - Implementer pending B02_UART_PATH_LINE + B02_UART_STREAM
  - deferred firmware to implementer; DE polarity bench confirm to tester
Open questions:
  - 485_CTR=1 drive confirmation on hardware
```

## Sync entry

```text
Consumed IDs: (no new BASE-CHANGE; particular OLED HMI)
Date consumed: 2026-11-18
Agent role: architect
Actions taken in this repo:
  - docs/board_main_uart_oled_architecture.md (new) — STA FULL_FOUR_LINES HMI
  - Updated particular_scope, architect/implementer/tester handoffs
  - Firmware pending B02_MAIN_UART_OLED
Open questions:
  - None for layout v1
```

## Sync entry

```text
Consumed IDs: (no new BASE-CHANGE; particular framing widen)
Date consumed: 2026-11-18
Agent role: architect
Actions taken in this repo:
  - docs/board_uart_framing_architecture.md (new) — full ESP32-C3 matrix
    (27 profiles: 5–8 data, N/E/O, stop 1|2|1.5@5data) × existing bauds
  - Updated path_line, OLED line-3 stop token, extensions, handoffs
  - Firmware pending B02_UART_FRAMING_HW_FULL
Open questions:
  - None
```
