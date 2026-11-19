# Implementer handoff — `b02_hil`

## Current — _(none — last handoff closed)_

## Prior — `B02_UART_FRAMING_HW_FULL` (**implemented** + **tester PASS / CLOSED** 2026-11-18)


```text
ID: B02_UART_FRAMING_HW_FULL
Date: 2026-11-18
Role: implementer only (no idf.py)
Source: tester feedback.md — unlock 8E1/8O1; human asked for ALL combinations
  the present ESP32-C3 UART hardware supports.
Normative: docs/board_uart_framing_architecture.md  (READ FIRST)
Also update: path_line doc rules, OLED stop token 15, capabilities notes
Objective:
  Replace framing_valid "8/none/1 only" with the full HW matrix; map to
  ESP-IDF uart_param_config; OLED already shows E/O — extend stop 1.5 → "15".
```

### Allowed framing (must match architecture doc)

**Baud (unchanged):** 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200  

**Data bits:** 5, 6, 7, 8  

**Parity strings:** `"none"`, `"even"`, `"odd"` only (reject mark/space)  

**Stop bits:** `1`, `2`, and `1.5` **only if** `data_bits == 5`  

That is **27** profiles (see table in framing architecture). Do **not** hard-code
216 baud×profile rows — validate with rules.

Boot default remains **9600 8N1**.

### ESP-IDF apply

| Field | Driver |
| --- | --- |
| data 5…8 | `UART_DATA_5_BITS` … `UART_DATA_8_BITS` |
| none/even/odd | `UART_PARITY_DISABLE` / `EVEN` / `ODD` |
| stop 1 / 1.5 / 2 | `UART_STOP_BITS_1` / `1_5` / `2` |

### Host API

- `uart_config_set` / stream open `params`: accept the matrix above.
- Parse `stop_bits` as JSON number (`1`, `2`, or `1.5`).
- Reject invalid combos with `bad_request` (clear message).
- Still `busy` if stream open on config_set.
- Capabilities / command notes: remove “only none / only 8 / only 1”.

### OLED (`app_core_uart_oled.c`)

Line 3 stays `BAUD %d %d%c%s` style:

- parity char `N`/`E`/`O`
- stop token: `"1"`, `"2"`, or `"15"` when stop is 1.5

Examples: `BAUD 9600 8E1`, `BAUD 9600 5N15`.

### Files

| Path | Change |
| --- | --- |
| `components/dut_uart/dut_uart.c` | `framing_valid` + parity/data/stop apply (today hard-codes DISABLE/8/1) |
| `components/dut_uart/include/dut_uart.h` | Store stop as float or enum; comments |
| `components/host_api/host_api_command.c` | Validation messages / parse 1.5 |
| `components/host_api/host_api_capabilities.c` | Descriptor notes |
| `components/app_core/app_core_uart_oled.c` | Stop token `15` |

### Forbidden

- `idf.py`
- Mark/space parity
- Accepting `1.5` stop with data_bits ≠ 5
- Changing baud allow-list

### Acceptance (code)

- `framing_valid` accepts all 27 profiles; rejects illegal 1.5 and mark/space
- Driver applies even/odd (not stuck on DISABLE)
- OLED can show `8E1` / `8O1` / `5N15`
- Notes list files; mark implemented (code only)

### Tester will later check

Echo TTL + RS-485 for `8N1`, `8E1`, `8O1` at least; optional smoke of other
profiles.

---

## Prior — `B02_MAIN_UART_OLED_FRAMING` (**implemented** / tester PASS)

Line 3 shows `BAUD n 8N1`; config still rejected even/odd until this handoff.

### Notes (`B02_UART_FRAMING_HW_FULL`)

Implemented 2026-11-18 (code only; no `idf.py`).

- `dut_uart`: `stop_bits` is `float`; `dut_uart_framing_allowed()` enforces
  27-profile rules; `apply_hw_config` maps data 5–8 / none|even|odd /
  stop 1|1.5|2 to ESP-IDF enums.
- `host_api_command` / `host_api_stream`: accept matrix; parse `stop_bits` 1.5;
  reject illegal combos (`1.5` without data=5, mark/space).
- `host_api_capabilities`: notes updated (no “only 8/none/1”).
- `app_core_uart_oled`: stop token `15` for 1.5 → e.g. `BAUD 9600 5N15`,
  `BAUD 9600 8E1`.

**Closed:** tester PASS (API 27/27, echo 64/64 TTL+RS485 8N1/8E1/8O1 + smoke).
See `../tester/handoff.md`.

