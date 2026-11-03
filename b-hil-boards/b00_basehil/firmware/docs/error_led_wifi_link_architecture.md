# Error LED WiFi Link Status Architecture

> **Superseded (v2):** Use [`error_led_connect_cycle_architecture.md`](error_led_connect_cycle_architecture.md)
> and [`wifi_connect_cycle_architecture.md`](wifi_connect_cycle_architecture.md).
> This document records the **v1** mapping (UNPROVISIONED=slow, CONNECTING=ON,
> DISCONNECTED=fast locked). Do not implement v1 for new work.

Normative contract for mapping WiFi provisioning link state to the board error LED
(`GPIO8`, active-low). Source handoff: `ERROR_LED_WIFI_LINK_STATUS` (v1).

Related:

- `docs/wifi_provisioning_architecture.md` — event model and `wifi_link_status_t`
- `docs/error_led_runtime_link_architecture.md` — runtime link loss LED updates (layer 1)
- `docs/wifi_runtime_reconnect_architecture.md` — runtime reconnect; `CONNECTING` LED during episode
- `docs/esp32_c3_supermini_connections.md` — GPIO8 polarity
- `docs/display_delivery_contract.md` — parallel decoupling pattern for OLED

## Purpose

Give operators a non-OLED signal of WiFi readiness:

| Product situation | LED pattern |
| --- | --- |
| No WiFi credentials in NVS / portal AP mode | Slow blink |
| Saved credentials, STA connecting (saved boot + retries) | Solid ON |
| Saved credentials, STA connected with IP | OFF |
| Saved credentials, cannot connect (retries exhausted) | Fast blink |

Portal `SUBMITTED_CONNECTING` (credentials not yet saved) remains **slow blink**
(`UNPROVISIONED`).

## Module boundaries

```text
wifi_provisioning  --neutral events + link_status-->  app_core_wifi
app_core_wifi      --pattern only----------------->  app_core_error_led facade
app_core           --delegates-------------------->  error_led component
error_led          --GPIO8 only-------------------->  hardware
```

Rules:

- `wifi_provisioning` MUST NOT include `error_led.h`, `app_core.h`, or GPIO driver
  headers for the error LED.
- `error_led` MUST NOT include WiFi, provisioning, or `app_core` headers.
- Only `app_core` (via `app_core_error_led_set_pattern`) may call
  `error_led_set_pattern` in v1.
- `app_core_wifi` maps `wifi_link_status_t` → `error_led_pattern_t`; it MUST NOT
  infer link status from individual `wifi_prov_event_t` values.

## Hardware

From `components/board/include/board_pins.h`:

- Pin: `BOARD_ERROR_LED_GPIO` (`GPIO8`)
- Active level: `BOARD_ERROR_LED_ACTIVE_LEVEL` (`0` = LED ON, `1` = LED OFF)

## Domain model: `wifi_link_status_t`

Declared in `components/wifi_provisioning/include/wifi_provisioning.h`:

```c
typedef enum {
    WIFI_LINK_STATUS_UNPROVISIONED = 0,
    WIFI_LINK_STATUS_CONNECTING,
    WIFI_LINK_STATUS_CONNECTED,
    WIFI_LINK_STATUS_DISCONNECTED,
} wifi_link_status_t;
```

Every `wifi_prov_event_info_t` MUST include:

```c
wifi_link_status_t link_status;
```

`wifi_provisioning.c` owns `static wifi_link_status_t s_link_status`. Update
`s_link_status` at every state transition; `emit_event` copies the current value
into `info.link_status`.

### Transition table (normative)

| Condition | `s_link_status` |
| --- | --- |
| `start_ap_portal()` success path (portal active, no saved creds) | `UNPROVISIONED` |
| `SUBMITTED_CONNECTING` | `UNPROVISIONED` |
| `SUBMITTED_FAILURE` (portal still active) | `UNPROVISIONED` |
| `connect_saved()` begins → `SAVED_CONNECTING` | `CONNECTING` |
| Saved boot retry loop (before success or lock) | `CONNECTING` |
| `SUBMITTED_SUCCESS` or `SAVED_SUCCESS` | `CONNECTED` |
| `SAVED_FAILURE_LOCKED` | `DISCONNECTED` |
| Factory reset erases credentials (before portal restart) | `UNPROVISIONED` |
| `ERROR` during portal startup (no creds path) | `UNPROVISIONED` |
| `ERROR` during saved connect before lock | `CONNECTING` until lock, then `DISCONNECTED` |
| Post-`CONNECTED` runtime: STA has IP (`IP_EVENT_STA_GOT_IP`, not stale) | `CONNECTED` |
| Post-`CONNECTED` runtime: link lost, active STA attempt in `wifi_provisioning` | `CONNECTING` |
| Post-`CONNECTED` runtime: reconnect episode active (`WIFI_RUNTIME_RECONNECT`) | `CONNECTING` |
| Post-`CONNECTED` runtime: link lost, no reconnect (layer 1 interim) | `DISCONNECTED` |
| Boot lock (`SAVED_FAILURE_LOCKED`) | `DISCONNECTED` |

`DISCONNECTED` from runtime link loss alone is **not** a v1 target once
`WIFI_RUNTIME_RECONNECT` is implemented; boot lock remains the only locked fast-blink
path for saved credentials.

After `SUBMITTED_SUCCESS`, status stays `CONNECTED` through AP teardown until a
runtime link-loss transition (see `docs/error_led_runtime_link_architecture.md`).

Runtime LED updates use `WIFI_PROV_EVENT_LINK_STATUS_CHANGED` (layer 1) and
`RUNTIME_RECONNECTING` / `RUNTIME_RESTORED` (layer 2). See
`docs/wifi_runtime_reconnect_architecture.md` for OLED on layer 2.

### Non-goals (v1)

- Error LED for I2C, INA219, or non-WiFi faults (future priority layer in
  `app_core`).
- Solid ON during portal `SUBMITTED_CONNECTING`.
- OLED refresh on runtime link loss in layer 1 only (`LINK_STATUS_CHANGED`); layer 2
  OLED is in `docs/wifi_runtime_reconnect_architecture.md`.

## Component: `error_led`

Path: `components/error_led/`

Public API (`include/error_led.h`):

```c
typedef enum {
    ERROR_LED_PATTERN_OFF = 0,
    ERROR_LED_PATTERN_SLOW_BLINK,
    ERROR_LED_PATTERN_ON,
    ERROR_LED_PATTERN_FAST_BLINK,
} error_led_pattern_t;

esp_err_t error_led_init(void);
esp_err_t error_led_set_pattern(error_led_pattern_t pattern);
```

### Timing (v1 constants in `error_led.c`)

| Pattern | Behavior |
| --- | --- |
| `OFF` | Inactive GPIO (LED off) |
| `SLOW_BLINK` | 500 ms ON / 500 ms OFF |
| `ON` | Active GPIO held (LED on) |
| `FAST_BLINK` | 125 ms ON / 125 ms OFF |

Implementation notes:

- Use `esp_timer` or equivalent; one periodic callback toggles GPIO for blink
  patterns.
- `set_pattern` is idempotent; changing pattern resets blink phase.
- `init` configures output and applies `OFF`.

## `app_core` facade

```c
esp_err_t app_core_error_led_set_pattern(error_led_pattern_t pattern);
```

- `error_led_init()` from `app_core_start()` **before** `app_core_wifi_start()`.
- Facade forwards to `error_led_set_pattern` only.

## Adapter: `app_core_wifi.c`

```c
static void apply_wifi_link_status_led(wifi_link_status_t status)
{
    error_led_pattern_t pattern;
    switch (status) {
    case WIFI_LINK_STATUS_UNPROVISIONED:
        pattern = ERROR_LED_PATTERN_SLOW_BLINK;
        break;
    case WIFI_LINK_STATUS_CONNECTING:
        pattern = ERROR_LED_PATTERN_ON;
        break;
    case WIFI_LINK_STATUS_CONNECTED:
        pattern = ERROR_LED_PATTERN_OFF;
        break;
    case WIFI_LINK_STATUS_DISCONNECTED:
        pattern = ERROR_LED_PATTERN_FAST_BLINK;
        break;
    default:
        pattern = ERROR_LED_PATTERN_SLOW_BLINK;
        break;
    }
    (void)app_core_error_led_set_pattern(pattern);
}
```

Call `apply_wifi_link_status_led(info->link_status)` at the end of every
`on_wifi_prov_event` branch.

Boot gap (before first callback):

```c
if (!has_credentials) {
    apply_wifi_link_status_led(WIFI_LINK_STATUS_UNPROVISIONED);
} else {
    apply_wifi_link_status_led(WIFI_LINK_STATUS_CONNECTING);
}
```

Run immediately after `wifi_credentials_load` and before
`wifi_provisioning_start_ap_portal` / `wifi_provisioning_connect_saved`.

## Mapping summary

| `wifi_link_status_t` | `error_led_pattern_t` | Operator view |
| --- | --- | --- |
| `UNPROVISIONED` | `SLOW_BLINK` | Needs WiFi setup |
| `CONNECTING` | `ON` | Connecting with saved creds |
| `CONNECTED` | `OFF` | Link OK |
| `DISCONNECTED` | `FAST_BLINK` | Creds present, no link |

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/error_led/*` | New component (complete header + `error_led.c` + CMake) |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `wifi_link_status_t`, `link_status` field |
| `components/wifi_provisioning/wifi_provisioning.c` | `s_link_status`, update on transitions |
| `components/app_core/app_core.c` | `error_led_init`, facade |
| `components/app_core/include/app_core.h` | facade declaration |
| `components/app_core/app_core_wifi.c` | `apply_wifi_link_status_led` |
| `components/app_core/CMakeLists.txt` | `REQUIRES error_led` |

## Acceptance criteria

1. Fresh NVS: slow blink in portal and during `SUBMITTED_CONNECTING`.
2. Saved-credentials boot: solid ON from early boot until connect or lock.
3. `SAVED_SUCCESS` / `SUBMITTED_SUCCESS`: LED off.
4. `SAVED_FAILURE_LOCKED`: fast blink.
5. No WiFi headers in `error_led`; no `error_led` headers in `wifi_provisioning`.
6. Blink timing changes only in `error_led.c`; semantics only in
   `wifi_provisioning` + adapter table.

## Validation plan

- Implementer: `idf.py build`; note pattern at each boot phase in serial log.
- Tester: erase NVS (slow blink) → provision (off on success) → reboot with
  creds (solid then off) → wrong AP / disconnect router (fast blink after lock).
