# Error LED Connect Cycle Architecture (v2)

Normative contract for GPIO8 error LED patterns under the v2 WiFi connect cycle.
Source handoff: `WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2`.

**Supersedes** `docs/error_led_wifi_link_architecture.md` (v1 mapping).

Related:

- **`docs/wifi_connect_cycle_implementation_reference.md`** — as-built LED adapter
  (`apply_wifi_link_status_led`, `default → ON`, end-of-callback rule)
- `docs/wifi_connect_cycle_architecture.md` — connect round/alert policy
- `docs/wifi_provisioning_architecture.md` — `wifi_link_status_t` and events
- `docs/esp32_c3_supermini_connections.md` — GPIO8 active-low

## Purpose

Give operators a consistent LED signal:

| Product situation | LED pattern |
| --- | --- |
| No WiFi credentials in NVS | **Solid ON** |
| Credentials in NVS, connect round active | **Slow blink** |
| Credentials in NVS, alert phase after 5 failed attempts in round | **Fast blink** (15 s) |
| STA connected with IP | **OFF** |

There is **no** terminal fast-blink lock. After 15 s fast blink, the device
returns to slow blink and starts a new connect round.

Portal `SUBMITTED_CONNECTING` (credentials not yet saved): `UNPROVISIONED` → **solid
ON** (same as no credentials).

## Hardware

From `components/board/include/board_pins.h`:

- Pin: `BOARD_ERROR_LED_GPIO` (`GPIO8`)
- Active level: `BOARD_ERROR_LED_ACTIVE_LEVEL` (`0` = LED ON)

## Domain model: `wifi_link_status_t` (v2)

```c
typedef enum {
    WIFI_LINK_STATUS_UNPROVISIONED = 0,
    WIFI_LINK_STATUS_CONNECTING,
    WIFI_LINK_STATUS_CONNECTING_ALERT,
    WIFI_LINK_STATUS_CONNECTED,
} wifi_link_status_t;
```

`WIFI_LINK_STATUS_DISCONNECTED` is **deprecated** for credential connect paths in
v2. Do not drive the LED from `DISCONNECTED` when NVS credentials exist.

Every `wifi_prov_event_info_t` MUST include `link_status` copied from
`s_link_status` at emit time.

### Transition table (normative)

| Condition | `s_link_status` | LED |
| --- | --- | --- |
| No credentials / portal active | `UNPROVISIONED` | ON |
| `SUBMITTED_CONNECTING` (not saved yet) | `UNPROVISIONED` | ON |
| Connect round start (`CONNECT_CYCLE_ACTIVE`) | `CONNECTING` | SLOW |
| Between attempts within round | `CONNECTING` | SLOW |
| Round exhausted (`CONNECT_ALERT_PHASE`) | `CONNECTING_ALERT` | FAST |
| After 15 s alert, new round | `CONNECTING` | SLOW |
| `GOT_IP` / `CONNECT_RESTORED` | `CONNECTED` | OFF |
| Factory reset before portal | `UNPROVISIONED` | ON |
| Runtime link loss with creds | `CONNECTING` (new round) | SLOW |

MUST NOT transition to a permanent `DISCONNECTED` LED state while credentials
remain in NVS.

## As-built adapter (`app_core_wifi.c`)

LED updates run at the **end of every** `on_wifi_prov_event()` invocation — not only
on `CONNECT_CYCLE_*` events. Portal, AP, and success events also refresh the LED via
`info->link_status`.

Primary cycle driver: `CONNECT_CYCLE_ACTIVE` / `CONNECT_ALERT_PHASE` /
`CONNECT_RESTORED` with `link_status`. `WIFI_PROV_EVENT_LINK_STATUS_CHANGED` is
**secondary** (e.g. stale GOT_IP when no STA attempt is active); do not rely on it as
the main connect-cycle LED path.

### `apply_wifi_link_status_led` (validated switch)

```c
static void apply_wifi_link_status_led(wifi_link_status_t status)
{
    error_led_pattern_t pattern;
    switch (status) {
    case WIFI_LINK_STATUS_UNPROVISIONED:
        pattern = ERROR_LED_PATTERN_ON;
        break;
    case WIFI_LINK_STATUS_CONNECTING:
        pattern = ERROR_LED_PATTERN_SLOW_BLINK;
        break;
    case WIFI_LINK_STATUS_CONNECTING_ALERT:
        pattern = ERROR_LED_PATTERN_FAST_BLINK;
        break;
    case WIFI_LINK_STATUS_CONNECTED:
        pattern = ERROR_LED_PATTERN_OFF;
        break;
    default:
        pattern = ERROR_LED_PATTERN_ON;  /* covers deprecated DISCONNECTED if ever set */
        break;
    }
    (void)app_core_error_led_set_pattern(pattern);
}
```

See implementation reference for full `on_wifi_prov_event` grouping and
`SAVED_FAILURE_LOCKED` empty branch.

### Boot gap (before first callback)

```c
if (!has_credentials) {
    apply_wifi_link_status_led(WIFI_LINK_STATUS_UNPROVISIONED);  /* ON */
} else {
    apply_wifi_link_status_led(WIFI_LINK_STATUS_CONNECTING);     /* slow */
}
```

## Timing (`error_led.c`)

Unchanged from v1:

| Pattern | Timing |
| --- | --- |
| `OFF` | LED inactive |
| `SLOW_BLINK` | 500 ms ON / 500 ms OFF |
| `ON` | Held active |
| `FAST_BLINK` | 125 ms ON / 125 ms OFF |

Alert phase duration (15 s) is enforced by `wifi_provisioning` state machine, not
by `error_led` timers.

## Module boundaries

```text
wifi_provisioning  --events + link_status-->  app_core_wifi
app_core_wifi      --pattern only----------->  error_led
```

- `wifi_provisioning` MUST NOT call `error_led_set_pattern`.
- `app_core_wifi` MUST NOT infer LED from `wifi_prov_event_t` alone; use
  `info->link_status`.

## v1 → v2 mapping change summary

| `wifi_link_status_t` | v1 LED | v2 LED |
| --- | --- | --- |
| `UNPROVISIONED` | Slow | **ON** |
| `CONNECTING` | ON | **Slow** |
| `DISCONNECTED` (locked) | Fast | **Removed** (use `CONNECTING_ALERT` temporarily) |
| `CONNECTED` | OFF | OFF |
| (new) `CONNECTING_ALERT` | — | Fast |

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `CONNECTING_ALERT` enum value |
| `components/wifi_provisioning/wifi_provisioning.c` | Set `link_status` per cycle |
| `components/app_core/app_core_wifi.c` | v2 `apply_wifi_link_status_led` table |
| `components/error_led/error_led.c` | No timing change expected |

## Acceptance criteria

1. Fresh NVS / portal: **solid ON** (not slow).
2. Saved credentials, connecting: **slow blink** (not solid ON).
3. Five failed attempts in round: **fast blink** for ~15 s.
4. After alert: **slow blink** again; no permanent fast lock.
5. Connected: LED **off**.
6. Never fast blink forever with creds present unless stuck in repeated alert
   phases (expected during prolonged outage).
7. No `SAVED_FAILURE_LOCKED` OLED; LED must not imply factory reset required.

## Validation plan

- Tester: NVS erase (solid ON) → provision (off) → AP down (slow → fast 15 s →
  slow cycle) → AP up (off).
- Serial: correlate `link_status` on cycle events with observed LED pattern.
