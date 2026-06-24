# WiFi Connect Cycle — Implementation Reference (As-Built)

Normative as-built record for reproducing the validated v2 connect cycle and LED
behavior. Source handoff: `WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2`. Operator
validation: 2026-06-23.

Policy overview: [`wifi_connect_cycle_architecture.md`](wifi_connect_cycle_architecture.md)

LED mapping: [`error_led_connect_cycle_architecture.md`](error_led_connect_cycle_architecture.md)

## Authorized source files

| File | Role |
| --- | --- |
| [`components/wifi_provisioning/wifi_provisioning.c`](../components/wifi_provisioning/wifi_provisioning.c) | Connect cycle engine, WiFi events, emissions |
| [`components/wifi_provisioning/include/wifi_provisioning.h`](../components/wifi_provisioning/include/wifi_provisioning.h) | Public enums and API |
| [`components/app_core/app_core_wifi.c`](../components/app_core/app_core_wifi.c) | OLED + LED adapter |

Do not reintroduce logic in other modules for connect cycle or error LED patterns.

## Compile-time constants (`wifi_provisioning.c`)

```c
#define CONNECT_ROUND_MAX_ATTEMPTS 5U
#define CONNECT_PER_ATTEMPT_TIMEOUT_MS 12000U
#define CONNECT_ALERT_PHASE_MS 15000U
#define CONNECT_BOOT_SETTLE_MS 1000U
```

Intra-round backoff array (index `attempt - 1` after failed attempt):

```c
static const uint32_t s_connect_backoff_within_round_ms[CONNECT_ROUND_MAX_ATTEMPTS] = {
    0U, 1000U, 3000U, 5000U, 8000U,
};
```

## Static state (connect cycle relevant)

| Symbol | Type | Purpose |
| --- | --- | --- |
| `s_link_status` | `wifi_link_status_t` | Product link state; copied into every `emit_event` |
| `s_connect_attempt` | `uint8_t` | Current attempt 1..5 during `run_sta_connect_attempt` |
| `s_pending_sta_source` | `wifi_prov_sta_source_t` | Active STA attempt source for event handler |
| `s_last_disconnect_reason` | `int` | Last `WIFI_EVENT_STA_DISCONNECTED` reason in round |
| `s_runtime_reconnect_task` | `TaskHandle_t` | Dedupes runtime reconnect; NULL when idle |
| `s_portal_active` | `bool` | Exit cycle if portal opens |
| `s_sta_ipv4` | `char[16]` | Cached STA IP on GOT_IP |

**Removed in v2 (must not reintroduce):** `s_locked_disconnected`,
`s_saved_policy_exhausted`.

## Internal STA source enum

```c
typedef enum {
    WIFI_PROV_STA_SOURCE_NONE = 0,
    WIFI_PROV_STA_SOURCE_SUBMITTED,
    WIFI_PROV_STA_SOURCE_SAVED,    /* logs as "boot" */
    WIFI_PROV_STA_SOURCE_RUNTIME,  /* logs as "runtime" */
} wifi_prov_sta_source_t;
```

`sta_source_label(WIFI_PROV_STA_SOURCE_SAVED)` returns **`"boot"`**, not `"saved"`.

## Call graph

```text
app_core_wifi_start()
  └─ wifi_provisioning_connect_saved()     [boot path]
       ├─ esp_wifi_set_mode/start, apply_sta_config
       ├─ vTaskDelay(CONNECT_BOOT_SETTLE_MS)
       └─ connect_cycle_run_forever(SAVED)  [blocks caller task]

wifi_event_handler STA_DISCONNECTED / STA_LOST_IP
  └─ handle_runtime_link_loss()
       └─ start_runtime_reconnect_if_needed()
            └─ xTaskCreate(runtime_reconnect_task, "wifi_rt_rc", 4096, …, 5, …)

runtime_reconnect_task()
  ├─ wifi_credentials_load
  ├─ apply_sta_config (once)
  └─ connect_cycle_run_forever(RUNTIME)

connect_cycle_run_forever(source)
  └─ while (true)
       └─ connect_cycle_run_round(source)
            ├─ emit CONNECT_CYCLE_ACTIVE, link_status CONNECTING
            ├─ for attempt 1..5: run_sta_connect_attempt
            │    └─ on OK: emit CONNECT_RESTORED, return true
            ├─ emit CONNECT_ALERT_PHASE, link_status CONNECTING_ALERT
            └─ vTaskDelay(CONNECT_ALERT_PHASE_MS); return false

run_sta_connect_attempt(attempt, timeout, source)
  ├─ s_connect_attempt = attempt; s_pending_sta_source = source
  ├─ esp_wifi_disconnect(); delay 100ms; esp_wifi_connect()
  └─ wait WIFI_CONNECTED_BIT | WIFI_STA_DISCONNECTED_BIT
```

## Boot vs runtime execution context

| Aspect | Boot (`SAVED`) | Runtime (`RUNTIME`) |
| --- | --- | --- |
| Entry | `wifi_provisioning_connect_saved()` | `runtime_reconnect_task` |
| Thread | **Same task as** `app_core_wifi_start()` (blocking) | FreeRTOS task `wifi_rt_rc` |
| `apply_sta_config` | In `connect_saved` before cycle | Once at task start |
| Pre-cycle delay | `CONNECT_BOOT_SETTLE_MS` (1000 ms) | None |
| Task stack / priority | N/A | 4096 bytes, priority 5 |
| Dedup guard | N/A | `s_runtime_reconnect_task != NULL` |

With AP down at boot, `connect_cycle_run_forever` loops in the **startup task**;
LED/OLED update via `emit_event` → `on_wifi_prov_event` callbacks.

## `connect_cycle_run_forever` exit conditions

Returns from the function (stops cycling) when:

1. `connect_cycle_run_round` returns **true** (success → `CONNECT_RESTORED`).
2. `s_portal_active` is true at loop head or inside round.
3. `wifi_credentials_load` fails (credentials erased, e.g. factory reset).

Otherwise loops unbounded (round → alert → round).

## `connect_cycle_run_round` behavior

1. `set_link_status(CONNECTING)`; `emit_event(CONNECT_CYCLE_ACTIVE, …)`.
2. Log: `connect cycle round start source=%s`.
3. For each attempt 1..5:
   - Re-check `s_portal_active` and `wifi_credentials_load`.
   - Log: `connect cycle attempt %u/5 start source=%s`.
   - `run_sta_connect_attempt(attempt, CONNECT_PER_ATTEMPT_TIMEOUT_MS, source)`.
   - On success: log `connect cycle restored ip=%s`; `emit_sta_success_event(CONNECT_RESTORED)`; return **true**.
   - On failure: log `connect cycle attempt %u/5 failed reason=%d backoff_ms=%lu source=%s`; backoff if attempt &lt; 5.
4. `set_link_status(CONNECTING_ALERT)`; `emit_event(CONNECT_ALERT_PHASE, …)`.
5. Log: `connect cycle alert_phase_ms=15000 source=%s`.
6. `vTaskDelay(CONNECT_ALERT_PHASE_MS)`; return **false**.

## Events emitted by `wifi_provisioning` (as-built)

| Situation | Event | `link_status` |
| --- | --- | --- |
| Round start (boot/runtime) | `WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE` | `CONNECTING` |
| Round exhausted (5 fails) | `WIFI_PROV_EVENT_CONNECT_ALERT_PHASE` | `CONNECTING_ALERT` |
| GOT_IP in round | `WIFI_PROV_EVENT_CONNECT_RESTORED` | `CONNECTED` |
| Portal POST trying STA | `WIFI_PROV_EVENT_SUBMITTED_CONNECTING` | `UNPROVISIONED` |
| Portal POST success | `WIFI_PROV_EVENT_SUBMITTED_SUCCESS` | `CONNECTED` |
| Portal / AP events | `AP_STARTED`, `PORTAL_READY`, etc. | per existing rules |

### Do NOT emit (v2 as-built)

| Event | Reason |
| --- | --- |
| `WIFI_PROV_EVENT_SAVED_CONNECTING` | Replaced by `CONNECT_CYCLE_ACTIVE` on boot |
| `WIFI_PROV_EVENT_SAVED_SUCCESS` | Replaced by `CONNECT_RESTORED` on boot |
| `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED` | No terminal lock; use alert + new round |
| `WIFI_PROV_EVENT_RUNTIME_RECONNECTING` | Replaced by `CONNECT_CYCLE_ACTIVE` |
| `WIFI_PROV_EVENT_RUNTIME_RESTORED` | Replaced by `CONNECT_RESTORED` |

### Secondary emission

`WIFI_PROV_EVENT_LINK_STATUS_CHANGED` — still emitted by `publish_link_status()`
when `update_runtime_link_status()` changes enum (e.g. stale GOT_IP path when
`s_pending_sta_source == NONE`). **Not** the primary LED driver for the connect
cycle; cycle events carry `link_status` for LED.

## `app_core_wifi.c` adapter (as-built)

### LED — `apply_wifi_link_status_led`

Called at the **end of every** `on_wifi_prov_event()` invocation.

| `wifi_link_status_t` | `error_led_pattern_t` |
| --- | --- |
| `UNPROVISIONED` | `ON` |
| `CONNECTING` | `SLOW_BLINK` |
| `CONNECTING_ALERT` | `FAST_BLINK` |
| `CONNECTED` | `OFF` |
| `default` (incl. deprecated `DISCONNECTED`) | `ON` |

Boot gap in `app_core_wifi_start()` before first callback:

- No credentials → `UNPROVISIONED` (ON).
- Has credentials → `CONNECTING` (slow) before `connect_saved` blocks.

### OLED — grouped `switch` cases

| Events (any of) | Display |
| --- | --- |
| `SAVED_CONNECTING`, `RUNTIME_RECONNECTING`, `CONNECT_CYCLE_ACTIVE`, `CONNECT_ALERT_PHASE` | `WIFI` / `CONNECTING` |
| `SAVED_SUCCESS`, `RUNTIME_RESTORED`, `CONNECT_RESTORED`, `SUBMITTED_SUCCESS` | `show_wifi_connected_display()` |
| `SAVED_FAILURE_LOCKED` | **empty `break`** — no HOLD RESET |

Legacy event cases remain for enum compatibility; `wifi_provisioning` does not
emit them on connect-cycle paths.

## Public API stubs

```c
bool wifi_provisioning_locked_disconnected(void)
{
    return false;  /* v2: never locked */
}
```

`WIFI_LINK_STATUS_DISCONNECTED` remains in `wifi_provisioning.h` with comment
`deprecated v2`; **must not** be assigned on credential connect paths.

## Required serial log strings (regression)

Exact format from implementation:

```text
wifi_prov: connect cycle policy round_attempts=5 per_attempt_timeout_ms=12000 alert_phase_ms=15000
wifi_prov: connect cycle round start source=boot
wifi_prov: connect cycle round start source=runtime
wifi_prov: connect cycle attempt 1/5 start source=boot
wifi_prov: connect cycle attempt 1/5 failed reason=202 backoff_ms=0 source=boot
wifi_prov: connect cycle alert_phase_ms=15000 source=boot
wifi_prov: connect cycle restored ip=192.168.x.x source=boot
wifi_prov: STA got IP ip=192.168.x.x source=boot attempt=1/5
wifi_prov: STA disconnected reason=202 source=boot attempt=1/5
```

**Must NOT appear** after v2 connect-cycle-only failure:

```text
wifi_prov: saved boot failed attempts=5
```

## Runtime link-loss entry (`handle_runtime_link_loss`)

Guards (all required):

- `!s_portal_active`
- `s_link_status == WIFI_LINK_STATUS_CONNECTED`
- `s_pending_sta_source != WIFI_PROV_STA_SOURCE_SAVED` (boot owns STA during `connect_saved`)

Actions:

- Clear `s_sta_ipv4[0]`.
- `start_runtime_reconnect_if_needed()` (creates task if `s_runtime_reconnect_task == NULL`).

Does **not** set `DISCONNECTED` or emit failure lock.

## Anti-patterns (do not reintroduce)

1. Emit `SAVED_FAILURE_LOCKED` or show OLED `HOLD RESET` after failed rounds.
2. Set `s_locked_disconnected` or return `true` from `wifi_provisioning_locked_disconnected()`.
3. Emit `SAVED_CONNECTING` / `SAVED_SUCCESS` on boot connect path.
4. Run `connect_cycle_run_forever` in a new task on **boot** (must block `connect_saved` caller).
5. Run boot connect cycle without `CONNECT_BOOT_SETTLE_MS` delay.
6. Use `source=saved` in connect-cycle logs (implementation uses `boot`).
7. Map `CONNECTING` LED to solid ON (v1 mapping).
8. Terminal `DISCONNECTED` fast blink while credentials remain in NVS.

## Validation checklist (tester / next implementer)

1. Empty NVS: LED solid ON; portal OLED.
2. Creds + AP up: slow blink → off; `CONNECT_RESTORED` or success screen.
3. Creds + AP down (boot): blocking cycle on startup task; slow → fast 15s → slow; OLED `WIFI`/`CONNECTING` only.
4. Creds + AP down (runtime): same cycle in `wifi_rt_rc` task after link loss.
5. AP returns: `connect cycle restored` in log; LED off.
6. Serial: `source=boot` or `source=runtime`; no `saved boot failed attempts=5`.
7. Never OLED four-line `HOLD RESET`.
