# Board Host API Relay Architecture — `b02_hil` (minimal single-channel)

Normative particular architecture for **electronic bring-up tests** of the four
path/bias relays on `b02_hil` over the family host API (`POST /v1/command`).

Source handoff: `agent-workspaces/architect/handoff.md`,
`B02_HOST_API_RELAY_SINGLE`.

Sister-board pattern: `b03_hil` `relay_set` / `relay_get` (single channel only).
This board deliberately **omits** `relay_set_all` / masks / timed actions.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

Freeze a **minimal** remote contract so a human tester can:

1. Energize **exactly one** relay coil.
2. Release **exactly one** relay coil.
3. Optionally read back the software shadow for that channel.

No multi-channel masks, no `states[]`, no pulse/hold, no DE/UART product API.

## Related documents

| Document | Role |
| --- | --- |
| `docs/external_host_api_architecture.md` | Transport, when API is up |
| `docs/external_host_command_api.md` | Envelope / mandatory commands |
| `docs/board_host_api_extensions.md` | Checklist; points here |
| `docs/board_gpio_bringup_architecture.md` | `/KOE` init + pin map |
| `docs/board_hardware_description.md` | PCB authority |

## Normative language

`MUST` / `MUST NOT` / `SHOULD` / `MAY` as in base host API docs.

## Scope

### In scope (this handoff)

- Commands: **`relay_set`**, **`relay_get`**
- Channel index **`1`…`4`** only (one relay per request)
- Software shadow state per channel
- `capabilities` descriptors for those two commands
- Extend `components/board_ctrl/` with mutex-protected set/get
- Wire handlers in `components/host_api/` → `board_ctrl_*`

### Out of scope

- `relay_set_all`, `mask`, `states[]`, bitfields
- `relay_pulse` / hold / encoder / timed actions
- Exposing `485_CTR` / MAX485 DE as a “relay” channel
- Binary streams for relays
- SoftAP-portal relay control (host API only when STA + IPv4)
- Changing `/v1` paths or mandatory `ping` / `info` / `capabilities`
- `echo` / `loopback`

## Transport (unchanged family rules)

```text
POST http://HIL-02-<MAC4>.local/v1/command
Content-Type: application/json
```

Envelope: `docs/external_host_command_api.md`.  
Host API active only when STA has IPv4 and SoftAP portal httpd is not on
port 80.

Handler timeout remains **5000 ms**. Handlers **MUST** complete in well under
that budget (GPIO + mutex only).

## Channel map (canonical)

| `channel` (int) | Relay | Coil GPIO | Cold function when off | On = coil energized |
| --- | --- | --- | --- | --- |
| `1` | `K1` | `GPIO21` | UART mux **TTL** | UART mux **RS-485** |
| `2` | `K2` | `GPIO9` | Termination **off** | Termination **on** |
| `3` | `K3` | `GPIO3` | Pull-down **off** | Pull-down **on** |
| `4` | `K4` | `GPIO4` | Pull-up **off** | Pull-up **on** |

Rules:

- Host API, logs, and C APIs **MUST** use channels `1`…`4` only.
- Channel outside `1`…`4` → `bad_request` (host) / `ESP_ERR_INVALID_ARG` (C).
- **One channel per command.** There is no mask API in this handoff.
- Setting channel N **MUST NOT** change the coil state of any other channel.
- `485_CTR` stays at bootstrap inactive (`0`) and is **not** addressable here.

JSON types:

- `channel`: JSON number, integer value `1`…`4`
- `on`: JSON boolean (`true` = energize coil, `false` = release)

## Shared single-channel result object

```json
{
  "channel": 1,
  "on": true
}
```

Used by `relay_set` and `relay_get` success `result`.

## Command: `relay_set`

Set **one** relay coil on or off. Does not affect other coils.

### Request — energize K1

```json
{
  "id": "r-on-1",
  "name": "relay_set",
  "args": { "channel": 1, "on": true }
}
```

### Request — release K1

```json
{
  "id": "r-off-1",
  "name": "relay_set",
  "args": { "channel": 1, "on": false }
}
```

| Args field | Type | Required | Rules |
| --- | --- | --- | --- |
| `channel` | int | Yes | Exactly `1`…`4` |
| `on` | bool | Yes | `true` = energize, `false` = release |

Unknown top-level arg keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "r-on-1",
  "api_version": 1,
  "result": {
    "channel": 1,
    "on": true
  }
}
```

`result` **MUST** echo the applied channel shadow state (post-set).

### Errors (outer envelope)

| Condition | `error.code` |
| --- | --- |
| Missing/invalid `channel` or `on`; wrong JSON types; channel∉1..4 | `bad_request` |
| Host API not active | `unavailable` |
| `board_ctrl` not ready / unexpected GPIO fault | `internal` |

## Command: `relay_get`

Read **one** channel software shadow (not contact sense hardware).

### Request

```json
{
  "id": "r-get-1",
  "name": "relay_get",
  "args": { "channel": 1 }
}
```

| Args field | Type | Required | Rules |
| --- | --- | --- | --- |
| `channel` | int | Yes | `1`…`4` |

Unknown keys → `bad_request`.

### Success

```json
{
  "ok": true,
  "id": "r-get-1",
  "api_version": 1,
  "result": {
    "channel": 1,
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

## Prerequisite: `/KOE` must already be enabled

Relays **cannot** switch unless the 74HCT541 buffer is output-enabled.

| Signal | Required state during `relay_set` | GPIO level |
| --- | --- | --- |
| `/KOE` (`BOARD_CTRL_OE_GPIO`) | Buffer **enabled** | **0** (active-low) |

How that state is reached:

1. `board_ctrl_init()` (already required before I2C/WiFi) **MUST** end with
   `/KOE` driven **active** (low) after level-before-direction cold CTR/DE
   preload — see `docs/board_gpio_bringup_architecture.md` step 5.
2. `board_ctrl_relay_set` / `relay_get` **MUST** require
   `board_ctrl_is_ready() == true` (init completed ⇒ OE already enabled).
3. `relay_set` **MUST NOT** disable `/KOE`, re-run the OE claim sequence, or
   pulse OE. It only changes the selected `KnCTR` line while OE stays enabled.
4. If OE were left inactive (high), CTR edges stay behind Hi-Z `U8` outputs →
   **no coil click** — treat that as an init bug, not something `relay_set`
   should “fix” by toggling OE ad hoc.

## `board_ctrl` C API extension

Extend `components/board_ctrl/` (keep existing init / level-before-direction).

```c
/** channel 1..4; on true=energize. Other channels unchanged.
 *  Requires board_ctrl_init() already done (/KOE enabled). */
esp_err_t board_ctrl_relay_set(int channel, bool on);

/** Read software shadow for channel 1..4. */
esp_err_t board_ctrl_relay_get(int channel, bool *on_out);
```

Semantics:

| Rule | Requirement |
| --- | --- |
| Init / OE | Both APIs **MUST** fail with `ESP_ERR_INVALID_STATE` if `!board_ctrl_is_ready()` (implies `/KOE` already enabled by init) |
| Mutex | Take the same mutex as init for set/get |
| Shadow | Maintain `bool shadow[4]` (index `channel-1`); init all `false` |
| GPIO | `on==true` → CTR level `BOARD_RELAY_CTR_ACTIVE_LEVEL` (1); `false` → 0 |
| Isolation | `board_ctrl_relay_set(ch, on)` **MUST** drive only that channel’s CTR GPIO |
| OE | **MUST** leave `/KOE` enabled (active low). **MUST NOT** set OE inactive, re-init OE, or pulse OE around set/get |
| `485_CTR` | **MUST NOT** change in these APIs |

## Host API wiring

- Register `relay_set` / `relay_get` in `host_api_command.c` dispatch.
- Call `board_ctrl_relay_set` / `board_ctrl_relay_get` directly from handlers
  (`host_api` **REQUIRES** `board_ctrl` in CMake — same pattern as `b03_hil`
  `host_api` → `relay_ctrl`, avoids `app_core` cycles).
- Do **not** invent alternate command names (`relay_on`, `k1_set`, etc.).

## `capabilities` manifest

`capabilities.summary` **MUST** be exactly:

```text
Isolated RS-485/TTL HIL board with single-channel relay test API
```

`commands` **MUST** include mandatory `ping`, `info`, `capabilities`, plus:

### Descriptor: `relay_set`

```json
{
  "name": "relay_set",
  "summary": "Energize or release exactly one relay coil (K1..K4)",
  "args": [
    {
      "name": "channel",
      "type": "int",
      "required": true,
      "notes": "Relay channel 1..4 (K1 mux, K2 term, K3 PD, K4 PU)"
    },
    {
      "name": "on",
      "type": "bool",
      "required": true,
      "notes": "true=energize coil, false=release; other channels unchanged"
    }
  ],
  "result": [
    { "name": "channel", "type": "int", "notes": "Echo channel" },
    { "name": "on", "type": "bool", "notes": "Applied shadow state" }
  ]
}
```

### Descriptor: `relay_get`

```json
{
  "name": "relay_get",
  "summary": "Read one relay channel software shadow",
  "args": [
    {
      "name": "channel",
      "type": "int",
      "required": true,
      "notes": "Relay channel 1..4"
    }
  ],
  "result": [
    { "name": "channel", "type": "int", "notes": "Echo channel" },
    { "name": "on", "type": "bool", "notes": "Shadow state" }
  ]
}
```

`streams` **MUST** remain `[]`.

## Human tester examples (after STA + host API up)

Replace host with the board mDNS name or STA IPv4.

```bash
# Energize K1 only
curl -sS -X POST "http://HIL-02-XXXX.local/v1/command" \
  -H 'Content-Type: application/json' \
  -d '{"id":"1","name":"relay_set","args":{"channel":1,"on":true}}'

# Release K1 only
curl -sS -X POST "http://HIL-02-XXXX.local/v1/command" \
  -H 'Content-Type: application/json' \
  -d '{"id":"2","name":"relay_set","args":{"channel":1,"on":false}}'

# Read shadow
curl -sS -X POST "http://HIL-02-XXXX.local/v1/command" \
  -H 'Content-Type: application/json' \
  -d '{"id":"3","name":"relay_get","args":{"channel":1}}'
```

Repeat `channel` `2`…`4` for K2…K4. Expect audible click / continuity change
only on the selected relay.

## Acceptance criteria

1. After boot, `/KOE` is enabled (from `board_ctrl_init`) before any
   `relay_set`; sets do not disable or pulse OE.
2. `relay_set` with `on:true` / `on:false` for each of channels 1…4 works
   electrically (coil click) — which requires OE already active.
3. Setting one channel does not clear or set any other channel’s coil.
4. No mask / `set_all` commands exist in dispatch or `capabilities`.
5. `relay_get` matches last `relay_set` shadow for that channel.
6. Cold boot still uses level-before-direction; all shadows start `false`.
7. `capabilities.summary` exact string above; catalog lists both commands.
8. Human can exercise relays via `POST /v1/command` after WiFi onboarding.

## Non-goals

- Multi-channel atomic updates
- DE / UART host control
- Building/flashing as implementer (tester / human)
