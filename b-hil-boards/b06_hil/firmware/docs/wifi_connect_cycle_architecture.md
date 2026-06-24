# WiFi Connect Cycle Architecture (v2)

Normative contract for STA connect/reconnect when credentials exist in NVS.
Source handoff: `WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2`.

Supersedes for connect policy and terminal failure behavior:

- Saved boot lock (`SAVED_FAILURE_LOCKED`, `s_locked_disconnected`)
- Split boot vs runtime reconnect policies in `docs/wifi_runtime_reconnect_architecture.md`

Related:

- **`docs/wifi_connect_cycle_implementation_reference.md`** — **as-built source of truth**
  (function names, static state, boot-blocking vs runtime task, exact logs, anti-patterns)
- `docs/error_led_connect_cycle_architecture.md` — LED mapping v2
- `docs/wifi_provisioning_architecture.md` — event model and product table
- `docs/error_led_wifi_link_architecture.md` — v1 LED mapping (superseded)

## Purpose

When valid WiFi credentials are stored in NVS, the device MUST keep trying to
obtain a STA IP **indefinitely** using a repeating **connect cycle**. There is no
terminal locked failure, no OLED `HOLD RESET` screen, and no credential erase due
to network outage.

The same cycle applies on **cold boot** (`connect_saved`) and **runtime link
loss** (`runtime_reconnect_task` path).

## Product rules

| Situation | WiFi / OLED | Error LED (see LED doc) |
| --- | --- | --- |
| No credentials in NVS | Provisioning portal UX | Solid ON |
| Credentials present, connect cycle active | `WIFI` / `CONNECTING` | Slow blink |
| Round of 5 attempts failed | `WIFI` / `CONNECTING` (no HOLD RESET) | Fast blink 15 s |
| After 15 s alert | New round; same OLED | Slow blink |
| STA has IP | `WIFI OK` + IP + MAC | OFF |
| Factory reset GPIO7 2 s | Erase creds; portal | Solid ON |

Cross-cutting rules:

- MUST NOT emit `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED` in v2.
- MUST NOT set `s_locked_disconnected` on credential connect paths.
- MUST NOT reopen provisioning AP automatically while credentials remain in NVS.
- MUST NOT erase credentials because connect cycles fail.

## Connect cycle (normative)

### Constants

```text
CONNECT_ROUND_MAX_ATTEMPTS: 5
CONNECT_PER_ATTEMPT_TIMEOUT_MS: 12000
CONNECT_ALERT_PHASE_MS: 15000
CONNECT_BOOT_SETTLE_MS: 1000   /* boot only, after esp_wifi_start before first round */
CONNECT_BACKOFF_WITHIN_ROUND_MS: [0, 1000, 3000, 5000, 8000]
```

Definitions:

- **Round**: up to 5 calls to `run_sta_connect_attempt()` without `GOT_IP`.
- **Cycle**: repeating sequence `CONNECTING round` → optional `CONNECTING_ALERT` →
  next round until success or factory reset.

### Cycle algorithm

```text
1. link_status = CONNECTING
   emit WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE (once per round start)
   log: connect cycle round start source=<boot|runtime>

2. For attempt = 1 .. CONNECT_ROUND_MAX_ATTEMPTS:
     run_sta_connect_attempt(attempt, CONNECT_PER_ATTEMPT_TIMEOUT_MS, source)
     if success (GOT_IP):
       link_status = CONNECTED
       emit WIFI_PROV_EVENT_CONNECT_RESTORED with sta_ipv4/sta_mac
       exit cycle
     backoff CONNECT_BACKOFF_WITHIN_ROUND_MS[attempt-1] if attempt < 5

3. Round failed:
   link_status = CONNECTING_ALERT
   emit WIFI_PROV_EVENT_CONNECT_ALERT_PHASE
   log: connect cycle alert_phase_ms=15000 source=<boot|runtime>
   delay CONNECT_ALERT_PHASE_MS

4. Goto step 1 (unbounded)
```

### `connect_cycle_run_forever` exit conditions

The outer loop returns (stops cycling) when:

1. A round succeeds (`CONNECT_RESTORED` emitted).
2. `s_portal_active` becomes true (portal opens during cycle).
3. `wifi_credentials_load` fails (credentials erased, e.g. factory reset).

Otherwise the loop repeats unbounded (round → alert → round).

### Boot blocking vs runtime task

| Path | Execution context |
| --- | --- |
| Boot (`connect_saved`) | `connect_cycle_run_forever(SAVED)` runs in the **same task** as `app_core_wifi_start()` — blocks caller until success or exit above |
| Runtime | `handle_runtime_link_loss()` creates task `wifi_rt_rc` (stack 4096, priority 5); task calls `connect_cycle_run_forever(RUNTIME)` |

Do **not** run boot connect in a new task. Do **not** run runtime connect synchronously
in the WiFi event handler.

`apply_sta_config()` runs **once** before the cycle on boot (`connect_saved`, after
`esp_wifi_start` and `CONNECT_BOOT_SETTLE_MS`) and **once** at `runtime_reconnect_task`
start — not before every round.

### Sources (logging only)

Internal `wifi_prov_sta_source_t` values for serial logs:

```text
boot    — connect_saved() cold start path
runtime — runtime_reconnect_task after link loss
```

Both sources MUST execute the **same** cycle algorithm above.

## Triggers

| Entry | When |
| --- | --- |
| Boot | `wifi_provisioning_connect_saved()` after NVS load |
| Runtime | Link loss while `CONNECTED`, portal inactive, credentials in NVS |

Runtime trigger guards (unchanged intent from layer 1):

- Do not start a second reconnect task if one is active.
- Do not start during portal activity or active saved-boot STA ownership.
- Deduplicate `STA_DISCONNECTED` and `STA_LOST_IP`.

## Domain model: `wifi_link_status_t` (v2)

```c
typedef enum {
    WIFI_LINK_STATUS_UNPROVISIONED = 0,
    WIFI_LINK_STATUS_CONNECTING,
    WIFI_LINK_STATUS_CONNECTING_ALERT,
    WIFI_LINK_STATUS_CONNECTED,
    /* WIFI_LINK_STATUS_DISCONNECTED — deprecated v2; do not set on creds path */
} wifi_link_status_t;
```

| Value | Meaning |
| --- | --- |
| `UNPROVISIONED` | No valid credentials in NVS |
| `CONNECTING` | Credentials present; round attempts in progress |
| `CONNECTING_ALERT` | Post-round fast LED phase (15 s); not terminal |
| `CONNECTED` | STA has assigned IP |

## Event model (v2)

Add to `wifi_provisioning.h`:

```c
WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE,
WIFI_PROV_EVENT_CONNECT_ALERT_PHASE,
WIFI_PROV_EVENT_CONNECT_RESTORED,
```

### `WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE`

- Emitted at the start of each connect round (boot or runtime).
- `link_status`: `WIFI_LINK_STATUS_CONNECTING`.
- `ssid`: home network SSID (MAY be non-NULL).
- `sta_ipv4`, `sta_mac`: NULL.

### `WIFI_PROV_EVENT_CONNECT_ALERT_PHASE`

- Emitted when a round exhausts 5 attempts without IP.
- `link_status`: `WIFI_LINK_STATUS_CONNECTING_ALERT`.
- `ssid`: home network SSID (MAY be non-NULL).
- `sta_ipv4`, `sta_mac`: NULL.

### `WIFI_PROV_EVENT_CONNECT_RESTORED`

- Emitted on successful `GOT_IP` during any round.
- `link_status`: `WIFI_LINK_STATUS_CONNECTED`.
- `sta_ipv4`, `sta_mac`: same rules as `SAVED_SUCCESS` / `RUNTIME_RESTORED`.

### Deprecated emissions (v2)

Do **not** emit:

- `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED`
- Terminal `WIFI_LINK_STATUS_DISCONNECTED` on credential connect failure

Enum values MAY remain for incremental migration; `app_core_wifi` MUST NOT show
`HOLD RESET` for any v2 path.

### Events emitted by `wifi_provisioning` (as-built)

| Situation | Event emitted | `link_status` |
| --- | --- | --- |
| Round start | `CONNECT_CYCLE_ACTIVE` | `CONNECTING` |
| 5 failures in round | `CONNECT_ALERT_PHASE` | `CONNECTING_ALERT` |
| GOT_IP in round | `CONNECT_RESTORED` | `CONNECTED` |
| Portal POST connecting | `SUBMITTED_CONNECTING` | `UNPROVISIONED` |
| Portal POST success | `SUBMITTED_SUCCESS` | `CONNECTED` |

**Do not emit on connect-cycle paths:** `SAVED_CONNECTING`, `SAVED_SUCCESS`,
`SAVED_FAILURE_LOCKED`, `RUNTIME_RECONNECTING`, `RUNTIME_RESTORED`.

### OLED legacy handlers in `app_core_wifi.c` (as-built)

`wifi_provisioning` emits v2 events only; `app_core_wifi` keeps grouped `switch`
cases for enum compatibility:

| Events handled (any of) | OLED |
| --- | --- |
| `SAVED_CONNECTING`, `RUNTIME_RECONNECTING`, `CONNECT_CYCLE_ACTIVE`, `CONNECT_ALERT_PHASE` | `WIFI` / `CONNECTING` |
| `SAVED_SUCCESS`, `RUNTIME_RESTORED`, `CONNECT_RESTORED`, `SUBMITTED_SUCCESS` | WiFi connected screen |
| `SAVED_FAILURE_LOCKED` | **empty `break`** — no HOLD RESET |

`LINK_STATUS_CHANGED` is secondary (stale GOT_IP path); cycle LED uses
`CONNECT_CYCLE_*` events with `link_status`.

## Display integration (`app_core_wifi.c`)

| Event | OLED |
| --- | --- |
| Portal / no creds | Provisioning setup UX (unchanged) |
| `CONNECT_CYCLE_ACTIVE` | `WIFI` / `CONNECTING` |
| `CONNECT_ALERT_PHASE` | `WIFI` / `CONNECTING` |
| `CONNECT_RESTORED` | `WIFI OK` + IP + split MAC |
| `SUBMITTED_CONNECTING` | `WIFI` / `CONNECTING`; `link_status` = `UNPROVISIONED` → LED solid ON |

Remove or disable OLED branch for `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED`.

Portal `SUBMITTED_CONNECTING`: credentials not yet saved → `UNPROVISIONED` → LED
solid ON (not slow).

## Required serial logs

```text
wifi_prov: connect cycle policy round_attempts=5 per_attempt_timeout_ms=12000 alert_phase_ms=15000
wifi_prov: connect cycle round start source=boot
wifi_prov: connect cycle attempt 1/5 start source=boot
wifi_prov: STA disconnected reason=<reason> source=boot attempt=1/5
wifi_prov: connect cycle attempt 1/5 failed reason=<reason> backoff_ms=<ms> source=boot
wifi_prov: connect cycle alert_phase_ms=15000 source=boot
wifi_prov: connect cycle round start source=boot
wifi_prov: STA got IP ip=<ipv4> source=boot attempt=<n>/5
wifi_prov: connect cycle restored ip=<ipv4> source=boot
```

Runtime logs use `source=runtime`. MUST NOT log `saved boot failed attempts=5`.

## Module boundaries

- `wifi_provisioning` owns cycle state machine and `s_link_status`.
- `app_core_wifi` maps events to OLED and LED via `link_status` only.
- `wifi_provisioning` MUST NOT include `error_led.h` or display headers.

## Obsolete behavior (explicit non-goals v2)

- Boot lock after 5 failures (`SAVED_FAILURE_LOCKED`).
- OLED `WIFI` / `FAILED` / `HOLD` / `RESET`.
- Infinite runtime reconnect with LED solid ON only (replaced by slow/fast cycle).
- Opening provisioning AP when credentials exist but network is down.

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `CONNECTING_ALERT`, v2 events |
| `components/wifi_provisioning/wifi_provisioning.c` | Unified cycle engine; remove lock |
| `components/app_core/app_core_wifi.c` | v2 OLED + LED table; remove HOLD RESET |

## Acceptance criteria

1. Empty NVS: LED solid ON; portal OLED.
2. Credentials + AP up: slow blink → off on connect; WIFI OK screen.
3. Credentials + AP down (boot or runtime): slow during attempts; after 5 failures
   fast 15 s; returns slow; repeats; **never** HOLD RESET.
4. AP returns during cycle: connects; LED off.
5. Factory reset: credentials erased; LED solid ON; portal.
6. Serial shows round/alert logs; no `saved boot failed attempts=5`.

## Validation plan

- Implementer: `idf.py build`; log one full failed round + alert + second round.
- Tester: AP off/on; cold boot with AP down; photograph OLED and LED per phase.
