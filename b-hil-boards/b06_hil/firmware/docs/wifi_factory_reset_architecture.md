# WiFi Factory Reset Architecture

Normative contract for erasing WiFi credentials (`wifi_prov` NVS namespace) via the
`GPIO7` factory reset switch and returning the device to the provisioning portal.

Source handoff: `WIFI_FACTORY_RESET_RUNTIME`. Operator validation: Run 021 (2026-06-23).

Related:

- **`docs/wifi_factory_reset_implementation_reference.md`** — **as-built source of truth**
  (function names, monitor algorithm, abort integration, logs, Run 021 matrix)
- `docs/wifi_provisioning_architecture.md` — NVS contract, portal startup, events
- `docs/wifi_connect_cycle_architecture.md` — connect cycle v2 (must abort on reset)
- `docs/wifi_connect_cycle_implementation_reference.md` — as-built cycle engine
- `docs/error_led_connect_cycle_architecture.md` — LED after erase (`UNPROVISIONED` → ON)
- `docs/esp32_c3_supermini_connections.md` — `GPIO7` active-low wiring

## Purpose

Give the operator a **single hardware recovery path** to clear saved WiFi credentials
and reopen the provisioning portal **without** reflashing firmware or erasing NVS from
the host.

The switch is **not** a momentary “tap to erase”. A valid reset requires **holding**
the button active-low continuously for **10000 ms (10 s)**. A short press or release before
10000 ms MUST NOT erase credentials.

This contract covers:

1. **Boot factory reset** — blocking check in `app_core_wifi_start()` before WiFi init.
2. **Runtime factory reset** — monitor task `wifi_fr_mon` after `wifi_provisioning_init()`.

## Product rules

| Situation | Button action | Result |
| --- | --- | --- |
| No credentials in NVS | Hold `GPIO7` ≥ 10 s | Erase (no-op on NVS); portal remains or restarts; LED solid ON |
| STA connected | Hold ≥ 10 s | Erase NVS; stop STA; open provisioning AP + HTTP; OLED setup UX |
| Connect cycle active (boot or runtime) | Hold ≥ 10 s | Abort cycle within bounded time; erase; portal |
| Runtime reconnect task active | Hold ≥ 10 s | Cancel task; erase; portal |
| Portal already active | Hold ≥ 10 s | Erase (idempotent); refresh portal UX |
| Press &lt; 10 s or release early | Any time | No erase; no mode change |

Cross-cutting rules:

- Factory reset is the **only** product path to erase credentials (no web reset, no
  automatic erase on connect failure).
- Erase MUST call `wifi_credentials_erase()` (full `wifi_prov` namespace + commit).
- After erase, `wifi_credentials_load()` MUST return false.
- MUST NOT `esp_restart()` on the success path unless soft transition to portal fails.
- MUST NOT leave STA connect cycle running after credentials are erased.

## Hardware input

From `docs/esp32_c3_supermini_connections.md`:

| Constant | Value |
| --- | --- |
| `BOARD_FACTORY_RESET_GPIO` | `GPIO7` |
| `BOARD_FACTORY_RESET_ACTIVE_LEVEL` | `0` (pressed = low) |
| Pull-up | Enabled on input |

Pressed: `gpio_get_level(GPIO7) == BOARD_FACTORY_RESET_ACTIVE_LEVEL`.

## Timing constants (normative)

```text
FACTORY_RESET_HOLD_MS:        10000
FACTORY_RESET_SAMPLE_MS:      50
FACTORY_RESET_CONNECT_POLL_MS: 100   /* wifi_provisioning.c — connect abort slices */
FACTORY_RESET_ABORT_MAX_MS:   500   /* product budget; not a compile-time define */
```

`FACTORY_RESET_HOLD_MS` is **10000** for boot and runtime. Boot and runtime share the
same `#define` values in `app_core_wifi.c`.

## NVS erase contract

Use existing API only:

```c
esp_err_t wifi_credentials_erase(void);
```

Rules:

- Erase the full `wifi_prov` namespace and commit.
- On `ESP_ERR_NVS_NOT_FOUND`, treat as success (already empty).
- If erase fails, MUST NOT start portal; log error; remain in prior mode.
- Successful erase MUST be logged before any WiFi mode transition.

## Boot factory reset (normative)

**Entry:** `factory_reset_requested_at_boot()` in `app_core_wifi_start()` **before**
`wifi_provisioning_init()`.

**Helpers (shared):** `factory_reset_gpio_configure()`, `factory_reset_button_pressed()`,
`factory_reset_hold_confirmed()` (blocking hold loop).

**Algorithm:**

```text
1. factory_reset_gpio_configure()
2. If not pressed → return false
3. factory_reset_hold_confirmed() — false if released before 10000 ms
4. Log: factory reset requested
5. wifi_credentials_erase() in app_core_wifi_start (when boot returns true)
6. Continue boot with has_credentials = false → wifi_provisioning_start_ap_portal()
```

**Threading:** Blocks `app_core_wifi_start()` up to 10000 ms when button held at boot.

**As-built:** Boot does **not** wait for button release after erase. Monitor task
`wifi_fr_mon` starts immediately after `wifi_provisioning_init()`.

## Runtime factory reset (normative)

### Monitor task

Started by `factory_reset_monitor_start()` after `wifi_provisioning_init()` succeeds.

| Property | Value |
| --- | --- |
| Task name | `wifi_fr_mon` |
| Stack | 3072 bytes |
| Priority | 6 |
| Entry | `factory_reset_monitor_task` |

**As-built algorithm:** Incremental `pressed_ms` counter with 50 ms polling and
**double sample** after each delay (does not call blocking `factory_reset_hold_confirmed()`).
On `pressed_ms >= 10000`: `factory_reset_runtime_sequence()` then
`factory_reset_wait_for_release()` before re-arming.

See implementation reference for exact loop pseudocode.

### Runtime reset sequence

Executed only from `factory_reset_monitor_task` (not from WiFi event handlers):

```text
1. Log: app_core_wifi: factory reset requested (runtime)
2. err = wifi_credentials_erase()
   - on failure: log and return (no portal transition)
3. err = wifi_provisioning_factory_reset_to_portal()
   - on failure: log; optional esp_restart() only as last resort (see non-goals)
4. Log: app_core_wifi: factory reset complete portal_active=true
```

`app_core_wifi` orchestrates erase + provisioning transition. `wifi_credentials` owns
NVS; `wifi_provisioning` owns WiFi/HTTP mode changes.

### `wifi_provisioning_factory_reset_to_portal()`

Public API in `wifi_provisioning.h`. Called only from `app_core_wifi` after
`wifi_credentials_erase()`.

**As-built steps:**

1. Log `factory reset abort connect cycle`; set `s_factory_reset_abort = true`.
2. `vTaskDelete(s_runtime_reconnect_task)` if non-NULL; clear handle.
3. Clear STA attempt state (`s_pending_sta_source`, `s_connect_attempt`, `s_sta_ipv4`, …).
4. Log `factory reset stopping wifi`; `esp_wifi_disconnect()`.
5. Stop HTTP server and `esp_wifi_stop()` if active.
6. Clear `s_factory_reset_abort` **before** portal start.
7. `wifi_provisioning_start_ap_portal()` → `UNPROVISIONED` + portal events.

**Idempotency:** Safe when portal already active (Run 021 Phase A: 2× reset PASS).

### Connect cycle abort integration

`factory_reset_should_abort()` returns true when `s_factory_reset_abort` **or**
`!wifi_credentials_load()`.

Observed at:

- `connect_cycle_run_forever` / `connect_cycle_run_round` loop heads
- `run_sta_connect_attempt` — event wait in `FACTORY_RESET_CONNECT_POLL_MS` (100 ms) slices
- `delay_interruptible_ms()` — intra-round backoff and 15 s alert phase

When abort detected: exit cycle without `CONNECT_RESTORED`; portal owns WiFi mode.

### Interaction with connect cycle v2

| Prior state | After runtime reset |
| --- | --- |
| `CONNECTING` / `CONNECTING_ALERT` | `UNPROVISIONED`; portal; LED ON |
| `CONNECTED` | `UNPROVISIONED`; portal; LED ON |
| Portal active | `UNPROVISIONED`; portal refreshed |
| `wifi_rt_rc` running | Task ended; portal |

Connect cycle MUST NOT resume after erase until new credentials are saved via POST.

## Events, LED, and OLED

No new `wifi_prov_event_t` is required. Portal transition reuses:

- `WIFI_PROV_EVENT_AP_STARTED`
- `WIFI_PROV_EVENT_PORTAL_READY`

Both carry `link_status = WIFI_LINK_STATUS_UNPROVISIONED`.

`app_core_wifi` `on_wifi_prov_event()` already:

- Shows provisioning setup OLED + QR on portal events.
- Sets LED solid ON via `apply_wifi_link_status_led(UNPROVISIONED)`.

**Non-goal:** Special OLED text during the 10 s hold (e.g. “HOLD TO RESET”).

## Module boundaries

```text
board           — GPIO7 pin constants and active level only
wifi_credentials — NVS erase/load/save
wifi_provisioning — abort connect, stop WiFi/HTTP, start portal API
app_core_wifi   — boot hold check, monitor task, orchestration, LED/OLED adapter
```

- `wifi_provisioning` MUST NOT call `wifi_credentials_erase()`.
- `wifi_credentials` MUST NOT touch GPIO or WiFi mode.
- `app_core_wifi` MUST NOT reimplement portal HTTP or connect cycle logic.

## Required serial logs

Boot (unchanged):

```text
app_core_wifi: factory reset requested
```

Runtime (new):

```text
app_core_wifi: factory reset requested (runtime)
wifi_prov: factory reset abort connect cycle
wifi_prov: factory reset stopping wifi
wifi_prov: starting SoftAP ssid=HIL-06-XXXX
wifi_prov: provisioning portal ready at http://192.168.4.1
app_core_wifi: factory reset complete portal_active=true
```

On erase failure:

```text
app_core_wifi: factory reset erase failed err=<esp_err_name>
```

**Must NOT** log `saved boot failed attempts=5` as part of factory reset.

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/app_core/app_core_wifi.c` | Shared hold helper; start `wifi_fr_mon`; runtime sequence |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `wifi_provisioning_factory_reset_to_portal()` |
| `components/wifi_provisioning/wifi_provisioning.c` | Abort flag; cancel `wifi_rt_rc`; abort waits; portal API |
| `components/board/` | Use only; no change expected |

Do **not** add factory reset logic to `error_led`, `display`, or `wifi_credentials.c`
beyond existing `wifi_credentials_erase()`.

## Acceptance criteria

| # | Criterion | Run 021 |
| --- | --- | --- |
| 1 | Boot hold ≥ 10 s → portal | Deferred |
| 2 | Boot short press &lt; 10 s → no erase | Deferred |
| 3 | Runtime from `WIFI OK` stable | Deferred |
| 4 | Runtime connect cycle → portal | **PASS** |
| 5 | Runtime reconnect task + reset | Deferred |
| 6 | Portal idempotent 2× hold | **PASS** |
| 7 | NVS erase + reprovision POST | **PASS** (Entry 024) |
| 8 | Serial runtime reset logs | **PASS** |

Operator-critical path (rows 4, 6, 7, 8) validated. Rows 1–3, 5 recommended follow-up.

## Validation plan

- Implementer: `idf.py build`; log runtime reset from CONNECTED and from connect
  cycle (AP off).
- Tester: provision → connect → hold reset → confirm portal; provision new SSID;
  repeat from connect-cycle-outage scenario without power cycle.

## Non-goals

- Web or serial credential erase UI.
- Single tap (no hold) erase.
- Different hold duration for boot vs runtime.
- `esp_restart()` as the primary runtime reset mechanism.
- Encrypted NVS or credential backup.
- Factory reset of non-WiFi NVS namespaces.

## Open questions

- None. Hold duration, GPIO, and soft portal transition are fixed for v1.
