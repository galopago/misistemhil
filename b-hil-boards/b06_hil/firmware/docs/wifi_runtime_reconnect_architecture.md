# WiFi Runtime Reconnect Architecture

> **Superseded (v2):** Boot and runtime reconnect use motor
> `connect_cycle_run_forever` in
> [`wifi_connect_cycle_architecture.md`](wifi_connect_cycle_architecture.md).
> As-built: [`wifi_connect_cycle_implementation_reference.md`](wifi_connect_cycle_implementation_reference.md).
> Boot lock (`SAVED_FAILURE_LOCKED`) and split policies below are **obsolete**.

Normative contract for reconnecting STA after link loss at runtime when valid
credentials remain in NVS. Source handoff: `WIFI_RUNTIME_RECONNECT` (v1).

Related:

- `docs/wifi_provisioning_architecture.md` — event model, saved boot policy
- `docs/error_led_wifi_link_architecture.md` — `wifi_link_status_t` → LED mapping
- `docs/error_led_runtime_link_architecture.md` — LED layer on link loss (layer 1)

## Purpose

After `SUBMITTED_SUCCESS` or `SAVED_SUCCESS`, the device operates in STA-only mode
with credentials in NVS. If the home AP/router goes offline or the STA loses IP,
the product MUST attempt to reconnect **indefinitely** with bounded backoff until
`IP_EVENT_STA_GOT_IP` or factory reset.

Layer 1 (`ERROR_LED_RUNTIME_LINK_STATUS`) already updates `link_status` and the
error LED on loss. This contract adds layer 2: **application reconnect policy**,
explicit product events, and OLED feedback during recovery.

## Product decision

| Situation | Behavior |
| --- | --- |
| Saved credentials fail at **cold boot** | Up to 5 attempts → `SAVED_FAILURE_LOCKED`; fast LED; no portal reopen; factory reset to recover |
| Link lost at **runtime** with valid creds | Unbounded reconnect with capped backoff; **never** enter boot lock from runtime loss alone |
| Factory reset (GPIO7 hold 10 s) | Erase credentials; cancel reconnect task; restart portal |

Runtime reconnect MUST NOT erase credentials, MUST NOT reopen the provisioning AP,
and MUST NOT set `s_locked_disconnected` or `s_saved_policy_exhausted`.

## Layer model

```text
wifi_provisioning
  ├─ detect link loss (STA_DISCONNECTED / STA_LOST_IP)     [layer 1, implemented]
  ├─ runtime_reconnect_task (unbounded STA attempts)       [layer 2, this contract]
  └─ emit neutral events + link_status
        │
        v
app_core_wifi
  ├─ OLED templates per event
  └─ apply_wifi_link_status_led(info->link_status)
```

Module boundaries unchanged: `wifi_provisioning` MUST NOT include display or
`error_led` headers.

## Trigger conditions

Start or keep `runtime_reconnect_task` when **all** are true:

1. `s_link_status` was `WIFI_LINK_STATUS_CONNECTED` (or transitioning from it on
   first loss detection).
2. Valid credentials exist in NVS (`wifi_credentials_load` succeeds).
3. `s_portal_active == false`.
4. `s_locked_disconnected == false` (boot lock path; no runtime reconnect).
5. `s_pending_sta_source != WIFI_PROV_STA_SOURCE_SAVED` (saved boot owns STA).
6. No `runtime_reconnect_task` already running (dedupe `STA_DISCONNECTED` +
   `STA_LOST_IP`).

Do **not** start runtime reconnect during portal provisioning or boot lock.

## Reconnect policy (normative)

```text
Policy: unbounded attempts until GOT_IP or factory reset.

Per attempt:
  - Reuse saved-boot attempt semantics: clear bits, esp_wifi_disconnect(),
    esp_wifi_connect(), wait for GOT_IP or early STA_DISCONNECTED.
  - per_attempt_timeout_ms: 12000
  - apply_sta_config() before first attempt in a task episode (same WPA2/WPA3 profile)

Backoff after failed attempt (capped exponential, ms):
  [1000, 3000, 5000, 8000, 12000, 15000, 20000, 30000]
  After index 7, hold at 30000 ms forever.

link_status during reconnect episode:
  - CONNECTING for entire episode (active attempt + backoff between attempts).
  - CONNECTED on GOT_IP success.
  - MUST NOT transition to DISCONNECTED from runtime reconnect failure alone.

Credentials:
  - Load from NVS at task start; never erase on reconnect failure.
```

### Required serial logs

```text
wifi_prov: runtime reconnect policy per_attempt_timeout_ms=12000 backoff_cap_ms=30000
wifi_prov: runtime reconnect attempt <n> start
wifi_prov: STA disconnected reason=<reason> source=runtime attempt=<n>
wifi_prov: runtime reconnect attempt <n> failed reason=<reason> backoff_ms=<ms>
wifi_prov: STA got IP ip=<ipv4> source=runtime attempt=<n>
wifi_prov: runtime reconnect restored ip=<ipv4>
```

Boot logs (`source=saved`, `saved boot failed attempts=5`) MUST NOT appear for
runtime-only failures.

## Event model

Add to `wifi_provisioning.h` (after `WIFI_PROV_EVENT_LINK_STATUS_CHANGED`):

```c
WIFI_PROV_EVENT_RUNTIME_RECONNECTING,
WIFI_PROV_EVENT_RUNTIME_RESTORED,
```

### `WIFI_PROV_EVENT_RUNTIME_RECONNECTING`

- Emitted once when `runtime_reconnect_task` starts (first loss after CONNECTED).
- `link_status`: `WIFI_LINK_STATUS_CONNECTING`.
- `ssid`: home network SSID from loaded credentials (MAY be non-NULL).
- `setup_url`, `sta_ipv4`, `sta_mac`: NULL.
- `status`: `ESP_OK`.

### `WIFI_PROV_EVENT_RUNTIME_RESTORED`

- Emitted when runtime reconnect obtains IP.
- `link_status`: `WIFI_LINK_STATUS_CONNECTED`.
- `sta_ipv4`, `sta_mac`: same rules as `SAVED_SUCCESS` (non-NULL on success).
- `ssid`: home network SSID (MAY be non-NULL).
- `status`: `ESP_OK`.

Do **not** reuse `SAVED_CONNECTING` / `SAVED_SUCCESS` for runtime paths (tester
and serial logs must distinguish boot from runtime).

`WIFI_PROV_EVENT_LINK_STATUS_CHANGED` remains valid for LED-only transitions;
with `RUNTIME_*` events, `link_status` on every `emit_event` is sufficient for
LED updates.

## Internal STA source

Add internal enum value (not necessarily public API):

```c
WIFI_PROV_STA_SOURCE_RUNTIME,
```

Use in `sta_source_label()` for logs (`source=runtime`). `wait_for_sta_attempt`
MUST accept `RUNTIME` with the same wait semantics as `SAVED`.

## Refactor contract (`wifi_provisioning.c`)

Extract shared helper from `connect_saved()` and `wait_for_saved_sta_attempt()`:

```c
static esp_err_t run_sta_connect_attempt(uint8_t attempt, uint32_t timeout_ms,
                                         wifi_prov_sta_source_t source);
```

- `connect_saved()`: loop 5 attempts with `source=SAVED`; on exhaustion set
  `s_locked_disconnected`, emit `SAVED_FAILURE_LOCKED` (unchanged).
- `runtime_reconnect_task(void *arg)`: infinite loop with `source=RUNTIME` and
  runtime backoff table; on success call `emit_sta_success_event` equivalent for
  `RUNTIME_RESTORED`.

### Task lifecycle

```c
static TaskHandle_t s_runtime_reconnect_task = NULL;

static void start_runtime_reconnect_if_needed(void)
{
    if (s_runtime_reconnect_task != NULL || s_locked_disconnected || s_portal_active) {
        return;
    }
    wifi_credentials_t creds;
    if (!wifi_credentials_load(&creds)) {
        return;
    }
    set_link_status(WIFI_LINK_STATUS_CONNECTING);
    emit_event(WIFI_PROV_EVENT_RUNTIME_RECONNECTING, NULL, creds.ssid, NULL, NULL, ESP_OK);
    xTaskCreate(runtime_reconnect_task, "wifi_rt_rc", 4096, NULL, 5, &s_runtime_reconnect_task);
}
```

Call `start_runtime_reconnect_if_needed()` from `handle_runtime_link_loss()` instead
of immediately setting `DISCONNECTED` when reconnect is authorized.

On task exit (success or factory reset): clear `s_runtime_reconnect_task` handle.

### Interaction with layer 1

After this handoff is implemented, `runtime_link_status_after_loss()` MUST NOT
force `DISCONNECTED` when runtime reconnect will start; the task sets `CONNECTING`
before the first `esp_wifi_connect()`.

## Display integration (`app_core_wifi.c`)

| Event | OLED |
| --- | --- |
| `RUNTIME_RECONNECTING` | `WIFI` / `CONNECTING` (two lines, same as saved boot) |
| `RUNTIME_RESTORED` | `WIFI OK` + IP + split MAC via `show_wifi_connected_display()` |
| Backoff between attempts | Keep `CONNECTING` screen (no flicker) |

`apply_wifi_link_status_led(info->link_status)` at end of `on_wifi_prov_event`
(unchanged mapping table).

## Error LED during runtime reconnect

| Phase | `link_status` | LED |
| --- | --- | --- |
| Connected | `CONNECTED` | OFF |
| Reconnect episode (attempt + backoff) | `CONNECTING` | Solid ON |
| Boot lock only | `DISCONNECTED` | Fast blink |

Runtime loss alone MUST NOT produce locked fast blink. See
`docs/error_led_runtime_link_architecture.md`.

## Edge cases

| Case | Rule |
| --- | --- |
| Duplicate loss events | Ignore if reconnect task already running |
| Loss during `connect_saved()` | Boot path owns STA; no runtime task |
| `s_saved_policy_exhausted` | Stale GOT_IP rule unchanged; runtime never sets this flag |
| Factory reset during reconnect | Cancel task; erase creds; portal path |
| Portal active | No runtime reconnect |
| AP returns after outage | Task reconnects; emit `RUNTIME_RESTORED` |

Open question (v1): 500 ms debounce before starting task if only `LOST_IP` arrives
without `STA_DISCONNECTED` — add only if tester reports false starts.

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `RUNTIME_RECONNECTING`, `RUNTIME_RESTORED` |
| `components/wifi_provisioning/wifi_provisioning.c` | `run_sta_connect_attempt`, `runtime_reconnect_task`, trigger, dedupe |
| `components/app_core/app_core_wifi.c` | OLED for runtime events |

**Not authorized:** change boot 5-attempt lock, open portal on runtime loss, erase
credentials on runtime failure, `error_led` timing changes.

## Acceptance criteria

1. Connected → AP off 30 s → LED solid ON; OLED `WIFI` / `CONNECTING`.
2. AP on → recovers IP within reasonable time (&lt; 2 min on bench); LED off; OLED
   `WIFI OK` with current IP/MAC.
3. AP off indefinitely → LED stays ON (reconnecting); never `HOLD RESET` / boot lock.
4. Cold boot with bad saved creds → still `SAVED_FAILURE_LOCKED` after 5 attempts.
5. Serial shows `source=runtime attempt=N`; no `saved boot failed attempts=5` from
   runtime-only outage.
6. `idf.py build`; module boundary rules pass.

## Validation plan

- Implementer: build; log each runtime attempt and backoff.
- Tester: AP power-cycle while connected; long AP outage; boot-lock regression with
  wrong creds; photograph OLED and LED per phase.
