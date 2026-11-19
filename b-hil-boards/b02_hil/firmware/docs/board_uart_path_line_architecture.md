# Board UART Path / Line / Config Architecture — `b02_hil`

Normative particular architecture for **semantic host commands** that select
the isolated UART electrical path (TTL vs RS-485), RS-485 line network
resistors, and UART framing — independently of binary streaming.

Source handoff: `agent-workspaces/architect/handoff.md`, `B02_UART_PATH_LINE`
(with `B02_UART_STREAM` for the byte plane).

Human decisions (2026-11-18):

- Path and line resistors are controlled by **POST `/v1/command`** (not only
  stream open params), so relays can be toggled for realistic DUT tests
  **without** reconfiguring baud/bits.
- Default UART framing after boot: **9600 8N1**.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

Freeze command names, args, results, errors, relay mapping, and module
boundaries so path/line/config behavior is deterministic for implementers and
agents.

## Related documents

| Document | Role |
| --- | --- |
| `docs/board_uart_stream_architecture.md` | `dut_uart` stream + RS-485 DE |
| `docs/board_host_api_relay_architecture.md` | Raw `relay_set` / `relay_get` coil test |
| `docs/board_gpio_bringup_architecture.md` | `/KOE` init; pin macros |
| `docs/board_hardware_description.md` | PCB authority |
| `docs/external_host_command_api.md` | Envelope / errors |
| `docs/board_host_api_extensions.md` | Checklist |

## Normative language

`MUST` / `MUST NOT` / `SHOULD` / `MAY` as in base host API docs.

## Scope

### In scope

- Commands: `uart_path_set`, `uart_path_get`, `uart_line_set`, `uart_line_get`,
  `uart_config_set`, `uart_config_get`
- Mapping to relays K1…K4 via `board_ctrl` (and `/KOE` already enabled)
- Independence: path/line **MUST NOT** change baud; config **MUST NOT** move
  relays
- Interaction rules with an open `dut_uart` stream

### Out of scope

- Binary frame I/O (see stream architecture)
- User-facing DE control (`485_CTR` owned by stream TX path)
- Masks / `relay_set_all`
- Changing family `/v1` paths

## Hardware mapping

| Semantic | Relay | Coil GPIO | `on` / selected meaning |
| --- | --- | --- | --- |
| Path `ttl` | K1 de-energized | `GPIO21` | UART mux → TTL on `J2` |
| Path `rs485` | K1 energized | `GPIO21` | UART mux → MAX485 |
| `term` true | K2 on | `GPIO9` | Termination A↔B via `R6` |
| `pulldown` true | K3 on | `GPIO3` | Pull-down on B |
| `pullup` true | K4 on | `GPIO4` | Pull-up on A |

Cold boot (after `board_ctrl_init`): path **ttl**, term/pullup/pulldown
**false**, `/KOE` **enabled**.

`/KOE` **MUST** remain enabled for any path/line change (same rule as
`relay_set`). Path/line setters **MUST NOT** pulse or disable OE.

## Transport

```text
POST http://HIL-02-<MAC4>.local/v1/command
Content-Type: application/json
```

Family envelope: `docs/external_host_command_api.md`. Handler budget **5000 ms**.

## Command: `uart_path_set`

Select exactly one electrical path. Does **not** change UART baud/framing or
line resistor relays.

### Request

```json
{
  "id": "p1",
  "name": "uart_path_set",
  "args": { "path": "rs485" }
}
```

| Args | Type | Required | Rules |
| --- | --- | --- | --- |
| `path` | string | Yes | Exactly `"ttl"` or `"rs485"` (lowercase) |

Unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "p1",
  "api_version": 1,
  "result": { "path": "rs485" }
}
```

### Behavior

| `path` | Action |
| --- | --- |
| `"ttl"` | `board_ctrl_relay_set(1, false)` (or equivalent helper) |
| `"rs485"` | `board_ctrl_relay_set(1, true)` |

**MAY** be called while `dut_uart` stream is open (realistic path switching).
Firmware **MUST NOT** auto-close the stream. Document for agents that the DUT
may see electrical glitches.

### Errors

| Condition | `error.code` |
| --- | --- |
| Bad/missing `path` | `bad_request` |
| Host API inactive | `unavailable` |
| `board_ctrl` not ready / GPIO fault | `internal` |

## Command: `uart_path_get`

### Request

```json
{ "id": "p2", "name": "uart_path_get", "args": {} }
```

Empty args object; unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "p2",
  "api_version": 1,
  "result": { "path": "ttl" }
}
```

`path` **MUST** reflect software shadow (K1 off → `"ttl"`, on → `"rs485"`).

## Command: `uart_line_set`

Set RS-485 network resistors. Each provided flag updates that relay only;
omitted flags **MUST** leave that relay unchanged (partial update).

### Request

```json
{
  "id": "l1",
  "name": "uart_line_set",
  "args": {
    "term": true,
    "pullup": true,
    "pulldown": false
  }
}
```

| Args | Type | Required | Rules |
| --- | --- | --- | --- |
| `term` | bool | No* | Termination (K2) |
| `pullup` | bool | No* | Pull-up (K4) |
| `pulldown` | bool | No* | Pull-down (K3) |

\* At least **one** of the three **MUST** be present. All three omitted →
`bad_request`. Unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "l1",
  "api_version": 1,
  "result": {
    "term": true,
    "pullup": true,
    "pulldown": false
  }
}
```

`result` **MUST** return the **full** line shadow after the update (all three
bools), not only the keys that were set.

### Behavior

| Flag | Relay channel |
| --- | --- |
| `term` | 2 (K2) |
| `pulldown` | 3 (K3) |
| `pullup` | 4 (K4) |

**MAY** run with stream open; **MUST NOT** auto-close stream; **MUST NOT**
change path or UART config.

Line settings are meaningful primarily for RS-485 bus topology; firmware
**MUST** still apply them when path is `ttl` (hardware allows; agent
responsibility).

### Errors

| Condition | `error.code` |
| --- | --- |
| No flags / wrong types / unknown keys | `bad_request` |
| Host API inactive | `unavailable` |
| Driver fault | `internal` |

## Command: `uart_line_get`

### Request

```json
{ "id": "l2", "name": "uart_line_get", "args": {} }
```

### Success

```json
{
  "ok": true,
  "id": "l2",
  "api_version": 1,
  "result": {
    "term": false,
    "pullup": false,
    "pulldown": false
  }
}
```

## Command: `uart_config_set`

Configure UART framing on the MCU DUT UART (`GPIO5` TX / `GPIO6` RX). Does
**not** move relays.

### Request

```json
{
  "id": "c1",
  "name": "uart_config_set",
  "args": {
    "baud": 9600,
    "data_bits": 8,
    "parity": "none",
    "stop_bits": 1
  }
}
```

| Args | Type | Required | Rules |
| --- | --- | --- | --- |
| `baud` | int | No* | Allow-list below; default **9600** |
| `data_bits` | int | No* | **5…8** — full matrix in `docs/board_uart_framing_architecture.md` |
| `parity` | string | No* | `"none"` \| `"even"` \| `"odd"` |
| `stop_bits` | number | No* | `1`, `2`, or `1.5` (**1.5 only if data_bits=5**) |

\* At least one field **MUST** be present. Partial update: omitted fields keep
previous values. After cold boot, stored config is **9600 / 8 / none / 1**
(`8N1`) before any set.

**Allowed `baud` values:** `1200`, `2400`, `4800`, `9600`, `19200`,
`38400`, `57600`, `115200`. Other values → `bad_request`.

**Framing:** every combination allowed by
`docs/board_uart_framing_architecture.md` (27 profiles × 8 bauds). Habitual
`8N1` / `8E1` / `8O1` are included. Mark/space and invalid 1.5-stop combos →
`bad_request`.

Unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "c1",
  "api_version": 1,
  "result": {
    "baud": 9600,
    "data_bits": 8,
    "parity": "none",
    "stop_bits": 1
  }
}
```

`result` **MUST** be the full effective config after update.

### Interaction with stream

If `dut_uart` session is **open**: `uart_config_set` **MUST** fail with
`busy` and leave framing unchanged. Close stream, then set config, then
re-open.

### Errors

| Condition | `error.code` |
| --- | --- |
| Bad args / unsupported baud or framing | `bad_request` |
| Stream open | `busy` |
| Host API inactive | `unavailable` |
| UART driver fault | `internal` |

## Command: `uart_config_get`

### Request

```json
{ "id": "c2", "name": "uart_config_get", "args": {} }
```

### Success

Same `result` shape as `uart_config_set` success (current stored config).

## Relationship to `relay_set`

| API | Audience |
| --- | --- |
| `uart_path_*` / `uart_line_*` | Product / agent (semantic) |
| `relay_set` / `relay_get` | Electronic coil bring-up |

Both **MUST** share the same coil shadow in `board_ctrl`. Setting path via
`uart_path_set("rs485")` **MUST** make `relay_get` channel 1 report `on:true`,
and vice versa.

## C API expectations (implementer)

Prefer helpers on `board_ctrl` or a thin facade used by host handlers:

```c
typedef enum {
    BOARD_UART_PATH_TTL = 0,
    BOARD_UART_PATH_RS485 = 1,
} board_uart_path_t;

esp_err_t board_ctrl_uart_path_set(board_uart_path_t path);
esp_err_t board_ctrl_uart_path_get(board_uart_path_t *out);

esp_err_t board_ctrl_uart_line_set(bool term, bool pullup, bool pulldown,
                                   bool has_term, bool has_pullup, bool has_pulldown);
esp_err_t board_ctrl_uart_line_get(bool *term, bool *pullup, bool *pulldown);
```

UART framing storage/apply lives in `components/dut_uart/` (see stream doc),
not in relay GPIO code.

Host handlers: `host_api` **REQUIRES** `board_ctrl` (and `dut_uart` for
config); call helpers directly (no CMake cycle through `app_core`).

## `capabilities` descriptors (required names)

Commands **MUST** appear in firmware `capabilities.commands` with summaries:

| name | summary |
| --- | --- |
| `uart_path_set` | Select DUT UART path ttl or rs485 (relay K1; no baud change) |
| `uart_path_get` | Read current DUT UART path |
| `uart_line_set` | Set RS-485 term / pullup / pulldown relays (partial OK) |
| `uart_line_get` | Read RS-485 line resistor relay shadow |
| `uart_config_set` | Set DUT UART baud/framing (HW matrix; default 9600 8N1; busy if stream open) |
| `uart_config_get` | Read DUT UART baud/framing |

## Acceptance criteria

1. Path/line commands never alter baud; config never moves K1–K4.
2. Default config after boot is 9600 8N1.
3. Path/line may change with stream open; config_set returns `busy` if open.
4. Semantic path/line stay consistent with `relay_get` shadows.
5. `/KOE` remains enabled; no user DE command.

## Non-goals

- Hot-reconfig of baud while stream open
- Exposing `485_CTR` as a command
- Auto-enabling term/PU/PD when selecting `rs485`
