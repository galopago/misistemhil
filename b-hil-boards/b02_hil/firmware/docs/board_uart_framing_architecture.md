# Board UART Framing Architecture — `b02_hil`

Normative particular architecture for **DUT UART baud + word framing**
supported by the hardware on this board.

Source handoff: `agent-workspaces/architect/handoff.md`,
`B02_UART_FRAMING_HW_FULL`.

Supersedes the v1 “only 8 / none / 1” restriction in
`docs/board_uart_path_line_architecture.md` for `uart_config_*` and stream
open params. Boot default remains **9600 8N1**.

Tester return (2026-11-18): unlock habitual **`8E1` / `8O1`**. Human follow-up:
document and support **all framing combinations the present hardware can do**.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

Freeze the allow-list so implementer, host API, OLED, and tester share one
matrix: every combination the ESP32-C3 UART driver can apply on this PCB, at
every already-allowed baud.

## Hardware capability (what limits framing)

| Block | Effect on framing |
| --- | --- |
| ESP32-C3 SuperMini UART (`GPIO5`/`GPIO6`) | **Limits** word length, parity, stop bits (ESP-IDF `uart` driver) |
| ADuM1412 isolator | Transparent to framing |
| MAX485E + path/line relays | Transparent to framing |
| Lab USB–UART / USB–RS485 peers | May not support every MCU combo; tester validates peers separately |

ESP-IDF / ESP32-C3 UART (product authority for this board):

| Field | Hardware-supported values |
| --- | --- |
| Data bits | `5`, `6`, `7`, `8` |
| Parity | `none`, `even`, `odd` (no mark/space on this SoC UART) |
| Stop bits | `1`, `2`, and **`1.5` only when data bits = 5** |

Mark/space parity **MUST** remain rejected (`bad_request`).

## Baud allow-list (unchanged)

```text
1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
```

Any other baud → `bad_request`. Default baud: **9600**.

## Framing allow-list (normative matrix)

A config is valid iff:

1. `baud` ∈ baud allow-list  
2. `data_bits` ∈ {5,6,7,8}  
3. `parity` ∈ {`"none"`,`"even"`,`"odd"`} (exact lowercase strings)  
4. `stop_bits` ∈ {`1`, `2`} normally; **`1.5` allowed only if `data_bits == 5`**  
5. Else → `bad_request`

### All valid framing profiles (27)

OLED / human shorthand uses `<data><parityLetter><stopToken>` where parity
letter is `N`/`E`/`O` and stop token is `1`, `2`, or `15` (means 1.5).

| Profile | data_bits | parity | stop_bits | OLED cluster |
| --- | --- | --- | --- | --- |
| 5N1 | 5 | none | 1 | `5N1` |
| 5N15 | 5 | none | 1.5 | `5N15` |
| 5N2 | 5 | none | 2 | `5N2` |
| 5E1 | 5 | even | 1 | `5E1` |
| 5E15 | 5 | even | 1.5 | `5E15` |
| 5E2 | 5 | even | 2 | `5E2` |
| 5O1 | 5 | odd | 1 | `5O1` |
| 5O15 | 5 | odd | 1.5 | `5O15` |
| 5O2 | 5 | odd | 2 | `5O2` |
| 6N1 | 6 | none | 1 | `6N1` |
| 6N2 | 6 | none | 2 | `6N2` |
| 6E1 | 6 | even | 1 | `6E1` |
| 6E2 | 6 | even | 2 | `6E2` |
| 6O1 | 6 | odd | 1 | `6O1` |
| 6O2 | 6 | odd | 2 | `6O2` |
| 7N1 | 7 | none | 1 | `7N1` |
| 7N2 | 7 | none | 2 | `7N2` |
| 7E1 | 7 | even | 1 | `7E1` |
| 7E2 | 7 | even | 2 | `7E2` |
| 7O1 | 7 | odd | 1 | `7O1` |
| 7O2 | 7 | odd | 2 | `7O2` |
| 8N1 | 8 | none | 1 | `8N1` |
| 8N2 | 8 | none | 2 | `8N2` |
| 8E1 | 8 | even | 1 | `8E1` |
| 8E2 | 8 | even | 2 | `8E2` |
| 8O1 | 8 | odd | 1 | `8O1` |
| 8O2 | 8 | odd | 2 | `8O2` |

**Forbidden examples:** `6N15`, `8N15`, `7E15` (1.5 stop without 5 data bits);
`parity: "mark"` / `"space"`; `data_bits: 9`.

Cross product with 8 bauds ⇒ **216** legal full configs (27 × 8). Implementer
**MUST NOT** hand-list 216 in code; validate with the rules above.

### Habitual profiles (tester priority)

Still first-class (subset of the matrix); boot default remains **8N1 @ 9600**:

1. `8N1`  
2. `8E1`  
3. `8O1`  

## Host API (`uart_config_set` / `uart_config_get` / stream open params)

### JSON types

| Field | Type | Notes |
| --- | --- | --- |
| `baud` | int | Allow-list |
| `data_bits` | int | 5…8 |
| `parity` | string | `none` \| `even` \| `odd` |
| `stop_bits` | number | `1`, `2`, or `1.5` (JSON number; only with 5 data bits) |

Partial updates unchanged: omitted fields keep previous values; at least one
field required on set. Stream open may still pass the same fields.

`uart_config_set` while `dut_uart` open → still `busy`.

### ESP-IDF mapping (implementer)

| Config | `uart_word_length_t` / parity / stop |
| --- | --- |
| data 5…8 | `UART_DATA_5_BITS` … `UART_DATA_8_BITS` |
| none / even / odd | `UART_PARITY_DISABLE` / `UART_PARITY_EVEN` / `UART_PARITY_ODD` |
| stop 1 / 1.5 / 2 | `UART_STOP_BITS_1` / `UART_STOP_BITS_1_5` / `UART_STOP_BITS_2` |

Apply via `uart_param_config` / reconfigure path already used by `dut_uart`.

## OLED line 3 (`docs/board_main_uart_oled_architecture.md`)

Keep grammar `BAUD <n> <d><p><s>` with:

| stop_bits | `<s>` token on glass |
| --- | --- |
| 1 | `1` |
| 2 | `2` |
| 1.5 | `15` |

Examples:

```text
BAUD 9600 8N1
BAUD 9600 8E1
BAUD 9600 8O1
BAUD 115200 8E1
BAUD 9600 7E2
BAUD 9600 5N15
```

Longest common case `BAUD 115200 5E15` = 15 characters ≤ 16 MEDIUM budget.

Update OLED formatter: stop token `15` when stop is 1.5; do not print a decimal
point on glass.

## Capabilities / docs text

- Command notes for `uart_config_set`: allow data 5–8, parity none|even|odd,
  stop 1|2|1.5(with 5 data), baud list unchanged.
- Remove “only 8 / none / 1 in v1” language from capabilities descriptors and
  `board_host_api_extensions.md`.

## Files expected (implementer)

| Path | Change |
| --- | --- |
| `components/dut_uart/dut_uart.c` | `framing_valid` + apply parity/data/stop to driver |
| `components/dut_uart/include/dut_uart.h` | Comments / types if stop stored as float or enum |
| `components/host_api/host_api_command.c` | Accept new parity/data/stop; parse 1.5 |
| `components/host_api/host_api_capabilities.c` | Descriptor notes |
| `components/app_core/app_core_uart_oled.c` | Stop token `15` for 1.5 |

## Acceptance criteria

1. All 27 framing profiles accepted by `uart_config_set` at baud 9600 (smoke).
2. `8N1` / `8E1` / `8O1` bidirectional echo PASS on TTL and RS-485 (tester).
3. `1.5` stop rejected unless `data_bits==5`; mark/space rejected.
4. OLED shows matching cluster (`8E1`, `5N15`, etc.).
5. Default after boot remains `BAUD 9600 8N1`.

## Non-goals

- Mark/space parity  
- Baud rates outside the existing eight  
- Hot framing change while stream open (still `busy`)  
- Redesigning path/line or stream DE behavior  

## Related documents

- `docs/board_uart_path_line_architecture.md` — commands (update allow rules)  
- `docs/board_main_uart_oled_architecture.md` — line 3 glass  
- `docs/board_uart_stream_architecture.md` — open params use same matrix  
