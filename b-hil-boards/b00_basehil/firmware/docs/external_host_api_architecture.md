# External Host API Architecture

Normative home for the **family-wide communication methodology** between an
external entity and each HIL board.

Source handoff: `agent-workspaces/architect/handoff.md`,
`EXTERNAL_HOST_API_V1_CONCEPT` (agent-primary),
`HOST_API_CAPABILITIES_ECHO_LOOPBACK` (ask-the-board + base demos).

Pillar ownership: **Base pillar 3** in `docs/hil_base_platform_scope.md`.

Sibling contracts (read with this file):

- [`docs/external_host_command_api.md`](external_host_command_api.md) — slow
  command / response
- [`docs/external_host_stream_api.md`](external_host_stream_api.md) — binary
  frame exchange over HTTP JSON (agent-bounded)

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Status

**v1 concept — normative structure fixed; firmware not yet implemented.**

This document and its siblings define the family host↔board methodology.
Implementer work is pending under `EXTERNAL_HOST_API_V1_CONCEPT`. Until firmware
ships, hosts must not assume the endpoints exist on device.

Derivative boards MUST NOT invent a competing family-wide host protocol.
Board-only commands and stream IDs go in `board_host_api_extensions.md`.

Material changes append a `BASE-CHANGE-…` entry to
`docs/hil_base_spec_changelog.md`.

## Primary Client (normative)

The **primary client is an LLM agent** (tool-calling / discrete HTTP turns).

Design implications that are normative for v1:

- Every interaction the agent needs MUST be expressible as a **short, bounded
  HTTP request/response** (curl-shaped). No long-lived raw socket held open in
  the agent terminal.
- Binary payloads travel as **Base64 inside JSON**, with documented size limits
  and read timeouts, so the agent can send/receive one or a few frames per turn.
- Secondary clients (scripts, dashboards) MAY use the same HTTP surface; they
  are not the design center.

## Mission

After WiFi onboarding and network detection succeed, an agent must be able to
address a board via its discovery identity and:

1. **Ask the board what it can do** — `POST /v1/command` name `capabilities`
   returns a firmware-embedded manifest (not git docs).
2. **Slow commands** — request/response over HTTP JSON.
3. **Binary frames** — open a named stream session, then **write/read frames**
   with bounded HTTP calls, still over HTTP JSON.

Board-particular product features are **extensions** of this contract, not
replacements.

## Single Transport Surface (v1)

```mermaid
flowchart LR
  Agent["LLM agent"] -->|"HTTP JSON /v1"| HostApi["host_api"]
  HostApi --> AppCore["app_core"]
  AppCore --> BoardExt["board particular drivers"]
  Mdns["HIL-NN-MAC4.local"] --> Agent
```

| Surface | Purpose | Transport v1 |
| --- | --- | --- |
| **Commands** | Slow request/response (`ping`, `info`, `capabilities`, particular) | `POST /v1/command` |
| **Stream session** | Open/close named binary stream | `POST /v1/stream/open\|close` |
| **Stream frames** | Agent-bounded binary I/O | `POST /v1/stream/write\|read` |

### Agent discovery (normative)

```text
ping → info → capabilities → (commands / stream/* using manifest names only)
```

The repository is **not** the runtime discovery channel for agents.

Why this is agent-friendly:

- One stack: HTTP + JSON (+ Base64 for opaque bytes).
- Each agent tool call completes; nothing collapses a terminal on a blocking
  TCP accept/read loop.
- Framing semantics (max payload, session rules) stay explicit without a second
  protocol.

## When the Host API Is Active

| Condition | Host API |
| --- | --- |
| STA connected with IPv4 on user LAN, SoftAP portal httpd **not** bound | **Available** |
| Provisioning SoftAP / portal httpd on port 80 | **Unavailable** |
| Link lost / no STA IP | **Unavailable** |
| Factory reset → portal | Stop host API with SoftAP transition |

### Port 80 exclusivity (normative)

SoftAP portal httpd and STA `host_api` httpd **MUST NOT** bind port **80** at the
same time. They are separate `httpd_handle_t` instances.

**Failure mode (Run 029 / `TESTER_RETURN_HOST_API_AFTER_PROVISION`):** on
`POST /provision` success, link becomes `CONNECTED` (OLED/mDNS) while SoftAP
httpd still owns `:80` during the teardown grace window. An early
`host_api_start()` fails to bind; if the error is discarded and nothing starts
host API **after** SoftAP httpd stops, STA+mDNS look healthy but `/v1` is dead
until reboot. Saved-credential cold boot has no SoftAP httpd, so it can PASS
while the fresh-POST path FAILs.

**Required order after successful submitted provision:**

1. Keep SoftAP httpd up long enough for the success HTML (existing 2000 ms grace).
2. Stop SoftAP httpd and leave STA-only mode (existing teardown task).
3. Emit `WIFI_PROV_EVENT_PORTAL_TEARDOWN_COMPLETE` (see WiFi provisioning docs).
4. `app_core` starts `host_api` on that event while link is still `CONNECTED`.

`host_api_start()` from the initial `CONNECTED` transition MAY fail with a
**WARN** while SoftAP httpd is still up; that is expected on the POST path. The
**authoritative** start for that path is step 4. When SoftAP portal is already
down (saved boot), the `CONNECTED` path start MUST succeed.

Do not leave `(void)host_api_start()` as the only attempt on the POST-success
path.

Hosts locate the board with the existing discovery identity
(`docs/device_discovery_mdns_architecture.md`):

```text
http://HIL-<board>-<MAC4>.local/v1/...
```

Example: `http://HIL-00-24CE.local/v1/command`.

IPv4 literals from OLED/serial are also valid when mDNS is unavailable on the
client OS.

## Transport and Versioning

| Item | v1 value |
| --- | --- |
| Scheme | HTTP/1.1 |
| Port | **80** (default HTTP) |
| API path prefix | `/v1/` |
| JSON field | `"api_version": 1` on responses that include version metadata |
| Binary encoding in JSON | **Base64** (standard alphabet, no newlines) |
| Auth | **None** on LAN in v1 |
| TLS / HTTPS | Non-goal v1 |

Provisioning SoftAP HTTP (`192.168.4.1` portal) remains a **separate** stack and
MUST NOT share handlers or sockets with the STA host API.

## Module Boundaries

Conceptual component: `host_api` (HTTP `/v1` server only in v1).

| Module | May do | Must not |
| --- | --- | --- |
| `app_core` | Start/stop host API with WiFi/mDNS lifecycle; dispatch to handlers | Embed board UART bit-banging |
| `host_api` | Bind HTTP `/v1/*`, JSON + Base64, timeouts, errors | Include display headers; own WiFi credentials; open a raw data TCP port in v1 |
| `wifi_provisioning` | Unrelated SoftAP portal | Serve `/v1` host API |
| Board particular drivers | Register command names / stream backends | Redefine HTTP paths or frame size/encoding rules |
| Display stack | Unrelated | Depend on `host_api` |

`host_api` MUST NOT include WiFi provisioning portal page code. Display delivery
rules in `docs/display_delivery_contract.md` remain unchanged.

## Mandatory Base Surface vs Particular Extensions

**Every family board that ships the host API MUST implement:**

- Control listen when STA has IP.
- Commands `ping`, `info`, and `capabilities` (see command API doc).
- Stream endpoints `open` / `close` / `write` / `read` (even if the board has
  zero streams: unknown `stream_id` returns a stable error).

**Reference board `b00_basehil` (demos, not family-mandatory):**

- Command `echo` and stream `loopback` so agents can exercise command + stream
  paths without specialty DUT hardware. Specified in command/stream docs and
  `board_host_api_extensions.md`. Sibling boards do **not** inherit these unless
  they opt in.

**Particular boards MAY add:**

- Extra `name` values under `POST /v1/command`.
- Extra `stream_id` values and payload semantics under the stream API.
- Human checklist in `board_host_api_extensions.md`; runtime truth is always
  the on-device `capabilities` manifest.

## Error Model

All JSON error responses use:

```json
{
  "ok": false,
  "id": "<echo request id or null>",
  "error": {
    "code": "bad_request",
    "message": "human-readable ASCII detail"
  }
}
```

Stable `code` values (v1):

| Code | Meaning |
| --- | --- |
| `bad_request` | Malformed JSON, missing fields, invalid types / Base64 |
| `unknown_command` | `name` not registered |
| `unknown_stream` | `stream_id` not registered |
| `timeout` | Handler or stream read exceeded timeout |
| `busy` | Resource locked (e.g. stream already open) |
| `unavailable` | Host API or feature not active in this mode |
| `internal` | Unexpected firmware failure |

HTTP status SHOULD be `200` with `"ok": false` for application errors so agents
can parse one JSON shape; `400`/`404`/`503` MAY be used when the HTTP layer
fails before a JSON body exists.

## Boundaries Table

| In base host API docs | Not in base host API docs |
| --- | --- |
| Transport, ports, encoding, versioning | Board pin maps |
| Mandatory `ping` / `info` / `capabilities` | Board-unique command semantics |
| Stream session + write/read contracts | UART baud, RS-485 timing, relay maps |
| Discovery URL binding; ask-the-board flow | Per-board validation evidence |
| Shape of `capabilities` manifest | Full product manuals in flash |

## Non-Goals (v1)

- LAN authentication, API keys, HTTPS/TLS
- Raw TCP data port (e.g. 9000) held open by the agent
- WebSocket, MQTT, gRPC, Protobuf, CBOR
- Async long-running job queue (commands are synchronous with a fixed timeout)
- Replacing mDNS with a custom discovery protocol
- Serving host API on the provisioning SoftAP
- Coupling host API into display or provisioning portal modules
- Defining RS-485 / relay / analyzer payloads here (particular extensions only)
- Agent discovery by reading the git repository
- OpenAPI / Swagger UI on device

**Deferred (not v1):** WebSocket or a native framed TCP plane may be added later
**as an optional high-throughput path** for non-agent tools. They MUST NOT replace
the agent HTTP write/read surface.

## Implementation Readiness Note

Firmware implementation is **pending** under `EXTERNAL_HOST_API_V1_CONCEPT` and
`HOST_API_CAPABILITIES_ECHO_LOOPBACK`. Architect deliverable is this
documentation set. Implementer must embed the `capabilities` manifest on device
and ship `echo` + `loopback` on `b00_basehil` per the sibling contracts.
