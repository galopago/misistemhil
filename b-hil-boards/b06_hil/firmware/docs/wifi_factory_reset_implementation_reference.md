# WiFi Factory Reset — Implementation Reference (As-Built)

Normative as-built record for reproducing the validated GPIO7 factory reset behavior.
Source handoff: `WIFI_FACTORY_RESET_RUNTIME`. Operator validation: Run 021 (2026-06-23).

Policy overview: [`wifi_factory_reset_architecture.md`](wifi_factory_reset_architecture.md)

Connect cycle interaction: [`wifi_connect_cycle_implementation_reference.md`](wifi_connect_cycle_implementation_reference.md)

## Authorized source files

| File | Role |
| --- | --- |
| [`components/app_core/app_core_wifi.c`](../components/app_core/app_core_wifi.c) | GPIO7 boot hold, monitor task, erase orchestration |
| [`components/wifi_provisioning/wifi_provisioning.c`](../components/wifi_provisioning/wifi_provisioning.c) | Abort flag, connect-cycle interrupt, portal transition API |
| [`components/wifi_provisioning/include/wifi_provisioning.h`](../components/wifi_provisioning/include/wifi_provisioning.h) | `wifi_provisioning_factory_reset_to_portal()` |
| [`components/wifi_credentials/wifi_credentials.c`](../components/wifi_credentials/wifi_credentials.c) | `wifi_credentials_erase()` only |

Do not add GPIO sampling or NVS erase inside `wifi_provisioning` except via the public
portal API called from `app_core_wifi`.

## Compile-time constants

### `app_core_wifi.c`

```c
#define FACTORY_RESET_HOLD_MS 10000U
#define FACTORY_RESET_SAMPLE_MS 50U
#define FACTORY_RESET_MONITOR_STACK 3072U
#define FACTORY_RESET_MONITOR_PRIORITY 6
```

### `wifi_provisioning.c`

```c
#define FACTORY_RESET_CONNECT_POLL_MS 100U
```

| Constant | Value | Purpose |
| --- | --- | --- |
| `FACTORY_RESET_HOLD_MS` | 10000 | Continuous press duration (boot + runtime) |
| `FACTORY_RESET_SAMPLE_MS` | 50 | GPIO poll interval |
| `FACTORY_RESET_MONITOR_STACK` | 3072 | `wifi_fr_mon` stack bytes |
| `FACTORY_RESET_MONITOR_PRIORITY` | 6 | Above `wifi_rt_rc` (5) |
| `FACTORY_RESET_CONNECT_POLL_MS` | 100 | Connect wait/backoff/alert interrupt slice |

Architecture doc `FACTORY_RESET_ABORT_MAX_MS` (500) is a **product budget**, not a
`#define` in code. With 100 ms polling + `esp_wifi_disconnect()`, abort typically
completes within that budget.

## Static state

| Symbol | Module | Purpose |
| --- | --- | --- |
| `s_factory_reset_abort` | `wifi_provisioning.c` | `volatile bool`; set during `factory_reset_to_portal()` |
| `s_wifi_started` | `app_core_wifi.c` | WiFi startup guard |

GPIO uses `board_pins()->factory_reset_switch` and `BOARD_FACTORY_RESET_ACTIVE_LEVEL` (0).

## Boot path — function table

| Function | Role |
| --- | --- |
| `factory_reset_gpio_configure()` | Input, pull-up, no interrupt |
| `factory_reset_button_pressed()` | `gpio_get_level == ACTIVE_LEVEL` |
| `factory_reset_hold_confirmed()` | Blocking loop up to `FACTORY_RESET_HOLD_MS`; false if released early |
| `factory_reset_requested_at_boot()` | Configure GPIO; if pressed, require hold; log and return true |
| `factory_reset_monitor_start()` | Create `wifi_fr_mon` after `wifi_provisioning_init()` |

Boot sequence in `app_core_wifi_start()`:

```text
wifi_credentials_init()
factory_reset_requested_at_boot()
  -> if true: wifi_credentials_erase(); credentials_erased = true
wifi_provisioning_init(...)
factory_reset_monitor_start()          /* monitor runs before connect path */
has_credentials = !credentials_erased && wifi_credentials_load(...)
portal or connect_saved(...)
```

**As-built:** Boot path does **not** call `factory_reset_wait_for_release()`. Monitor
task starts immediately after init; if the button remains held, runtime counter may
accumulate separately (idempotent re-erase is safe).

Boot log (no `(runtime)` suffix):

```text
W app_core_wifi: factory reset requested
```

## Runtime path — monitor task

| Property | Value |
| --- | --- |
| Task name | `wifi_fr_mon` |
| Entry | `factory_reset_monitor_task` |
| Stack / priority | 3072 / 6 |

### Monitor algorithm (as-built — non-blocking counter)

Unlike boot, the monitor does **not** call `factory_reset_hold_confirmed()` (that
function blocks). It uses an incremental counter:

```text
pressed_ms = 0
loop forever:
  if button pressed:
    delay FACTORY_RESET_SAMPLE_MS
    if still pressed:
      pressed_ms += FACTORY_RESET_SAMPLE_MS
      if pressed_ms >= FACTORY_RESET_HOLD_MS:
        factory_reset_runtime_sequence()
        factory_reset_wait_for_release()   /* blocks until released */
        pressed_ms = 0
    else:
      pressed_ms = 0
  else:
    pressed_ms = 0
    delay FACTORY_RESET_SAMPLE_MS
```

The **double sample** after delay avoids counting a glitch press.

### Runtime reset sequence (`factory_reset_runtime_sequence`)

```text
1. Log: factory reset requested (runtime)
2. wifi_credentials_erase() — on fail: log erase failed; return (no portal)
3. wifi_provisioning_factory_reset_to_portal() — on fail: log portal transition failed
4. Log: factory reset complete portal_active=true
```

## `wifi_provisioning_factory_reset_to_portal()` (exact order)

```c
esp_err_t wifi_provisioning_factory_reset_to_portal(void)
{
    ESP_LOGI(TAG, "factory reset abort connect cycle");
    s_factory_reset_abort = true;

    if (s_runtime_reconnect_task != NULL) {
        vTaskDelete(s_runtime_reconnect_task);
        s_runtime_reconnect_task = NULL;
    }

    s_pending_sta_source = WIFI_PROV_STA_SOURCE_NONE;
    s_connect_attempt = 0;
    s_sta_ipv4[0] = '\0';
    s_last_disconnect_reason = 0;

    ESP_LOGI(TAG, "factory reset stopping wifi");
    (void)esp_wifi_disconnect();

    if (s_httpd != NULL) { httpd_stop(s_httpd); s_httpd = NULL; }
    if (s_wifi_stack_ready) { (void)esp_wifi_stop(); }

    s_factory_reset_abort = false;   /* cleared BEFORE portal start */

    return wifi_provisioning_start_ap_portal();
}
```

Portal start reuses existing path: `set_link_status(UNPROVISIONED)`,
`WIFI_MODE_AP`, HTTP handlers, `AP_STARTED` + `PORTAL_READY` events.

## Connect cycle abort integration

### `factory_reset_should_abort()`

Returns true when:

1. `s_factory_reset_abort == true`, **or**
2. `!wifi_credentials_load(&creds)` (NVS erased while cycle running)

### Call sites

| Location | Mechanism |
| --- | --- |
| `connect_cycle_run_forever` loop head | `factory_reset_should_abort()` |
| `connect_cycle_run_round` attempt loop | `factory_reset_should_abort()` + `wifi_credentials_load` |
| `run_sta_connect_attempt` wait loop | Poll event group in `FACTORY_RESET_CONNECT_POLL_MS` slices; check `factory_reset_should_abort()` each slice |
| Intra-round backoff | `delay_interruptible_ms(backoff_ms)` |
| Alert phase 15 s | `delay_interruptible_ms(CONNECT_ALERT_PHASE_MS)` |
| `runtime_reconnect_task` loop head | `s_portal_active \|\| factory_reset_should_abort()` |

### `delay_interruptible_ms(ms)`

Slices delay into steps of at most `FACTORY_RESET_CONNECT_POLL_MS`; returns false
(stops round) if `factory_reset_should_abort()` or `s_portal_active`.

Boot blocked task (`connect_saved` → `connect_cycle_run_forever`): monitor runs in
**another** task; abort unblocks via flag + credential erase + `esp_wifi_disconnect()`.

## Events, LED, OLED

No new `wifi_prov_event_t`. After portal transition:

- `WIFI_PROV_EVENT_AP_STARTED` / `PORTAL_READY`
- `link_status = UNPROVISIONED` → LED solid ON; provisioning setup OLED + QR

## Required serial logs (regression order)

### Boot

```text
W app_core_wifi: factory reset requested
```

### Runtime (full sequence)

```text
W app_core_wifi: factory reset requested (runtime)
I wifi_prov: factory reset abort connect cycle
I wifi_prov: factory reset stopping wifi
I wifi_prov: starting SoftAP ssid=HIL-06-XXXX
I wifi_prov: provisioning portal ready at http://192.168.4.1
I app_core_wifi: factory reset complete portal_active=true
```

Run 021 operator capture included the first, third, and last `app_core_wifi` lines plus
`abort connect cycle`. **`stopping wifi` is in firmware**; include in regression checks.

### Failure paths

```text
E app_core_wifi: factory reset erase failed err=<name>
E app_core_wifi: factory reset portal transition failed err=<name>
E app_core_wifi: factory reset monitor gpio init failed
E app_core_wifi: factory reset monitor task create failed
```

**Must NOT** log `saved boot failed attempts=5` as part of factory reset.

## Validation matrix (Run 021)

| Criterion | Run 021 | Evidence |
| --- | --- | --- |
| Runtime reset from connect cycle | **PASS** | Serial + operator Entry 024 |
| Portal + QR after reset | **PASS** | Human Entry 024 |
| Reprovision after reset | **PASS** | Human Entry 024 (vitriolina) |
| Portal idempotent (2× reset) | **PASS** | Phase A serial |
| Boot hold ≥ 10 s | **Deferred** | Not exercised |
| Boot short press &lt; 10 s | **Deferred** | Not exercised |
| Reset from stable `WIFI OK` only | **Deferred** | Reset from cycle/portal only |
| Runtime reconnect task + reset | **Deferred** | Not isolated in Run 021 |

Operator-critical v1 path is **validated**. Deferred items are recommended follow-up,
not blockers for accepting `WIFI_FACTORY_RESET_RUNTIME`.

## Anti-patterns

1. Call `wifi_credentials_erase()` from `wifi_provisioning.c`.
2. Use `esp_restart()` as primary runtime reset path.
3. Tap-to-erase without 10 s hold.
4. Block boot path without shared `FACTORY_RESET_HOLD_MS` / `FACTORY_RESET_SAMPLE_MS`.
5. Use blocking `factory_reset_hold_confirmed()` inside `wifi_fr_mon` (deadlocks UX).
6. Leave `s_factory_reset_abort` true during normal portal operation.
7. Skip `factory_reset_wait_for_release()` after runtime reset (repeat fires while held).
8. Full 12 s blocking wait in `run_sta_connect_attempt` without 100 ms slices.

## Regression checklist (tester / next LLM)

1. Fake NVS creds → connect cycle → hold GPIO7 10 s → serial runtime logs + portal OLED.
2. POST new SSID after reset → `WIFI OK` + LED off.
3. Hold &lt; 10 s → no erase (when tested).
4. Portal mode → hold 10 s twice → idempotent portal refresh.
5. `strings` firmware for `wifi_fr_mon`, `factory reset requested (runtime)`.
6. Boot hold 10 s at cold boot (deferred but should pass when run).
