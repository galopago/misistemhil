# External Host Command API (Slow Commands)

Normative contract for **slow, synchronous** host→board commands over HTTP JSON.

Parent: [`docs/external_host_api_architecture.md`](external_host_api_architecture.md).  
Source handoff: `EXTERNAL_HOST_API_V1_CONCEPT`,
`HOST_API_CAPABILITIES_ECHO_LOOPBACK`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

Give an **LLM agent** (primary client) a single, boring, predictable way to:

1. Send a named command with JSON arguments.
2. Wait for one JSON response (success or error).
3. Extend the command set per board without changing transport.

Binary frame I/O uses the same HTTP style via
[`docs/external_host_stream_api.md`](external_host_stream_api.md)
(`write`/`read` + Base64), not a raw socket.

## Endpoint

```text
POST /v1/command
Content-Type: application/json
```

Active only when the STA host API is up (STA + IPv4). Base URL example:

```text
http://HIL-00-24CE.local/v1/command
```

## Request Schema

```json
{
  "id": "req-001",
  "name": "ping",
  "args": {}
}
```

| Field | Type | Required | Rules |
| --- | --- | --- | --- |
| `id` | string | Yes | Non-empty; echoed in the response. Host-chosen correlation id. Max 64 chars ASCII. |
| `name` | string | Yes | Stable command name (`[a-z0-9_]+`). Max 64 chars. |
| `args` | object | Yes | JSON object (may be empty `{}`). No arrays at top level in v1. |

Invalid JSON or missing required fields → `ok: false`, `code: bad_request`.

## Success Response Schema

```json
{
  "ok": true,
  "id": "req-001",
  "api_version": 1,
  "result": {}
}
```

| Field | Type | Rules |
| --- | --- | --- |
| `ok` | boolean | Must be `true` |
| `id` | string | Echo of request `id` |
| `api_version` | number | Always `1` in v1 |
| `result` | object | Command-specific; may be empty |

## Error Response Schema

```json
{
  "ok": false,
  "id": "req-001",
  "error": {
    "code": "unknown_command",
    "message": "unknown command name=foo"
  }
}
```

`id` SHOULD echo the request id when it was parsed; otherwise `null`.

Error codes: see parent doc (shared control-plane table).

## Timing (Synchronous v1)

| Parameter | Value |
| --- | --- |
| Server handler timeout | **5000 ms** |
| Semantics | One HTTP request → one response; no async job id in v1 |

If the handler exceeds 5000 ms, respond with `ok: false`, `code: timeout`.
Long-running work is a **future** extension (not v1).

Only one command executes at a time on the device in v1. A second concurrent
`POST /v1/command` MAY return `busy`.

## Mandatory Base Commands

Every board that enables the host API MUST register these names.

### `ping`

Request:

```json
{ "id": "1", "name": "ping", "args": {} }
```

Success `result`:

```json
{ "pong": true }
```

### `info`

Request:

```json
{ "id": "2", "name": "info", "args": {} }
```

Success `result` (field set is normative; values are examples):

```json
{
  "api_version": 1,
  "board_id": "b00_basehil",
  "board_number": "00",
  "identity": "HIL-00-24CE",
  "sta_ipv4": "192.168.1.42",
  "streams_supported": false
}
```

| Field | Meaning |
| --- | --- |
| `api_version` | Host API major version (`1`) |
| `board_id` | Project / folder style id string |
| `board_number` | Two-digit board number |
| `identity` | Same as SoftAP/mDNS hostname (no `.local`) |
| `sta_ipv4` | Current STA IPv4 string, or `""` if unavailable |
| `streams_supported` | `true` if this firmware can open at least one binary stream |

Particular boards MAY add extra keys under `result` for `info` only if documented
in `board_host_api_extensions.md` and prefixed or clearly board-specific (do not
remove mandatory keys).

### `capabilities` (ask the board)

Runtime auto-description. The agent discovers what the board can do by **asking
the device on the LAN**, not by reading the git repository.

Request:

```json
{ "id": "3", "name": "capabilities", "args": {} }
```

Success `result` shape (normative fields). For **`b00_basehil`**, the catalog
MUST match the exact JSON in architect handoff
`HOST_API_CAPABILITIES_ECHO_LOOPBACK` (includes `echo` + `loopback`). Abbreviated
shape:

```json
{
  "api_version": 1,
  "board_id": "b00_basehil",
  "summary": "Family base HIL bench with host API demos (echo, loopback)",
  "commands": [ "…ping, info, capabilities, echo…" ],
  "streams": [ "…loopback…" ]
}
```

| Field | Meaning |
| --- | --- |
| `api_version` | Host API major version (`1`) |
| `board_id` | Same style as `info` |
| `summary` | 1–2 English sentences: what this board is for |
| `commands` | Array of command descriptors (MUST include `ping`, `info`, `capabilities`) |
| `streams` | Array of stream descriptors (may be empty) |

Each **command** descriptor:

| Field | Type | Rules |
| --- | --- | --- |
| `name` | string | Registered `name` for `POST /v1/command` |
| `summary` | string | Short English purpose |
| `args` | array | List of `{ "name", "type", "required"?, "notes"? }`; empty if none |
| `result` | array | List of `{ "name", "type", "notes"? }` describing success `result` keys |

Each **stream** descriptor:

| Field | Type | Rules |
| --- | --- | --- |
| `stream_id` | string | Registered id for `POST /v1/stream/open` |
| `summary` | string | Short English purpose |
| `params` | array | Same field-list style as command `args` (for open `params`) |
| `payload` | string | English note on frame meaning (host↔board) |

Field `type` values in v1: `bool`, `int`, `string`, `object` (not full JSON Schema).

Rules:

1. The manifest MUST be served from **firmware** (static table or equivalent).
2. Agents MUST treat this response as the runtime catalog; repo Markdown is not
   the discovery channel.
3. Typical response budget: under ~4 KiB JSON. Prefer shorter `notes` over
   pagination (no pagination in v1).
4. `streams` empty iff no openable stream; then `info.streams_supported` is
   `false`.

## Reference-board demo command (`b00_basehil` only)

Not mandatory for sibling boards. Documented here so the family base can exercise
the command path without specialty hardware. Listed in
`docs/board_host_api_extensions.md` and in this board's `capabilities` manifest.

### `echo`

Request:

```json
{ "id": "4", "name": "echo", "args": { "text": "hello" } }
```

| Args field | Type | Required | Rules |
| --- | --- | --- | --- |
| `text` | string | Yes | ASCII/UTF-8 text; max **128** bytes |

Success `result`:

```json
{ "text": "hello" }
```

Errors: `bad_request` if `text` missing, wrong type, or longer than 128 bytes.

## Particular Commands

Boards register additional `name` values in
`docs/board_host_api_extensions.md`. Those names MUST also appear in the
firmware `capabilities` manifest for that board.

```text
Command name: relay_set
Args: { "channel": <int>, "on": <bool> }
Result: { "channel": <int>, "on": <bool> }
Errors: bad_request, busy, ...
```

Particular names MUST NOT collide with base `ping` / `info` / `capabilities`.
Prefer board-prefixed names when ambiguous (`b06_uart_write`).

## Agent discovery sequence

1. `ping` — liveness.
2. `info` — identity + `streams_supported`.
3. `capabilities` — full actionable catalog from the board.
4. Issue particular commands / stream open using names from the manifest only.

## LLM / curl Examples

```bash
curl -sS -X POST "http://HIL-00-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"1","name":"ping","args":{}}'
```

```bash
curl -sS -X POST "http://HIL-00-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"2","name":"info","args":{}}'
```

```bash
curl -sS -X POST "http://HIL-00-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"3","name":"capabilities","args":{}}'
```

```bash
curl -sS -X POST "http://HIL-00-24CE.local/v1/command" \
  -H "Content-Type: application/json" \
  -d '{"id":"4","name":"echo","args":{"text":"hello"}}'
```

## Non-Goals

- Streaming request/response bodies for command payloads
- Binary args (use stream `write`/`read` with Base64)
- Batching multiple commands in one HTTP body
- Authentication headers in v1
- Serving capability text from the git repo to the agent
- Full OpenAPI / JSON Schema drafts inside `capabilities`
