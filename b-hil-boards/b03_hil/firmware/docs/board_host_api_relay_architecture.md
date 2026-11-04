# Board Host API Relay Architecture — `b03_hil`

Normative particular architecture for **remote relay commands** on `b03_hil`
over the family host API (`POST /v1/command`).

Source handoff: `agent-workspaces/architect/handoff.md`, `B03_HOST_API_RELAY`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

Freeze command names, args, results, errors, `capabilities` descriptors, and
module boundaries so an LLM agent and the implementer share one contract.

## Related documents

| Document | Role |
| --- | --- |
| `docs/external_host_api_architecture.md` | Transport, when API is up |
| `docs/external_host_command_api.md` | Envelope / mandatory commands |
| `docs/board_host_api_extensions.md` | Checklist; points here |
| `docs/board_relay_control_architecture.md` | Channel map + `relay_ctrl` |
| `docs/board_product_architecture.md` | Bring-up / no echo-loopback |

## Normative language

`MUST` / `MUST NOT` / `SHOULD` / `MAY` as in base host API docs.

## Scope

### In scope

- Four particular commands: `relay_set`, `relay_set_all`, `relay_get`,
  `relay_get_all`.
- Channel index `1`…`6` and mask `0`…`63` (bit0 = channel 1).
- `capabilities` catalog entries and `info` fields for this board.
- Wiring handlers to `app_core_relay_*` / `relay_ctrl_*`.

### Out of scope

- Binary streams for relays.
- SoftAP-portal relay control (host API only when STA + IPv4).
- Changing `/v1` paths or mandatory `ping` / `info` / `capabilities`.
- `echo` / `loopback` on this board.
- Timed pulses, schedules, or interlocks.

## Transport (unchanged family rules)

```text
POST http://HIL-03-<MAC4>.local/v1/command
Content-Type: application/json
```

Envelope: `docs/external_host_command_api.md`.  
Host API active only when STA has IPv4 and SoftAP portal httpd is not on
port 80.

Handler timeout remains **5000 ms**. Relay handlers **MUST** complete in well
under that budget (GPIO only).

## Channel and mask rules

Identical to `docs/board_relay_control_architecture.md`:

| `channel` | Relay |
| --- | --- |
| 1…6 | K1…K6 |

| Mask bit | Channel |
| --- | --- |
| `1 << 0` | 1 |
| `1 << 1` | 2 |
| `1 << 2` | 3 |
| `1 << 3` | 4 |
| `1 << 4` | 5 |
| `1 << 5` | 6 |

JSON types:

- `channel`: JSON number, integer value 1…6
- `on`: JSON boolean
- `mask`: JSON number, integer value 0…63
- `states`: JSON array of exactly 6 booleans, index 0 = channel 1

## Shared single-channel result object

```json
{
  "channel": 1,
  "on": true
}
```

Used by `relay_get` success `result` and as elements of `relay_get_all`.

## Command: `relay_set`

Set **one** channel.

### Request

```json
{
  "id": "r1",
  "name": "relay_set",
  "args": { "channel": 1, "on": true }
}
```

| Args field | Type | Required | Rules |
| --- | --- | --- | --- |
| `channel` | int | Yes | Exactly `1`…`6` |
| `on` | bool | Yes | `true` = energize, `false` = release |

Unknown top-level arg keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "r1",
  "api_version": 1,
  "result": {
    "channel": 1,
    "on": true
  }
}
```

`result` **MUST** echo the applied channel state (post-set shadow).

### Errors (outer envelope)

| Condition | `error.code` |
| --- | --- |
| Missing/invalid `channel` or `on`; wrong JSON types | `bad_request` |
| Host API not active | `unavailable` |
| `relay_ctrl` not inited / unexpected GPIO fault | `internal` |

## Command: `relay_set_all`

Set **all six** channels in one call.

### Request (normative form A — mask)

```json
{
  "id": "r2",
  "name": "relay_set_all",
  "args": { "mask": 5 }
}
```

`mask: 5` (`0b000101`) → channels **1 and 3** on; 2,4,5,6 off.

### Request (normative form B — states array)

```json
{
  "id": "r2b",
  "name": "relay_set_all",
  "args": {
    "states": [true, false, true, false, false, false]
  }
}
```

Rules:

- Args **MUST** contain **exactly one** of `mask` or `states` (not both, not
  neither).
- If `mask`: integer `0`…`63` inclusive.
- If `states`: JSON array length **exactly 6**, each element boolean.
- `states[i]` corresponds to channel `i+1`.
- Unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "r2",
  "api_version": 1,
  "result": {
    "mask": 5,
    "states": [true, false, true, false, false, false]
  }
}
```

`result.mask` and `result.states` **MUST** both be present and consistent
(`states[i] == ((mask >> i) & 1) != 0`).

### Errors

| Condition | `error.code` |
| --- | --- |
| Both/neither `mask`/`states`; bad types; mask>63; states length≠6 | `bad_request` |
| Host API not active | `unavailable` |
| Driver fault | `internal` |

## Command: `relay_get`

Read **one** channel shadow state.

### Request

```json
{
  "id": "r3",
  "name": "relay_get",
  "args": { "channel": 2 }
}
```

| Args field | Type | Required | Rules |
| --- | --- | --- | --- |
| `channel` | int | Yes | `1`…`6` |

### Success

```json
{
  "ok": true,
  "id": "r3",
  "api_version": 1,
  "result": {
    "channel": 2,
    "on": false
  }
}
```

### Errors

| Condition | `error.code` |
| --- | --- |
| Bad/missing `channel` | `bad_request` |
| Host API not active | `unavailable` |
| Driver fault | `internal` |

## Command: `relay_get_all`

Read all six shadow states.

### Request

```json
{
  "id": "r4",
  "name": "relay_get_all",
  "args": {}
}
```

| Args | Rules |
| --- | --- |
| `{}` | No fields; unknown keys → `bad_request` |

### Success

```json
{
  "ok": true,
  "id": "r4",
  "api_version": 1,
  "result": {
    "mask": 0,
    "states": [false, false, false, false, false, false]
  }
}
```

Rules:

- `states` length **exactly 6**, index 0 = channel 1.
- `mask` consistent with `states`.

### Errors

| Condition | `error.code` |
| --- | --- |
| Non-empty unsupported args | `bad_request` |
| Host API not active | `unavailable` |
| Driver fault | `internal` |

## `info` particular values

When host API `info` is implemented on this board:

| Field | Value |
| --- | --- |
| `board_id` | `"b03_hil"` |
| `board_number` | `"03"` |
| `streams_supported` | `false` |

## `capabilities` manifest (firmware)

`capabilities.summary` **MUST** be exactly:

```text
Six-channel relay HIL board with boot-safe buffered GPIO drive
```

`commands` **MUST** include mandatory `ping`, `info`, `capabilities`, plus the
four descriptors below. `streams` **MUST** be `[]`.

### Descriptor: `relay_set`

```json
{
  "name": "relay_set",
  "summary": "Set one relay channel on or off",
  "args": [
    {
      "name": "channel",
      "type": "int",
      "required": true,
      "notes": "Relay channel 1..6 (K1..K6)"
    },
    {
      "name": "on",
      "type": "bool",
      "required": true,
      "notes": "true=energize coil, false=release"
    }
  ],
  "result": [
    { "name": "channel", "type": "int", "notes": "Echo channel" },
    { "name": "on", "type": "bool", "notes": "Applied state" }
  ]
}
```

### Descriptor: `relay_set_all`

```json
{
  "name": "relay_set_all",
  "summary": "Set all six relays via mask or states[6]",
  "args": [
    {
      "name": "mask",
      "type": "int",
      "required": false,
      "notes": "0..63; bit0=ch1; XOR with states (exactly one required)"
    },
    {
      "name": "states",
      "type": "object",
      "required": false,
      "notes": "JSON array of 6 bools; index0=ch1; XOR with mask"
    }
  ],
  "result": [
    { "name": "mask", "type": "int", "notes": "Applied 0..63" },
    {
      "name": "states",
      "type": "object",
      "notes": "Applied array of 6 bools"
    }
  ]
}
```

(`type: object` for `states` matches base descriptor vocabulary when `array`
is not listed; **wire** value MUST be a JSON array of length 6.)

### Descriptor: `relay_get`

```json
{
  "name": "relay_get",
  "summary": "Get one relay channel commanded state",
  "args": [
    {
      "name": "channel",
      "type": "int",
      "required": true,
      "notes": "Relay channel 1..6"
    }
  ],
  "result": [
    { "name": "channel", "type": "int", "notes": "Echo channel" },
    { "name": "on", "type": "bool", "notes": "Commanded state (shadow)" }
  ]
}
```

### Descriptor: `relay_get_all`

```json
{
  "name": "relay_get_all",
  "summary": "Get all six relay commanded states",
  "args": [],
  "result": [
    { "name": "mask", "type": "int", "notes": "0..63 bit0=ch1" },
    {
      "name": "states",
      "type": "object",
      "notes": "Array of 6 bools; index0=ch1"
    }
  ]
}
```

### Non-commands on this board

- Do **not** advertise `echo` or `loopback`.

## Agent discovery sequence

```text
ping → info → capabilities → relay_get_all / relay_set / …
```

Agents **MUST** use names from the on-device manifest.

## curl examples

```bash
curl -sS -X POST "http://HIL-03-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"1","name":"relay_set","args":{"channel":1,"on":true}}'
```

```bash
curl -sS -X POST "http://HIL-03-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"2","name":"relay_set_all","args":{"mask":15}}'
```

```bash
curl -sS -X POST "http://HIL-03-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"3","name":"relay_get_all","args":{}}'
```

## Module ownership

| Module | May | Must not |
| --- | --- | --- |
| `host_api` | Parse args; call `app_core_relay_*` or `relay_ctrl_*`; build JSON; capabilities table | Bit-bang GPIO; include `driver/gpio.h` for relays |
| `app_core` | Facade + boot `relay_ctrl_init` | Bind HTTP |
| `relay_ctrl` | Own GPIO + mutex + shadow | Know host API JSON |
| `board` | Pin macros | Command names |

## Concurrency

- Command handlers for relays **MUST** call into the mutex-protected
  `relay_ctrl` API (directly or via facade).
- v1 still allows only one `POST /v1/command` at a time per base rules; relay
  mutex still required for any future local callers.

## Non-goals (recap)

- Streams of contact events
- Host API on SoftAP
- Persisted power-on relay mask
- `echo` / `loopback`

## Open questions

- None for command wire format. OLED address does not affect these commands.

## Acceptance criteria

1. `capabilities` lists exactly the four relay commands with descriptors above;
   `streams` empty; no `echo`/`loopback`; summary string exact.
2. `relay_set` channel 1…6 toggles only that channel; result echoes state.
3. `relay_set` with channel 0/7 or missing `on` → outer `bad_request`.
4. `relay_set_all` with `mask: 5` yields states
   `[true,false,true,false,false,false]` and matching LEDs.
5. `relay_set_all` with both `mask` and `states` → `bad_request`.
6. `relay_get_all` returns length-6 `states` consistent with `mask`.
7. After reboot, `relay_get_all` reports all false before any set.
8. Commands unavailable during SoftAP provisioning per base rules.
9. `relay_ctrl` has no `host_api` includes.

## Validation notes (tester)

- Prefer STA + mDNS `http://HIL-03-<MAC4>.local/v1/command`.
- Confirm `capabilities` alone is enough for an agent that never read this
  Markdown file.
- Cross-check channel LEDs `D1`…`D6` against `states`.
