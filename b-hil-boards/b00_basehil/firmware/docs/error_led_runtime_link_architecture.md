# Error LED Runtime Link Loss Architecture

> **Superseded (v2):** Runtime LED behavior is defined by the unified connect cycle in
> [`error_led_connect_cycle_architecture.md`](error_led_connect_cycle_architecture.md).
> `LINK_STATUS_CHANGED` is no longer the primary cycle LED path; use
> `CONNECT_CYCLE_*` events with `link_status`. Layer-1 runtime `DISCONNECTED` fast
> blink is replaced by `CONNECTING` (slow) and `CONNECTING_ALERT` (fast 15 s).

Normative LED-only contract for reflecting WiFi link changes **after** the device
has reached `WIFI_LINK_STATUS_CONNECTED`. Source handoff:
`ERROR_LED_RUNTIME_LINK_STATUS` (v1 layer 1).

Related:

- `docs/error_led_wifi_link_architecture.md` — base LED mapping and boot transitions
- `docs/wifi_provisioning_architecture.md` — event model and `wifi_link_status_t`
- `docs/wifi_runtime_reconnect_architecture.md` — runtime reconnect (layer 2); `CONNECTING` during reconnect episode

## Purpose

After `SUBMITTED_SUCCESS` or `SAVED_SUCCESS`, the error LED is OFF while
`s_link_status == CONNECTED`. If the STA link is lost at runtime, the LED MUST
update to follow the same `wifi_link_status_t` → pattern rules used at power-on,
without changing OLED content.

## Problem (current gap)

`wifi_provisioning.c` sets `s_link_status = CONNECTED` on success and does not
update it when `WIFI_EVENT_STA_DISCONNECTED` or `IP_EVENT_STA_LOST_IP` occur in
STA-only post-connect mode. `app_core_wifi` updates the LED only inside
`on_wifi_prov_event()`; without a new callback, the LED stays OFF.

## Scope

**In scope:**

- Keep `s_link_status` truthful after initial connect.
- Notify `app_core_wifi` so `apply_wifi_link_status_led()` runs.
- New neutral event `WIFI_PROV_EVENT_LINK_STATUS_CHANGED` (LED-only path).

**Out of scope (this document):**

- WiFi reconnect policy implementation (see `docs/wifi_runtime_reconnect_architecture.md`).
- Changes to `error_led` timing or the `wifi_link_status_t` → pattern table.

**Superseded by `WIFI_RUNTIME_RECONNECT` for reconnect episodes:**

- OLED updates during runtime reconnect (`RUNTIME_RECONNECTING` / `RUNTIME_RESTORED`).
- Holding `CONNECTING` (LED solid ON) for the full reconnect episode including backoff.

## Design principle

The LED does **not** infer WiFi state. It only consumes `info->link_status` from
the provisioning callback. `wifi_provisioning` owns `s_link_status` and MUST
align it with the STA link state it already models internally (same enum semantics
as saved-credentials boot).

| `wifi_link_status_t` | LED pattern (unchanged) |
| --- | --- |
| `UNPROVISIONED` | Slow blink |
| `CONNECTING` | Solid ON |
| `CONNECTED` | OFF |
| `DISCONNECTED` | Fast blink |

## Module boundaries

Unchanged from `docs/error_led_wifi_link_architecture.md`:

```text
wifi_provisioning  --LINK_STATUS_CHANGED + link_status-->  app_core_wifi
app_core_wifi      --pattern only----------------------->  error_led
```

- `wifi_provisioning` MUST NOT include `error_led.h` or `app_core.h`.
- `app_core_wifi` MUST NOT derive LED state from `wifi_prov_event_t` alone.

## Options considered (LED impact only)

| Option | Mechanism | LED | OLED | Verdict |
| --- | --- | --- | --- | --- |
| Reemit `SAVED_CONNECTING` / `SAVED_SUCCESS` | Reuse boot events | OK | Changes screen | Rejected |
| `WIFI_PROV_EVENT_LINK_STATUS_CHANGED` | Neutral event; adapter applies LED only | OK | Unchanged | **Chosen** |
| Polling in `app_core` | Periodic IP check | OK | Unchanged | Rejected (module boundary) |
| Direct GPIO from `wifi_provisioning` | Include `error_led.h` | OK | Unchanged | Rejected (coupling) |

## Event contract

Add to `wifi_provisioning.h`:

```c
WIFI_PROV_EVENT_LINK_STATUS_CHANGED,
```

Semantics:

- **LED-only notification.** `link_status` is authoritative.
- `setup_url`, `ssid`, `sta_ipv4`, `sta_mac` MUST be NULL.
- `status` MUST be `ESP_OK`.
- `app_core_wifi` MUST NOT change OLED on this event.

## Internal API (`wifi_provisioning.c`)

```c
static void publish_link_status(void)
{
    emit_event(WIFI_PROV_EVENT_LINK_STATUS_CHANGED, NULL, NULL, NULL, NULL, ESP_OK);
}
```

Call `publish_link_status()` only when `set_link_status()` changes the enum value
(idempotent if unchanged).

Helper to classify post-loss status without defining reconnect policy:

```c
static wifi_link_status_t runtime_link_status_after_loss(void)
{
    if (s_locked_disconnected) {
        return WIFI_LINK_STATUS_DISCONNECTED;
    }
    if (s_pending_sta_source != WIFI_PROV_STA_SOURCE_NONE) {
        return WIFI_LINK_STATUS_CONNECTING;
    }
    return WIFI_LINK_STATUS_DISCONNECTED;
}
```

`CONNECTING` here reuses the **existing** condition “STA connect attempt tracked by
`wifi_provisioning`” (same as saved-boot). This document does not specify when or
how WiFi reconnects.

## Runtime transition table (normative for LED)

Post-`CONNECTED`, credentials present in NVS, portal not active:

| Condition | `s_link_status` | LED |
| --- | --- | --- |
| STA has assigned IP | `CONNECTED` | OFF |
| Runtime reconnect episode active (see `wifi_runtime_reconnect_architecture.md`) | `CONNECTING` | ON |
| No IP and active STA attempt in `wifi_provisioning` (boot or runtime) | `CONNECTING` | ON |
| Boot lock (`s_locked_disconnected`) | `DISCONNECTED` | FAST |
| Layer 1 only (before `WIFI_RUNTIME_RECONNECT` implemented): no reconnect task | `DISCONNECTED` | FAST |

After `WIFI_RUNTIME_RECONNECT` is implemented, runtime link loss MUST transition to
`CONNECTING` (not `DISCONNECTED`) when `runtime_reconnect_task` starts. Runtime
loss alone MUST NOT enter boot lock or fast blink.

## WiFi event hooks (implementer)

In STA-only post-connect mode (`s_pending_sta_source == NONE`), update
`s_link_status` and call `publish_link_status()` on:

| Event | Action |
| --- | --- |
| `WIFI_EVENT_STA_DISCONNECTED` | If `s_link_status == CONNECTED`: clear cached IP, set status per `runtime_link_status_after_loss()`, publish |
| `IP_EVENT_STA_LOST_IP` | Same guard and transition as disconnect when `s_link_status == CONNECTED` |
| `IP_EVENT_STA_GOT_IP` | If not stale (`!s_saved_policy_exhausted`) and status is not already `CONNECTED`: cache IP, set `CONNECTED`, publish |

Guards:

- Do not publish runtime transitions while `s_portal_active` (portal uses
  `UNPROVISIONED`).
- Deduplicate: `publish_link_status()` only on real enum changes (both
  `STA_DISCONNECTED` and `STA_LOST_IP` may fire for one loss).
- Ignore stale `GOT_IP` when `s_saved_policy_exhausted` (existing rule).

## Adapter (`app_core_wifi.c`)

```c
case WIFI_PROV_EVENT_LINK_STATUS_CHANGED:
    break;  /* LED only; no OLED */
/* end of on_wifi_prov_event, unchanged: */
apply_wifi_link_status_led(info->link_status);
```

No changes to `apply_wifi_link_status_led()` or `error_led.c`.

## Authorized files (implementer)

| File | Change |
| --- | --- |
| `components/wifi_provisioning/include/wifi_provisioning.h` | `WIFI_PROV_EVENT_LINK_STATUS_CHANGED` |
| `components/wifi_provisioning/wifi_provisioning.c` | `publish_link_status`, runtime hooks, `runtime_link_status_after_loss` |
| `components/app_core/app_core_wifi.c` | empty `LINK_STATUS_CHANGED` case |

**Not authorized:** `error_led/*`, OLED paths, WiFi retry policy changes.

## Acceptance criteria

1. Connected device (LED OFF) → AP/router offline → LED is no longer OFF; shows ON
   or FAST per current `link_status` (not stuck OFF).
2. Restore AP → LED OFF again (`CONNECTED`).
3. Fresh NVS / portal: slow blink unchanged.
4. `SAVED_FAILURE_LOCKED` at boot: fast blink unchanged.
5. `LINK_STATUS_CHANGED` does not alter OLED.
6. `idf.py build`; no forbidden includes.

## Validation plan

- Implementer: serial log on each `LINK_STATUS_CHANGED` with `link_status` value.
- Tester: connect → power off AP → observe LED → power on AP → LED off; confirm
  OLED still shows last success screen during transient loss (expected v1).
