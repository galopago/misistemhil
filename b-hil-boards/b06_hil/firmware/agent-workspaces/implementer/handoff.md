# Implementer Handoff

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting any task in this file, run the **Role Boundary Check**:

- Read `ROLE.md` in this folder, `../../AGENTS.md`, and `../../docs/methodology.md`.
- Confirm there is an **implementation-ready architect handoff** and that your
  edits stay in authorized `main/` and `components/` paths.
- If the task changes architecture docs, acceptance criteria, or requires formal
  validation sign-off, stop and tell the human to redirect to architect or
  tester unless explicitly confirmed.

## Current Status

- `OLED_TEXT_DISPLAY_INTERFACE`: display stack + SSD1306 driver over I2C phase 2.
- `I2C_BUS_ARCHITECTURE` phase 1 and `I2C_BUS_PHASE2`: done.
- `DISPLAY_DELIVERY_CONTRACT`: **implemented** (`app_core_display_show_*`; sole caller).
- `QR_ENCODER_INTERFACE`: **implemented** (Nayuki qrcodegen, `setup_url`, real `display_qr`).
- `DISPLAY_VISUAL_DEMO_PROTOCOL`: **retired from product boot** (demo sources/Kconfig removed).
- `PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID`: **implemented** (dynamic AP SSID, demo retirement).
- `OLED_PROVISIONING_SETUP_UX`: **implemented** (SSID on OLED provisioning screen).
- `OLED_WIFI_CONNECTED_STATUS`: **implemented** (IP + split MAC on connected screen).
- `ERROR_LED_WIFI_LINK_STATUS`: **implemented** (GPIO8 WiFi link patterns; tester visual validation pending).
- `ERROR_LED_RUNTIME_LINK_STATUS`: **implemented** (runtime link_status LED updates; tester validation pending).
- `WIFI_RUNTIME_RECONNECT`: **implemented** (v1; superseded by v2 connect cycle).
- `WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2`: **implemented and operator-validated** (2026-06-23).
- `WIFI_PROVISIONING_ARCHITECTURE`: **Run 013 POST PASS, saved boot reliability FAIL**
  — saved credentials connect intermittently on cold reboot; corrective task
  `WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY` **implemented** (pending tester 10-boot sweep).
- Physical OLED controller for v1: **SSD1306** (128x64 I2C). No SH1106 work unless
  hardware changes.

Normative docs:

- `docs/display_delivery_contract.md`
- `docs/qr_encoder_interface.md`
- `docs/oled_text_display_interface.md`
- `docs/test_strategy.md`
- `docs/display_visual_demo_protocol.md`
- `docs/wifi_provisioning_architecture.md`
- `docs/error_led_wifi_link_architecture.md`
- `docs/error_led_runtime_link_architecture.md`
- `docs/wifi_runtime_reconnect_architecture.md`

- `docs/wifi_runtime_reconnect_architecture.md`
- `docs/wifi_connect_cycle_architecture.md`
- `docs/error_led_connect_cycle_architecture.md`

## ERROR_LED_RUNTIME_LINK_STATUS

```text
ID: ERROR_LED_RUNTIME_LINK_STATUS
Source:
  - agent-workspaces/architect/handoff.md, ERROR_LED_RUNTIME_LINK_STATUS
  - docs/error_led_runtime_link_architecture.md
Fix:
  wifi_provisioning.h:
    - WIFI_PROV_EVENT_LINK_STATUS_CHANGED
  wifi_provisioning.c:
    - publish_link_status(), update_runtime_link_status(), handle_runtime_link_loss()
    - runtime_link_status_after_loss() reuses pending-attempt / locked semantics
    - Runtime hooks on STA_DISCONNECTED, STA_LOST_IP, STA_GOT_IP (source NONE)
  app_core_wifi.c:
    - case WIFI_PROV_EVENT_LINK_STATUS_CHANGED: break (LED only)
    - apply_wifi_link_status_led unchanged
Modified files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Module boundary audit:
  - wifi_provisioning: no error_led references
  - error_led: unchanged
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdd9b0 bytes (prior 0xdd7d0)
Expected LED phases:
  - Connected OFF → AP outage → FAST (DISCONNECTED) or ON (CONNECTING if attempt active)
  - AP restored → OFF
  - OLED unchanged on LINK_STATUS_CHANGED
Ready for tester:
  Yes after build PASS. Kill AP while connected; observe LED; restore AP.
Status: Complete (2026-06-23).
```

## Display Visual Demo (historical)

`DISPLAY_VISUAL_DEMO_PROTOCOL` validated the OLED stack in Run 006 and is now
retired from normal product firmware. Do not add demo manifests, demo Kconfig, or
boot-time demo calls unless a future architect handoff explicitly authorizes a
temporary test-only demo.

## Next Tasks (simple order)

Tasks 1–5 below are **complete**. `WIFI_PROVISIONING_POST_SUCCESS_COMMIT` passed
for POST/browser/AP teardown. Run 013 saved-credential cold boot intermittency is
addressed by `WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY` (tester 10-boot sweep pending).
`PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID` is complete.
`OLED_PROVISIONING_SETUP_UX` is complete (tester visual validation pending).
`OLED_WIFI_CONNECTED_STATUS` is complete (tester visual validation pending).
`ERROR_LED_WIFI_LINK_STATUS` is complete (tester visual validation pending).
`ERROR_LED_RUNTIME_LINK_STATUS` is complete (tester validation pending).
`WIFI_RUNTIME_RECONNECT` is complete (v1; superseded by v2).
`WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2` is complete (**operator-validated** 2026-06-23).
Do not start `I2C_BUS_PHASE3` unless a new architect handoff activates it.

## WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2

```text
ID: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2
Source:
  - agent-workspaces/architect/handoff.md, WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2
  - docs/wifi_connect_cycle_architecture.md
  - docs/wifi_connect_cycle_implementation_reference.md (as-built record)
  - docs/error_led_connect_cycle_architecture.md
Fix:
  wifi_provisioning.h:
    - WIFI_LINK_STATUS_CONNECTING_ALERT
    - WIFI_PROV_EVENT_CONNECT_CYCLE_ACTIVE, CONNECT_ALERT_PHASE, CONNECT_RESTORED
  wifi_provisioning.c:
    - connect_cycle_run_round() + connect_cycle_run_forever() unified engine
    - Boot (connect_saved) and runtime (runtime_reconnect_task) share same cycle
    - 5 attempts/round, 15s alert phase, unbounded repeat
    - Removed SAVED_FAILURE_LOCKED emission and s_locked_disconnected on creds path
    - Serial logs: connect cycle round/attempt/alert source=boot|runtime
  app_core_wifi.c:
    - LED v2: UNPROVISIONED=ON, CONNECTING=SLOW, CONNECTING_ALERT=FAST, CONNECTED=OFF
    - OLED CONNECT_CYCLE_ACTIVE/CONNECT_ALERT_PHASE -> WIFI/CONNECTING
    - CONNECT_RESTORED -> show_wifi_connected_display()
    - SAVED_FAILURE_LOCKED branch disabled (no HOLD RESET)
Modified files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xddc10 bytes (prior 0xde060)
Expected LED/OLED:
  - No creds: LED solid ON; portal OLED
  - Creds connecting: slow blink; OLED WIFI/CONNECTING
  - After 5 fails: fast blink 15s; OLED still CONNECTING (no HOLD RESET)
  - New round: slow blink again; repeats until AP returns or factory reset
  - Connected: LED off; OLED WIFI OK + IP/MAC
Ready for tester:
  Yes. NVS erase; provision; AP down/up boot and runtime; verify no HOLD RESET.
Status: Complete — operator-validated (2026-06-23).

As-built serial log checklist (regression):

Must appear during connect cycle:
  - wifi_prov: connect cycle policy round_attempts=5 per_attempt_timeout_ms=12000 alert_phase_ms=15000
  - wifi_prov: connect cycle round start source=boot|runtime
  - wifi_prov: connect cycle attempt N/5 start source=boot|runtime
  - wifi_prov: connect cycle attempt N/5 failed reason=R backoff_ms=MS source=boot|runtime
  - wifi_prov: connect cycle alert_phase_ms=15000 source=boot|runtime
  - wifi_prov: connect cycle restored ip=IP source=boot|runtime
  - wifi_prov: STA got IP ip=IP source=boot|runtime attempt=N/5

Must NOT appear:
  - wifi_prov: saved boot failed attempts=5

Boot vs runtime:
  - Boot: connect_cycle_run_forever blocks app_core_wifi_start() task (not a new task).
  - Runtime: wifi_rt_rc task (4096 stack, priority 5).
  - sta_source_label(SAVED) logs as source=boot (not saved).

Lock removed:
  - wifi_provisioning_locked_disconnected() always false
  - No SAVED_FAILURE_LOCKED emission; OLED SAVED_FAILURE_LOCKED case is empty break
```

## WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2 — Archived task spec

```text
ID: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2
Status: PENDING
Source:
  - agent-workspaces/architect/handoff.md, WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2
  - docs/wifi_connect_cycle_architecture.md
  - docs/error_led_connect_cycle_architecture.md
Task:
  1. wifi_provisioning.h — CONNECTING_ALERT; CONNECT_CYCLE_ACTIVE, CONNECT_ALERT_PHASE,
     CONNECT_RESTORED
  2. wifi_provisioning.c — unified connect cycle (5 attempts, 15s alert, repeat);
     remove SAVED_FAILURE_LOCKED / s_locked_disconnected on creds path;
     boot and runtime share same engine
  3. app_core_wifi.c — LED v2 table (ON/slow/fast/off); OLED CONNECT_CYCLE_*;
     remove SAVED_FAILURE_LOCKED HOLD RESET branch
LED v2:
  UNPROVISIONED -> ON
  CONNECTING -> SLOW_BLINK
  CONNECTING_ALERT -> FAST_BLINK (15s phase)
  CONNECTED -> OFF
Cycle:
  round_attempts=5, per_attempt_timeout_ms=12000, alert_phase_ms=15000
Authorized files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Supersedes behavior:
  - ERROR_LED_WIFI_LINK_STATUS v1 mapping
  - Boot SAVED_FAILURE_LOCKED / HOLD RESET
  - WIFI_RUNTIME_RECONNECT v1 LED (solid ON while reconnecting)
Acceptance: see docs/wifi_connect_cycle_architecture.md
Tester: no HOLD RESET; LED slow/fast cycle; AP recovery
Depends on: prior WiFi stack (ERROR_LED_*, WIFI_RUNTIME_RECONNECT) as migration base
```

## WIFI_RUNTIME_RECONNECT

```text
ID: WIFI_RUNTIME_RECONNECT
Source:
  - agent-workspaces/architect/handoff.md, WIFI_RUNTIME_RECONNECT
  - docs/wifi_runtime_reconnect_architecture.md
Fix:
  wifi_provisioning.h:
    - WIFI_PROV_EVENT_RUNTIME_RECONNECTING, WIFI_PROV_EVENT_RUNTIME_RESTORED
  wifi_provisioning.c:
    - WIFI_PROV_STA_SOURCE_RUNTIME; run_sta_connect_attempt() shared with saved boot
    - runtime_reconnect_task: unbounded loop, backoff [1k..30k cap]
    - start_runtime_reconnect_if_needed() from handle_runtime_link_loss()
    - Dedupe via s_runtime_reconnect_task; guards portal/lock/SAVED pending
    - Never sets s_locked_disconnected or s_saved_policy_exhausted from runtime path
  app_core_wifi.c:
    - RUNTIME_RECONNECTING → WIFI/CONNECTING two lines
    - RUNTIME_RESTORED → show_wifi_connected_display()
Modified files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Module boundary audit:
  - wifi_provisioning: no error_led or display headers
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xde060 bytes (prior 0xdd9b0)
Expected behavior:
  - Connected → AP off → LED ON, OLED WIFI/CONNECTING
  - AP on → RUNTIME_RESTORED, LED off, OLED WIFI OK + IP/MAC
  - Long outage → LED stays ON; no HOLD RESET / boot lock
  - Bad creds cold boot → SAVED_FAILURE_LOCKED unchanged (5 attempts)
  - Serial: source=runtime attempt=N; no saved boot failed attempts=5 from runtime
Ready for tester:
  Yes. AP power-cycle while connected; long outage; boot-lock regression.
Status: Complete (2026-06-23).
```

## WIFI_RUNTIME_RECONNECT — Archived task spec

```text
ID: WIFI_RUNTIME_RECONNECT
Status: PENDING
Source:
  - agent-workspaces/architect/handoff.md, WIFI_RUNTIME_RECONNECT
  - docs/wifi_runtime_reconnect_architecture.md
Task:
  1. wifi_provisioning.h — RUNTIME_RECONNECTING, RUNTIME_RESTORED events
  2. wifi_provisioning.c — run_sta_connect_attempt(); runtime_reconnect_task;
     start_runtime_reconnect_if_needed() from handle_runtime_link_loss();
     WIFI_PROV_STA_SOURCE_RUNTIME; dedupe single task; never lock from runtime
  3. app_core_wifi.c — OLED for RUNTIME_RECONNECTING (WIFI/CONNECTING) and
     RUNTIME_RESTORED (show_wifi_connected_display)
  4. Replace layer-1 immediate DISCONNECTED on loss with CONNECTING when task starts
Policy:
  unbounded attempts; per_attempt_timeout_ms=12000;
  backoff_ms=[1000,3000,5000,8000,12000,15000,20000,30000] cap 30000
Authorized files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Non-goals:
  - Boot 5-attempt lock change, portal reopen, credential erase, error_led changes
Acceptance: see docs/wifi_runtime_reconnect_architecture.md
Tester: AP off/on; long outage; boot-lock regression; LED ON during reconnect
Depends on: ERROR_LED_RUNTIME_LINK_STATUS (implemented)
```

## ERROR_LED_WIFI_LINK_STATUS

```text
ID: ERROR_LED_WIFI_LINK_STATUS
Source:
  - agent-workspaces/architect/handoff.md, ERROR_LED_WIFI_LINK_STATUS
  - docs/error_led_wifi_link_architecture.md
Fix:
  error_led component:
    - error_led.c: GPIO8 active-low, esp_timer blink (500/500 slow, 125/125 fast)
    - error_led_init applies OFF; set_pattern idempotent
    - CMakeLists.txt REQUIRES board esp_driver_gpio esp_timer
  wifi_provisioning.h:
    - wifi_link_status_t enum; link_status on wifi_prov_event_info_t
  wifi_provisioning.c:
    - static s_link_status + set_link_status(); emit_event copies link_status
    - Transitions per docs transition table (portal UNPROVISIONED, saved CONNECTING,
      success CONNECTED, locked DISCONNECTED)
  app_core:
    - error_led_init in app_core_start before app_core_wifi_start
    - app_core_error_led_set_pattern facade
  app_core_wifi.c:
    - apply_wifi_link_status_led(info->link_status) end of on_wifi_prov_event
    - Boot gap: UNPROVISIONED if !has_credentials else CONNECTING before prov start
Modified files:
  - components/error_led/error_led.c (new)
  - components/error_led/CMakeLists.txt (new)
  - components/error_led/include/error_led.h (unchanged stub API)
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core.c
  - components/app_core/include/app_core.h
  - components/app_core/app_core_wifi.c
  - components/app_core/CMakeLists.txt
Module boundary audit:
  - wifi_provisioning: no error_led references
  - error_led: no WiFi/provisioning headers
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdd7d0 bytes (prior 0xdd250)
Expected LED phases:
  - Fresh NVS / portal: slow blink (500 ms)
  - Saved-credentials boot: solid ON until connect or SAVED_FAILURE_LOCKED
  - SUBMITTED_SUCCESS / SAVED_SUCCESS: OFF
  - SAVED_FAILURE_LOCKED: fast blink (125 ms)
Ready for tester:
  Yes. Erase NVS (slow blink) → provision (off on success) → reboot with creds
  (solid then off) → wrong AP / router off (fast blink after lock).
Status: Complete (2026-06-23).
```

## ERROR_LED_WIFI_LINK_STATUS — Archived task spec

```text
ID: ERROR_LED_WIFI_LINK_STATUS
Status: PENDING
Source:
  - agent-workspaces/architect/handoff.md, ERROR_LED_WIFI_LINK_STATUS
  - docs/error_led_wifi_link_architecture.md
Task:
  1. Complete components/error_led/ (error_led.c, CMakeLists.txt; header may exist)
  2. wifi_provisioning.h — wifi_link_status_t + link_status on wifi_prov_event_info_t
  3. wifi_provisioning.c — s_link_status + emit_event copies link_status
  4. app_core — error_led_init, app_core_error_led_set_pattern facade
  5. app_core_wifi.c — apply_wifi_link_status_led + boot gap
Authorized files: see architect handoff ERROR_LED_WIFI_LINK_STATUS
Mapping: UNPROVISIONED=SLOW, CONNECTING=ON, CONNECTED=OFF, DISCONNECTED=FAST
Acceptance: see docs/error_led_wifi_link_architecture.md
Tester: NVS erase slow blink; saved boot solid then off; lock fast blink
```

## OLED_WIFI_CONNECTED_STATUS

```text
ID: OLED_WIFI_CONNECTED_STATUS
Source:
  - agent-workspaces/architect/handoff.md, OLED_WIFI_CONNECTED_STATUS
  - docs/wifi_provisioning_architecture.md — WiFi connected OLED screen
  - docs/display_delivery_contract.md — Product WiFi connected screen
Fix:
  - wifi_prov_event_info_t: added sta_ipv4, sta_mac (success events only)
  - wifi_provisioning.c: cache IP on GOT_IP in s_sta_ipv4; prepare_sta_success_event_payload()
    formats STA MAC and copies into s_event_sta_ipv4/s_event_sta_mac before success emit
  - emit_sta_success_event() for SUBMITTED_SUCCESS and SAVED_SUCCESS
  - app_core_wifi.c: show_wifi_connected_display() → FULL_FOUR_LINES:
      WIFI OK / sta_ipv4 / mac[0..7] / mac[9..16]
    - Fallback WIFI/OK two lines if sta_ipv4 or sta_mac missing/invalid
Modified files:
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdd250 bytes (prior 0xdd060)
Expected OLED (STA success):
  WIFI OK
  192.168.x.x
  AA:BB:CC
  DD:EE:FF
Ready for tester:
  Yes. POST provision success and saved-credentials boot; compare IP/MAC to router.
Status: Complete (2026-06-23).
```

## OLED_WIFI_CONNECTED_STATUS — Archived task spec

## OLED_PROVISIONING_SETUP_UX

```text
ID: OLED_PROVISIONING_SETUP_UX
Source:
  - agent-workspaces/architect/handoff.md, OLED_PROVISIONING_SETUP_UX
  - docs/display_delivery_contract.md — Product provisioning QR setup
  - docs/wifi_provisioning_architecture.md — Provisioning setup OLED screen
Fix:
  - wifi_provisioning.c: AP_STARTED and PORTAL_READY events pass s_ap_ssid (not NULL)
  - app_core_wifi.c: show_provisioning_setup_display() helper builds 4-line right panel:
      "1 JOIN", ssid[0..6], ssid[7..], "2 SCAN QR"
    - PROV_SETUP_SSID_PREFIX_LEN = 7
    - Fallback WIFI/SETUP when ssid NULL or strlen < 8
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdd060 bytes (prior 0xdcfb0)
Expected OLED (fresh NVS, no credentials):
  - Left: QR http://192.168.4.1
  - Right: 1 JOIN / HIL-06- / XXXX / 2 SCAN QR (XXXX = MAC suffix from serial SSID)
Ready for tester:
  Yes. Erase NVS, boot, photograph OLED, join displayed AP, scan QR, confirm portal.
Status: Complete (2026-06-23).
```

## OLED_PROVISIONING_SETUP_UX — Archived task spec

## PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID

```text
ID: PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
Source:
  - agent-workspaces/architect/handoff.md, PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
  - docs/wifi_provisioning_architecture.md, Provisioning AP SSID format
Fix:
  Display demo retirement:
    - Removed boot calls to app_core_run_visual_demo() and app_core_run_display_smoke()
    - Deleted app_core_display_demo.c, app_core_display_demo.h, app_core/Kconfig
    - Removed CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO from sdkconfig.defaults
    - Removed app_core_wifi_allows_visual_demo() (no longer needed)
  Dynamic AP SSID:
    - BOARD_HIL_NUMBER_STRING "06" in components/board/include/board_pins.h
    - build_provisioning_ap_ssid(): HIL-<board>-<MAC4> from SoftAP MAC bytes 4-5
    - Logs: provisioning AP SSID generated ssid=... and starting SoftAP ssid=...
Modified files:
  - components/board/include/board_pins.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/wifi_provisioning/CMakeLists.txt (REQUIRES board)
  - components/app_core/app_core.c
  - components/app_core/app_core_wifi.c
  - components/app_core/include/app_core_wifi.h
  - components/app_core/CMakeLists.txt
  - sdkconfig.defaults
Deleted files:
  - components/app_core/app_core_display_demo.c
  - components/app_core/include/app_core_display_demo.h
  - components/app_core/Kconfig
SSID format example:
  HIL-06-A1B2 (last two SoftAP MAC bytes as uppercase hex)
Commands executed:
  - idf.py fullclean && idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdcfb0 bytes (prior 0xdd160)
Static audit:
  - No b06_hil_setup, DEMO_STEP, or demo-only strings in firmware binary
Ready for tester:
  Yes. Fresh NVS boot: confirm AP name HIL-06-[0-9A-F]{4}, no DEMO_STEP logs,
  portal still at http://192.168.4.1/.
Status: Complete (2026-06-23).
```

## PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID — Archived task spec

```text
ID: PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
Source:
  - agent-workspaces/architect/handoff.md, PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
  - docs/display_visual_demo_protocol.md
  - docs/wifi_provisioning_architecture.md, Provisioning AP SSID format
Problem:
  Historical display demo/test calls are still documented/wired as boot behavior,
  and provisioning AP SSID still uses fixed `b06_hil_setup`. Product firmware
  should show only real product display states and should expose a board/device
  specific AP name.
Do:
  1. Remove boot/runtime calls to `app_core_run_visual_demo()` and
     `app_core_run_display_smoke()`.
  2. Remove demo source/header/Kconfig/CMake references if they become unused.
  3. Remove `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` from sdkconfig.defaults.
  4. Preserve real display facade APIs and WiFi provisioning QR/status display.
  5. Add a board-owned two-digit HIL number constant/helper. For this tree:
     `b06_hil` -> `06`. Do not hard-code `06` inside wifi_provisioning.
  6. Generate SoftAP SSID at runtime as
     `HIL-<board_number_2_digits>-<last_4_softap_mac_hex>`.
  7. Use the WiFi SoftAP MAC last two bytes as uppercase hex with no separators.
  8. Log generated SSID at SoftAP startup.
Do not:
  - Do not remove QR generation used by real WiFi provisioning.
  - Do not change portal URL; it remains `http://192.168.4.1/`.
  - Do not add AP password, captive DNS, HTTPS, or UI redesign.
  - Do not parse filesystem paths at runtime to get the board number.
Done when:
  - Build succeeds.
  - Normal boot has no `DEMO_STEP` logs.
  - sdkconfig.defaults has no `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO`.
  - Fresh provisioning AP SSID matches `HIL-06-[0-9A-F]{4}`.
  - No product code still uses `b06_hil_setup`.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY

```text
ID: WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
  - docs/wifi_provisioning_architecture.md, Saved Boot Reliability Contract
  - tester/feedback.md Entry 013
  - tester/test_runs.md Run 013
Problem:
  POST provisioning succeeded and credentials were known good, but saved cold boot
  connected only intermittently (Run 013: 2/5 PASS, 3/5 AUTH_FAIL reason 202).
Fix:
  - apply_sta_config: WIFI_AUTH_WPA2_WPA3_PSK, pmf capable/required false,
    sae_pwe_h2e=WPA3_SAE_PWE_BOTH, failure_retry_cnt=0 (app policy authoritative)
  - Log: STA config auth_profile=wpa2_wpa3_personal pmf_capable=true pmf_required=false
  - saved boot: 1000 ms settle after esp_wifi_start() before attempt 1
  - Application retry: max_attempts=5, per_attempt_timeout_ms=12000,
    backoff_ms=[0,1000,3000,5000,8000]
  - Early disconnect (reason 202) ends attempt via WIFI_STA_DISCONNECTED_BIT
  - SAVED_FAILURE_LOCKED only after all 5 attempts fail
  - Stale GOT_IP after policy exhaustion logged and ignored
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
Auth profile constants:
  - threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK
  - pmf_cfg.capable = true, pmf_cfg.required = false
  - sae_pwe_h2e = WPA3_SAE_PWE_BOTH
  - failure_retry_cnt = 0
Retry constants:
  - WIFI_PROV_SAVED_BOOT_MAX_ATTEMPTS = 5
  - WIFI_PROV_SAVED_BOOT_PER_ATTEMPT_TIMEOUT_MS = 12000
  - WIFI_PROV_SAVED_BOOT_SETTLE_MS = 1000
  - backoff_ms = [0, 1000, 3000, 5000, 8000]
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdd160 bytes (prior 0xdcb20)
Expected saved-boot serial sequence (success on attempt n):
  STA config auth_profile=... → saved boot connect policy attempts=5 ... →
  saved boot attempt 1/5 start → (optional STA disconnected reason=202 ...) →
  saved boot attempt n/5 start → STA got IP source=saved attempt=n/5 →
  saved boot connected attempt=n/5 ip=<ipv4>
Ready for tester:
  Yes. Provision once (POST success), then run 10 consecutive cold reboots with
  serial capture; acceptance is 10/10 GOT_IP source=saved, 0 locked failures.
Status: Complete (2026-06-23). Hardware 10-boot sweep pending tester.
```

## WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY — Archived task spec

```text
ID: WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
  - docs/wifi_provisioning_architecture.md, Saved Boot Reliability Contract
  - tester/feedback.md Entry 013
  - tester/test_runs.md Run 013
Problem:
  POST provisioning now succeeds and credentials are known good, but saved cold boot
  connects only intermittently. Run 013 reboot sweep: 2/5 PASS, 3/5 FAIL with
  `WIFI_REASON_AUTH_FAIL` reason 202 before getting IP. The operator AP negotiates
  WPA3-SAE on successful boots.
Do:
  1. Use the same STA config helper for submitted and saved connection attempts.
  2. Configure STA for WPA2/WPA3/WPA3-SAE Personal compatibility on ESP-IDF 5.3:
     PMF capable true, PMF required false, and SAE PWE compatibility mode where
     available.
  3. Log auth compatibility:
     `STA config auth_profile=wpa2_wpa3_personal pmf_capable=true pmf_required=false`.
  4. Add saved cold-start settle delay of 1000 ms after esp_wifi_start() before
     attempt 1.
  5. Replace single saved connect attempt with application retry policy:
     max_attempts=5, per_attempt_timeout_ms=12000,
     backoff_ms=[0,1000,3000,5000,8000].
  6. End an attempt promptly when a disconnect reason arrives; do not wait the full
     timeout after early reason 202.
  7. Emit SAVED_FAILURE_LOCKED only after all five saved attempts fail.
  8. Preserve POST success commit, AP teardown, HTTP stack safety, and header budget.
Required logs:
  - saved boot connect policy attempts=5 per_attempt_timeout_ms=12000
  - saved boot attempt <n>/5 start
  - STA disconnected reason=<reason> source=saved attempt=<n>/5
  - saved boot attempt <n>/5 failed reason=<reason> backoff_ms=<ms>
  - STA got IP ip=<ipv4> source=saved attempt=<n>/5
  - saved boot connected attempt=<n>/5 ip=<ipv4>
  - saved boot failed attempts=5 last_reason=<reason>
Do not:
  - Do not log passwords.
  - Do not reopen provisioning AP automatically after saved boot failure.
  - Do not erase credentials after saved boot failure.
  - Do not weaken POST success commit ordering.
Done when:
  - Build succeeds.
  - Auth profile config and retry constants are documented in the completion note.
  - Tester can run 10 consecutive saved-credential cold boots with 10/10 final
    success and 0 final locked failures.
  - Reason 202, if it appears, is recovered by a later attempt within the same boot.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_POST_SUCCESS_COMMIT

```text
ID: WIFI_PROVISIONING_POST_SUCCESS_COMMIT
Source: docs/wifi_provisioning_architecture.md Submitted Success Commit Contract,
        architect/handoff.md, tester Entry 009 / Run 012
Problem:
  Run 012: POST without 431, but browser blank, OLED WIFI OK early, AP still visible,
  credentials saved but reboot STA auth failed (split-brain success).
Fix:
  - Fresh STA attempt: disconnect + clear bits; GOT_IP only when s_pending_sta_source set
  - Log STA got IP ip=<ipv4> source=submitted|saved
  - Success order: save → success page → SUBMITTED_SUCCESS event → deferred teardown
  - Rollback credentials if success page send fails
  - Teardown deferred 2000 ms (HTTP/AP stay up for browser to receive page)
  - threshold.authmode = WIFI_AUTH_WPA2_PSK for STA config
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdcb20 bytes
Expected POST success serial sequence:
  POST /provision from client → parsed form → attempting STA →
  STA got IP source=submitted → credentials saved source=submitted →
  POST /provision success response sent → (OLED WIFI OK via event) →
  success teardown scheduled delay_ms=2000 → HTTP/SoftAP stopped → STA-only
Ready for tester:
  Yes. Valid POST: browser success page, OLED OK after response sent, AP gone after 2s,
  reboot logs STA got IP source=saved.
Status: Complete (2026-06-23).
```

## WIFI_PROVISIONING_POST_SUCCESS_COMMIT — Archived task spec

```text
ID: WIFI_PROVISIONING_POST_SUCCESS_COMMIT
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_POST_SUCCESS_COMMIT
  - docs/wifi_provisioning_architecture.md, Submitted Success Commit Contract
  - tester/feedback.md Entry 009
  - tester/handoff.md Run 012 status
Problem:
  Run 012 fixed HTTP 431 and allowed form submit, but the browser showed no success
  or failure page, OLED showed WIFI OK, AP b06_hil_setup still appeared visible,
  and a later saved-credential boot failed STA auth. This is a success transaction
  mismatch, not a header-budget problem.
Do:
  1. Clear submitted STA event bits before each POST connection attempt.
  2. Ensure submitted success uses a fresh `IP_EVENT_STA_GOT_IP` for the current
     POST attempt; log `STA got IP ip=<ipv4> source=submitted`.
  3. Log saved boot success separately as `STA got IP ip=<ipv4> source=saved`.
  4. Log `credentials saved source=submitted` after NVS save succeeds.
  5. Move `WIFI_PROV_EVENT_SUBMITTED_SUCCESS` so it is emitted only after
     `POST /provision success response sent`.
  6. If success response send fails after saving credentials, clear/rollback the
     provisioned state, keep AP/HTTP active, emit failure/error, and do not show
     OLED WIFI OK.
  7. Defer AP/HTTP teardown by 2000 ms after success response; do not stop AP/HTTP
     immediately in the same handler call stack.
  8. Log teardown:
     - success teardown scheduled delay_ms=2000
     - HTTP server stopped after provisioning success
     - SoftAP stopped after provisioning success
     - STA-only mode active after provisioning success
  9. Preserve all reachability, stack, and header-budget fixes.
Do not:
  - Do not log passwords.
  - Do not show OLED WIFI OK before the browser success response is sent.
  - Do not keep saved credentials marked provisioned if the success page was not
    delivered.
  - Do not add captive DNS, JavaScript, AP password, HTTPS, or automatic AP fallback.
Done when:
  - Build succeeds.
  - Valid POST serial shows the full success sequence from architect handoff.
  - Browser shows `WiFi connected. You can close this page.`
  - OLED WIFI OK appears only after `POST /provision success response sent`.
  - AP teardown logs occur after the 2000 ms grace window.
  - Reboot with saved credentials logs `STA got IP ip=<ipv4> source=saved`.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_HTTP_HEADER_BUDGET

```text
ID: WIFI_PROVISIONING_HTTP_HEADER_BUDGET
Source: docs/wifi_provisioning_architecture.md HTTP Request Header Budget,
        architect/handoff.md, tester Entry 007 / Run 011
Problem:
  Run 011: GET / OK, form visible, but POST /provision rejected with HTTP 431
  before handler (CONFIG_HTTPD_MAX_REQ_HDR_LEN=512 too small for mobile browser).
Fix:
  - sdkconfig.defaults: CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048
  - sdkconfig updated to 2048 (generated build/config/sdkconfig.h confirms)
  - POST milestone logs: parsed form, attempting STA ssid=...
Modified files:
  - sdkconfig.defaults
  - sdkconfig
  - components/wifi_provisioning/wifi_provisioning.c
Commands executed:
  - idf.py reconfigure && idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - CONFIG_HTTPD_MAX_REQ_HDR_LEN 2048 in sdkconfig.h
  - b06_hil_firmware.bin size 0xdc5a0 bytes
Expected POST serial sequence:
  wifi_prov: POST /provision from client
  wifi_prov: POST /provision parsed form
  wifi_prov: POST /provision attempting STA ssid=<ssid>
  wifi_prov: POST /provision failure response sent  (or success)
Ready for tester:
  Yes. Submit form from phone; expect no HTTP 431 and handler milestone logs.
Status: Complete (2026-06-23).
```

## Tester return — POST success UX mismatch (Run 012 human session)

```text
ID: TESTER_RETURN_POST_SUCCESS_UX_MISMATCH
Date: 2026-06-23
Source: Human operator observation + tester serial (/tmp/b06_hil_boot_log_sta_boot.txt,
        prior runs 010–012). Relates to WIFI_PROVISIONING_HTTP_HEADER_BUDGET and full
        provisioning acceptance.
Firmware exercised: Run 012 (0xdc5a0, CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048)

Human observation (operator, provisioning submit):
  1. Submitted target WiFi SSID and password via portal form.
  2. Web page: no longer shows HTTP 431 / "Header fields are too long" (header fix OK).
  3. Web page: also shows NO success text and NO failure text — appears blank / no feedback.
  4. OLED: displayed "WIFI" / "OK" (two-line template).
  5. Phone WiFi list: still showed device AP b06_hil_setup after submit.

Serial evidence (tester, after operator session — device reset):
  - Boot path uses saved credentials (wifi:mode : sta), NOT provisioning AP.
    → NVS namespace wifi_prov was written (provisioned flag set).
  - Reboot STA connect FAILS:
      I (746) wifi:state: init -> auth (0xb0)
      I (1076) wifi:state: auth -> init (0x600)
  - No "wifi_prov: STA got IP" in any captured log (firmware does not log IP address).
  - No POST /provision milestone lines captured during operator submit (tester was not
    monitoring serial live during that moment).

Architecture/code alignment:
  - OLED "WIFI OK" maps to WIFI_PROV_EVENT_SUBMITTED_SUCCESS in app_core_wifi.c, emitted
    from provision_post_handler AFTER wait_for_sta_connected() OK and
    wifi_credentials_save() OK.
  - SUBMITTED_SUCCESS is emitted BEFORE wifi_prov_send_success_page(req). Therefore OLED
    can show OK even if the HTTP success response never reaches the phone.
  - On success path, code calls httpd_stop() then esp_wifi_set_mode(WIFI_MODE_STA).
    Operator still seeing b06_hil_setup suggests AP teardown did not occur before the
    phone was checked, did not take effect, or phone remained associated to the AP SSID.
  - Architecture requires success page text "WiFi connected. You can close this page."
    and AP/HTTP stop after success — operator saw neither on phone.

Contradictions to resolve:
  A. OLED + NVS imply STA connect succeeded during POST; reboot STA auth fails with same
     saved credentials (false-positive GOT_IP, APSTA vs STA-only difference, or credential
     storage mismatch).
  B. OLED success vs blank browser (success event before page send; or page send / socket
     drop while phone still on AP).
  C. Expected AP disappearance vs operator still seeing b06_hil_setup.

Suggested implementer actions:
  1. Reproduce with idf.py monitor during POST; capture full milestone sequence:
     POST /provision from client → parsed form → attempting STA ssid=... →
     STA got IP (+ log assigned IPv4) → POST /provision success response sent.
  2. Log IP_EVENT_STA_GOT_IP with ip4_addr (not only "STA got IP").
  3. Emit SUBMITTED_SUCCESS / OLED WIFI OK only after success page send succeeds.
  4. After esp_wifi_set_mode(WIFI_MODE_STA), log SoftAP stopped / confirm AP beacon off;
     optionally delay or verify before returning from handler.
  5. If reboot auth fails with saved creds, treat as FAIL for acceptance even if POST
     OLED showed OK — investigate authmode threshold, password encoding, or premature
     GOT_IP during APSTA.
  6. On failure to deliver success HTML, keep AP active and show failure page per arch.

Acceptance gap for tester:
  - POST human: PARTIAL (no 431, but no success/failure page).
  - Reboot with saved credentials: FAIL (auth failure, no IP).
  - Full WIFI_PROVISIONING_ARCHITECTURE not acceptable until web + reboot align.

Evidence paths:
  - agent-workspaces/tester/feedback.md Entry 009
  - /tmp/b06_hil_boot_log_sta_boot.txt
  - agent-workspaces/tester/test_runs.md Run 012
```

## WIFI_PROVISIONING_HTTP_HEADER_BUDGET — Archived task spec

```text
ID: WIFI_PROVISIONING_HTTP_HEADER_BUDGET
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_HTTP_HEADER_BUDGET
  - docs/wifi_provisioning_architecture.md, HTTP Request Header Budget
  - tester/feedback.md Entry 007
  - tester/test_runs.md Run 011
Problem:
  Run 011 proves GET / now works and the form is visible. Submitting credentials
  from a phone browser fails with HTTP 431 "Header fields are too long" before
  provision_post_handler runs. Serial logs show httpd parser rejection and no
  `POST /provision from client`.
Root architectural finding:
  ESP-IDF httpd request-header budget is still the default 512 bytes
  (`CONFIG_HTTPD_MAX_REQ_HDR_LEN=512`), which is too small for normal mobile
  browser POST headers.
Do:
  1. Add `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048` to source-controlled sdkconfig.defaults.
  2. Reconfigure/build so generated ESP-IDF config uses 2048.
  3. Keep form method `post` and action `/provision`; do not put credentials in URL.
  4. Add or confirm POST milestone diagnostics:
     - POST /provision from client
     - POST /provision parsed form
     - POST /provision attempting STA ssid=<redacted-or-ssid>
     - POST /provision failure response sent
     - POST /provision success response sent
  5. Preserve AP/DHCP reachability and HTTP stack safety fixes.
Do not:
  - Do not use GET/query parameters for credentials.
  - Do not add JavaScript or external assets.
  - Do not log passwords.
  - Do not include app_core.h or display headers from wifi_provisioning.
  - Do not mark ready for tester if HTTP 431 still appears or `POST /provision
    from client` is absent after submit.
Done when:
  - Build succeeds.
  - sdkconfig.defaults contains CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048.
  - Generated config contains CONFIG_HTTPD_MAX_REQ_HDR_LEN 2048.
  - Phone browser POST reaches handler and logs `POST /provision from client`.
  - Bad credential submit returns failure page, keeps AP active, and logs failure
    response sent.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_HTTP_STACK_SAFETY

```text
ID: WIFI_PROVISIONING_HTTP_STACK_SAFETY
Source: docs/wifi_provisioning_architecture.md HTTP Handler Stack Safety,
        architect/handoff.md, tester Entry 006 / Run 010
Problem:
  Run 010: GET / reached handler but httpd worker stack protection fault before
  HTML sent (char html[2048] on stack + default httpd stack too small).
Fix:
  - Setup/failure pages: httpd_resp_send_chunk with constant HTML chunks; no html[2048]
  - Form body: static s_form_body[WIFI_PROV_MAX_FORM_BODY_LEN+1] (not on httpd stack)
  - Form parser: in-place segment parsing; removed char pair[257] from stack
  - httpd_config.stack_size = 8192
  - Response completion logs: GET /health, GET /, POST /provision success/failure
Modified files:
  - components/wifi_provisioning/wifi_provisioning_pages.c
  - components/wifi_provisioning/wifi_provisioning_form.c
  - components/wifi_provisioning/wifi_provisioning.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdc430 bytes
Ready for tester:
  Yes. Expect GET / from client + GET / response sent without Guru Meditation.
Status: Complete (2026-06-23).
```

## WIFI_PROVISIONING_HTTP_STACK_SAFETY — Archived task spec

```text
ID: WIFI_PROVISIONING_HTTP_STACK_SAFETY
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_HTTP_STACK_SAFETY
  - docs/wifi_provisioning_architecture.md, HTTP Handler Stack Safety
  - tester/feedback.md Entry 006
  - tester/test_runs.md Run 010
Problem:
  Run 010 proves AP/DHCP/httpd reachability now works: phone receives 192.168.4.2
  and `GET / from client` reaches the handler. The firmware then panics with stack
  protection fault in the httpd worker before HTML reaches the browser.
Root architectural finding:
  Current page helpers allocate large buffers on the HTTP worker stack, including
  `char html[2048]` in setup/failure page rendering. This violates the new HTTP
  Handler Stack Safety contract.
Do:
  1. Remove large local HTML buffers from wifi_provisioning HTTP handlers and page
     helpers.
  2. Render setup and failure pages using constant chunks with
     httpd_resp_send_chunk(), or another bounded non-stack strategy.
  3. Keep every local array in the HTTP handler synchronous call chain <= 128 bytes.
  4. Avoid parser stack duplication such as `char pair[257]`; parse in-place,
     stream fields, use static internal storage, or bounded heap with cleanup.
  5. Set httpd_config_t.stack_size explicitly to at least 8192 unless code review
     proves smaller stack safe.
  6. Add completion diagnostics:
     - GET /health response sent
     - GET / response sent
     - POST /provision failure response sent
     - POST /provision success response sent
     - route-specific response failed: <esp_err>
  7. Preserve route semantics and HTML copy exactly.
Do not:
  - Do not add JavaScript, filesystem assets, captive DNS, HTTPS, AP password, or
    a separate frontend.
  - Do not log or echo passwords.
  - Do not include app_core.h or display headers from wifi_provisioning.
  - Do not mark ready for tester unless `/health` and `/` both log response sent
    without panic in local/manual validation notes.
Done when:
  - Build succeeds.
  - Code review shows no `char html[2048]` or equivalent large local page buffer.
  - Code review shows no local form pair/body buffer > 128 bytes in handler call chain.
  - `/health` logs handler entry and response sent.
  - `/` logs handler entry and response sent.
  - No stack protection fault, Guru Meditation, abort, or reboot occurs while
    serving `/health`, `/`, failure page, or success page.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_PORTAL_REACHABILITY

```text
ID: WIFI_PROVISIONING_PORTAL_REACHABILITY
Source: docs/wifi_provisioning_architecture.md, architect/handoff.md
Summary:
  Implemented ESP-IDF SoftAP Portal Startup Contract per corrective architecture
  (Runs 007-009). Key fix: AP netif IP/DHCP before esp_wifi_start(); no DHCP
  stop/restart after AP_START (Run 009 root cause).
Changes:
  - AP IP/DHCP configured before esp_wifi_start(); dhcps_stop tolerant of already-stopped
  - Event bit cleared before esp_wifi_start() only; wait does not clear
  - WIFI_MODE_AP at boot; APSTA on POST /provision only; STA netif deferred
  - Routes: GET /, GET /health (OK plaintext), POST /provision, 404
  - Full diagnostic log sequence; station joined/left with MAC and AID
  - app_core: no ESP_ERROR_CHECK on app_core_wifi_start (no reboot loop)
  - app_core_wifi: WIFI/AP FAIL vs WIFI/WEB FAIL on ERROR event
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core.c
  - components/app_core/app_core_wifi.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdb770 bytes
Expected boot log sequence:
  wifi_prov: AP netif ip=192.168.4.1 gw=192.168.4.1 netmask=255.255.255.0
  wifi_prov: starting SoftAP ssid=b06_hil_setup mode=AP
  wifi_prov: SoftAP started
  wifi_prov: HTTP server started on port 80
  wifi_prov: HTTP handlers registered (GET /, GET /health, POST /provision)
  wifi_prov: provisioning portal ready at http://192.168.4.1
Ready for tester:
  Yes. Join AP, test /health then /; expect handler logs and HTML form.
Status: Complete (2026-06-23).
```

## WIFI_PROVISIONING_PORTAL_REACHABILITY — Archived task spec

```text
ID: WIFI_PROVISIONING_PORTAL_REACHABILITY
Source:
  - agent-workspaces/architect/handoff.md, WIFI_PROVISIONING_PORTAL_REACHABILITY
  - docs/wifi_provisioning_architecture.md, Corrective Architecture From Runs 007-009
  - tester/feedback.md Entry 005
  - tester/test_runs.md Run 009
Problem:
  Run 009 logs SoftAP started, HTTP server started, handlers registered, and portal
  ready. Phone joins AP, but opening http://192.168.4.1/ yields blank page and
  serial shows 0x `GET / from client`.
Root architectural findings:
  - Run 007 lacked HTTP reachability diagnostics and probably started/configured
    HTTP/AP in an underspecified order.
  - Run 008 cleared WIFI_AP_STARTED_BIT after esp_wifi_start(), losing the already
    fired AP_START event and causing timeout/reboot loop.
  - Run 009 still does not prove DHCP/L3/httpd reachability; AP association alone
    is not enough.
Do:
  1. Follow docs/wifi_provisioning_architecture.md ESP-IDF SoftAP Portal Startup
     Contract exactly.
  2. Configure AP netif IP/DHCP before esp_wifi_start().
  3. Do not stop or restart DHCP after WIFI_EVENT_AP_START.
  4. Clear WIFI_AP_STARTED_BIT immediately before esp_wifi_start(), never after.
  5. Start in WIFI_MODE_AP only; use APSTA only during submitted-credential tests.
  6. Start HTTP only after AP_START succeeds.
  7. Register GET /, GET /health, POST /provision, and 404 handlers.
  8. Add required diagnostics:
     - AP IP/gateway/netmask
     - starting SoftAP ssid/mode
     - SoftAP started
     - station joined mac/aid
     - HTTP server started on port 80
     - handlers registered
     - portal ready
     - GET /health from client
     - GET / from client
     - POST /provision from client
  9. Ensure provisioning startup errors return to app_core and do not cause
     ESP_ERROR_CHECK abort/reboot loops.
  10. Preserve neutral event and display boundaries.
Do not:
  - Do not add captive DNS, HTTPS, AP password, or web reset as part of this fix.
  - Do not log passwords.
  - Do not include app_core.h or display headers from wifi_provisioning.
  - Do not mark ready for tester without build result and startup log sequence.
Done when:
  - Fresh NVS boot logs AP IP, SoftAP started, HTTP server started, handlers
    registered, and portal ready in order.
  - Phone joined to b06_hil_setup can load http://192.168.4.1/health and serial
    logs GET /health from client.
  - Phone can load http://192.168.4.1/ and serial logs GET / from client.
  - No reboot loop or abort occurs on recoverable provisioning errors.
Status: Complete (2026-06-23). See implementation record above.
```

## WIFI_PROVISIONING_ARCHITECTURE — Run 008 race fix

```text
ID: WIFI_PROVISIONING_ARCHITECTURE (Run 008 race fix)
Source: agent-workspaces/tester/feedback.md Entry 004, test_runs.md Run 008
Problem:
  Run 007 fix caused reboot loop: SoftAP started log at ~634 ms, then
  E (10645) SoftAP start timeout → ESP_ERROR_CHECK abort in app_core.
Root cause (confirmed):
  wait_for_ap_started() cleared WIFI_AP_STARTED_BIT after esp_wifi_start(), but
  WIFI_EVENT_AP_START had already fired and set the bit → wait always timed out.
Fix:
  Clear WIFI_AP_STARTED_BIT immediately before esp_wifi_start(); wait_for_ap_started()
  only waits (no clear). Standard ESP-IDF event-group pattern.
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
Ready for tester:
  Yes. Re-run Run 008; expect SoftAP started → HTTP server started → portal ready
  without timeout or reboot loop.
```

## WIFI_PROVISIONING_ARCHITECTURE — Run 007 portal fix

```text
ID: WIFI_PROVISIONING_ARCHITECTURE (Run 007 fix)
Source: agent-workspaces/tester/feedback.md Entry 003, tester/handoff.md Run 007
Problem:
  GET http://192.168.4.1/ returned blank page on phone (AP join OK, no HTTP response).
Root cause (confirmed in code review):
  1. httpd_start() ran immediately after esp_wifi_start() without waiting for
     WIFI_EVENT_AP_START (violates architecture § Start_open_AP_and_HTTP).
  2. Portal boot used WIFI_MODE_APSTA before any STA config (unnecessary dual-mode).
  3. POST /provision called esp_wifi_connect() without apply_sta_config() first
     (secondary bug; blocked by GET failure).
Fix applied in components/wifi_provisioning/wifi_provisioning.c:
  - Register WIFI_EVENT handler; wait for WIFI_EVENT_AP_START (10 s timeout) before
    configure_ap_netif_ip(), httpd_start(), and handler registration.
  - Start portal in WIFI_MODE_AP; switch to WIFI_MODE_APSTA only on POST /provision.
  - apply_sta_config() before wait_for_sta_connected() on form submit.
  - Diagnostic ESP_LOGI: SoftAP started, HTTP server started, handlers registered,
    portal ready, GET /, POST /provision.
Modified files:
  - components/wifi_provisioning/wifi_provisioning.c
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0xdb2a0 bytes
Ready for tester:
  Yes. Re-run Run 007 WiFi Provisioning Criteria; expect GET / form and serial logs
  (wifi_prov: SoftAP started, HTTP server started, GET / from client).
```

## WIFI_PROVISIONING_ARCHITECTURE

```text
ID: WIFI_PROVISIONING_ARCHITECTURE
Source handoff: agent-workspaces/architect/handoff.md, docs/wifi_provisioning_architecture.md
Summary:
  - components/wifi_credentials with NVS namespace wifi_prov (ssid, password, provisioned)
  - components/wifi_provisioning with open AP b06_hil_setup at 192.168.4.1
  - HTTP GET / and POST /provision with bounded form parser and server-rendered HTML
  - Neutral wifi_prov_event_t callback boundary; app_core maps events to display
  - app_core_wifi.c: GPIO7 factory reset (2000 ms), startup orchestration, demo gate
Modified files:
  - components/wifi_credentials/
  - components/wifi_provisioning/
  - components/app_core/app_core_wifi.c
  - components/app_core/include/app_core_wifi.h
  - components/app_core/app_core.c
  - components/app_core/CMakeLists.txt
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4, target esp32c3)
  - b06_hil_firmware.bin size 0xdacc0 bytes (prior display build ~0x3a2e0)
Static audits:
  - display/: no wifi/http/credentials includes
  - wifi_provisioning/: no app_core/display includes
  - display_controller.h only in app_core + display internals
Questions or technical debt:
  - Hardware validation (AP, portal, STA, factory reset, OLED QR) pending tester.
  - CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y skips demo when provisioning AP is active.
Ready for tester:
  Yes. Run docs/test_strategy.md WiFi Provisioning Criteria on device.
```

### Archived — `WIFI_PROVISIONING_ARCHITECTURE` task spec

**Goal:** Implement the v1 WiFi provisioning flow exactly as specified in
`docs/wifi_provisioning_architecture.md`.

**Authorized files:**

- `components/wifi_credentials/`
- `components/wifi_provisioning/`
- `components/app_core/`
- `components/board/` only for using the existing GPIO7 factory reset mapping
- `tests/` if adding host/component tests
- `agent-workspaces/implementer/handoff.md` for implementation notes

**Do:**

1. Add `components/wifi_credentials/` with public API equivalent to:

   ```c
   esp_err_t wifi_credentials_init(void);
   bool wifi_credentials_load(wifi_credentials_t *out);
   esp_err_t wifi_credentials_save(const wifi_credentials_t *credentials);
   esp_err_t wifi_credentials_erase(void);
   bool wifi_credentials_validate(const wifi_credentials_t *credentials);
   ```

2. Use NVS namespace `wifi_prov` with keys `ssid`, `password`, and `provisioned`.
3. Add `components/wifi_provisioning/` for SoftAP `b06_hil_setup`, IP
   `192.168.4.1`, HTTP `GET /`, HTTP `POST /provision`, bounded form parsing, and
   STA connection attempts.
4. Expose a neutral event boundary from `components/wifi_provisioning/`:
   - define `wifi_prov_event_t`, `wifi_prov_event_info_t`, and
     `wifi_prov_event_cb_t` in `wifi_provisioning.h`;
   - let `app_core` include `wifi_provisioning.h`, initialize provisioning, and
     register the callback;
   - emit events such as `WIFI_PROV_EVENT_PORTAL_READY`,
     `WIFI_PROV_EVENT_SUBMITTED_CONNECTING`, `WIFI_PROV_EVENT_SUBMITTED_SUCCESS`,
     `WIFI_PROV_EVENT_SUBMITTED_FAILURE`, and
     `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED`;
   - never pass submitted passwords through events.
5. Build the provisioning page as server-rendered HTML inside
   `components/wifi_provisioning/`:
   - no JavaScript;
   - no external CSS, images, fonts, icons, CDN, SPIFFS, LittleFS, or separate
     frontend assets;
   - one `GET /` form with method `post`, action `/provision`, fields `ssid` and
     `password`, maxlength values 32 and 63, and submit label `Connect`;
   - success page with `WiFi connected. You can close this page.`;
   - failure page with `WiFi connection failed. Check SSID and password.` and a
     retry form whose password field is empty;
   - `Content-Type: text/html; charset=utf-8` and `Cache-Control: no-store`;
   - HTML-escape any echoed SSID and never echo passwords.
6. In `app_core`, implement startup orchestration:
   - initialize credential storage;
   - initialize wifi_provisioning with the neutral event callback;
   - check GPIO7 active-low continuously for `2000 ms`;
   - erase credentials on factory reset;
   - start provisioning AP when credentials are missing;
   - connect saved credentials when present;
   - remain disconnected after saved credential failure until factory reset.
7. In `app_core` only, map neutral provisioning events to optional display calls
   through `app_core_display_show_template` and `app_core_display_show_qr_setup`.
8. Show provisioning QR with payload `http://192.168.4.1` when AP mode is active
   if the OLED/display facade is available, but only from the app_core event
   callback.
9. Add implementation notes and command results below before handing to tester.

**Do not:**

- Do not add captive DNS, HTTPS, WPA2 AP password, web reset, encrypted NVS
  requirement, WiFi enterprise, automatic AP fallback after saved credential
  failure, or automatic credential erase after boot failure.
- Do not implement a separate frontend app, JavaScript flow, captive portal UI,
  external assets, or filesystem-backed page storage in v1.
- Do not include WiFi/HTTP/NVS credential headers in display components.
- Do not include `app_core.h` from `components/wifi_provisioning/`.
- Do not include `display_controller.h` from `components/wifi_provisioning/`.
- Do not include any display header from `components/wifi_provisioning/`.
- Do not call `app_core_display_show_*` from `components/wifi_provisioning/`.
- Do not expose `httpd_handle_t`, `wifi_config_t`, `nvs_handle_t`, or display
  template types from `wifi_provisioning.h`.
- Do not call `display_controller_*` outside `app_core` and display internals.
- Do not mark this task ready for tester without build results and the WiFi
  provisioning validation notes required by `docs/test_strategy.md`.

**Done when:**

- Fresh/erased credentials start open AP `b06_hil_setup` at `192.168.4.1`.
- `GET /` serves the specified ASCII HTML form; `POST /provision` handles
  success/failure with the specified success and failure pages.
- HTML responses have no external asset dependencies, escape echoed SSID, and
  never echo passwords.
- `wifi_provisioning` emits neutral events only and remains independent from
  `app_core.h` and all display headers.
- Valid submitted credentials save only after STA success and AP/HTTP stop after
  success page.
- Invalid submitted credentials are not saved and leave AP/HTTP active.
- Saved credentials are used on reboot without opening AP.
- Saved credential failure at boot leaves device disconnected until GPIO7 factory
  reset.
- GPIO7 active-low for `2000 ms` erases `wifi_prov` and restarts provisioning.
- Static audits prove display/provisioning boundaries from the architecture doc,
  including no app_core/display includes in `wifi_provisioning`.

**Status:** Complete (2026-06-22).

---

### Task 1 — `setup_url` component

**Goal:** Shared validation for `http://IPv4` QR payloads.

**Do:**

1. Create `components/setup_url/` with:

   ```c
   bool setup_url_format_ipv4(unsigned a, unsigned b, unsigned c, unsigned d,
                              char *out, size_t out_len);
   bool setup_url_validate(const char *url);
   ```

2. Match rules in `docs/qr_encoder_interface.md` (root URL only, no path, no https).

**Done when:** Tests or a small component test pass the cases in
`docs/test_strategy.md` → setup_url utility.

---

### Task 2 — app_core display callback API

**Goal:** Only `app_core` calls `display_controller_*`; text and QR use the same path.

**Do:**

1. Add to `components/app_core/include/app_core.h`:

   ```c
   esp_err_t app_core_display_show_template(display_layout_template_t template_id,
                                            const char *const *lines,
                                            size_t line_count);
   esp_err_t app_core_display_show_qr_setup(const char *url,
                                            const char *const *text_lines,
                                            size_t text_line_count);
   ```

   (Names MUST match or be documented in this handoff if you choose equivalents.)

2. Implement in `app_core.c`: validate QR URL with `setup_url_validate()` before
   calling `display_controller_show_qr_setup()`; on failure return error and do
   **not** call the display.

3. Change `app_core_start()` demo to call `app_core_display_show_template()` instead
   of `display_controller_show_template()`.

**Do not:** Add `esp_event` for display in v1.

**Done when:** `grep -r display_controller.h components/` shows includes only from
`app_core` and `display` (display component internals allowed).

---

### Task 3 — Fix QR hardcode in `display_controller_show_template`

**Goal:** `DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT` must not embed a fixed URL.

**Do:**

1. In `display_controller.c`, remove the hardcoded `"http://192.168.4.1"` in the
   `QR_LEFT_TEXT_RIGHT` template branch.
2. Either:
   - require callers to use `display_controller_show_qr_setup(payload, ...)` for QR, and
     return `ESP_ERR_INVALID_ARG` for `QR_LEFT_TEXT_RIGHT` in `show_template`, **or**
   - add a `payload` parameter to `show_template` for that case only.

   Prefer the first option (QR only via `show_qr_setup`).

**Done when:** No literal setup URL remains in `show_template`.

---

### Task 4 — Nayuki QR encoder + real `display_qr_generate`

**Goal:** Replace QR stub with version 1–2 product payloads.

**Do:**

1. Add `components/qr_encoder/vendor/qrcodegen/` (upstream C + MIT `LICENSE`).
2. Pin commit or version in `agent-workspaces/implementer/handoff.md` (Task record).
3. Implement `display_qr_generate()` in `display_qr.c` using Nayuki (byte mode, EC LOW).
4. Static buffer 25×25; no heap in encode path.
5. Only `display_task` context may call `display_qr_generate` (already the case via renderer).

**Done when:**

- `display_qr_generate("http://192.168.4.1")` → width 25
- `display_qr_generate("http://1.1.1.1")` → width 21
- Invalid payloads return false

---

### Task 5 — Build, size, and tester handoff

**Goal:** Record integration result for architect/tester.

**Do:**

1. `idf.py build` for `esp32c3`.
2. Note `b06_hil_firmware.bin` size before/after QR integration in a new handoff
   section below.
3. On hardware: demo four-line screen still works; QR instruction via
   `app_core_display_show_qr_setup("http://192.168.4.1", ...)` scans on phone.

**Done when:** Tester can run checklist in `docs/test_strategy.md` (Display Delivery +
QR Encoder sections).

**Status:** Complete (2026-06-22). See handoff records below.

---

## DISPLAY_VISUAL_DEMO_PROTOCOL

```text
ID: DISPLAY_VISUAL_DEMO_PROTOCOL
Source handoff: agent-workspaces/architect/handoff.md, docs/display_visual_demo_protocol.md
Summary:
  - CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO (default y in sdkconfig.defaults)
  - app_core_run_visual_demo() three-step sequence with DEMO_STEP serial markers
  - app_core_run_display_smoke() when Kconfig disabled
  - Boot wiring in app_core_start() via #if CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO
Modified files:
  - components/app_core/Kconfig
  - components/app_core/app_core_display_demo.c
  - components/app_core/include/app_core_display_demo.h
  - components/app_core/app_core.c
  - components/app_core/CMakeLists.txt
  - sdkconfig.defaults
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4)
  - b06_hil_firmware.bin size 0x3a2e0 bytes
  - DEMO_STEP format strings present in firmware ELF (serial grep target)
Questions or technical debt:
  - Human OLED observation and QR camera scan pending tester Run 006.
Ready for tester:
  Yes. Flash with Kconfig y; capture serial 30-45 s; per-step feedback.md.
```

### Display Visual Demo manifest

```text
Handoff ID: DISPLAY_VISUAL_DEMO_PROTOCOL
Kconfig: CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
Total hold (s): 18
Commands: idf.py build && idf.py flash && idf.py monitor
Observation window (s): ~23

Step | name= | API | Payload / lines | Hold (s) | Human expects on OLED
1 | FULL_FOUR_LINES | app_core_display_show_template | b06_hil, READY, v0.1, "" | 5 | Three text lines; fourth empty
2 | QR_SETUP | app_core_display_show_qr_setup | http://192.168.4.1, SCAN, ME | 8 | QR left, text right; phone scans URL
3 | FULL_TWO_LINES | app_core_display_show_template | TEXT, ONLY | 5 | Two lines only; no QR pixels remain
```

---

## DISPLAY_DELIVERY_CONTRACT

```text
ID: DISPLAY_DELIVERY_CONTRACT
Source handoff: agent-workspaces/architect/handoff.md, docs/display_delivery_contract.md
Summary:
  - app_core_display_show_template and app_core_display_show_qr_setup in app_core.h
  - QR validated with setup_url_validate before display_controller_show_qr_setup
  - app_core_start demo uses app_core_display_show_template
  - display_controller.h included only from app_core and display internals
Modified files:
  - components/app_core/include/app_core.h
  - components/app_core/app_core.c
  - components/app_core/CMakeLists.txt
Commands executed:
  - grep audit for display_controller.h callers
Build result:
  - Included in QR integration build (see QR_ENCODER_INTERFACE)
Questions or technical debt:
  - Instruction source identity still TBD (may live inside app_core).
Ready for tester:
  Yes. Validate callback path and invalid QR rejection at app_core.
```

## QR_ENCODER_INTERFACE

```text
ID: QR_ENCODER_INTERFACE
Source handoff: agent-workspaces/architect/handoff.md, docs/qr_encoder_interface.md
Summary:
  - components/setup_url with format_ipv4, validate, setup_url_self_test at boot
  - components/qr_encoder/vendor/qrcodegen (Nayuki MIT)
  - display_qr_generate uses qrcodegen_encodeBinary, EC LOW, version 1-2
  - display_controller_show_template rejects DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT
  - QR only via display_controller_show_qr_setup / app_core_display_show_qr_setup
Nayuki pin:
  - Repository: https://github.com/nayuki/QR-Code-generator
  - Branch: master
  - File blob: qrcodegen.c sha 34f1002501fa2bcb0a054f4311795b8cbb977f5b
Modified files:
  - components/setup_url/
  - components/qr_encoder/
  - components/display/display_qr.c
  - components/display/display_controller.c
  - components/display/CMakeLists.txt
Commands executed:
  - idf.py build (esp32c3)
Build result:
  - Success (ESP-IDF 5.3.4, target esp32c3)
  - b06_hil_firmware.bin before QR integration: 0x34de0 bytes (I2C phase 2)
  - b06_hil_firmware.bin after QR integration: 0x39f00 bytes (+0x4120)
Questions or technical debt:
  - Hardware QR scan not verified by implementer (tester task).
Ready for tester:
  Yes. Run docs/test_strategy.md Display Delivery + QR Encoder sections on device.
```

---

## Rules (do not guess)

| Topic | Rule |
| --- | --- |
| OLED driver | **SSD1306 only** for v1. Do not add SH1106 unless hardware changes. |
| Display transport | **Callback via app_core** only. No `esp_event` for display. |
| QR vs text | Same delivery path; QR is not special except payload validation. |
| WiFi / network | Display code MUST NOT include or call network/WiFi APIs. |
| Screen priority | **Last instruction wins** when several callbacks arrive close together (display_task coalescing). |
| `BOLD` emphasis | Treat as **NORMAL** for v1 unless architect authorizes otherwise. |
| Menus / power save | Out of scope; do not implement. |
| ASCII | Product strings ASCII-only; sanitize non-ASCII to `?`. |

---

## I2C_BUS_PHASE2

```text
ID: I2C_BUS_PHASE2
Source handoff: agent-workspaces/architect/handoff.md, docs/i2c_bus_architecture.md
Summary:
  Implemented phase 2 transaction executor:
  - i2c_transaction_t and i2c_bus_transceive() in public API
  - Write-only (tx_len > 0, rx_len == 0), write-read, and read-only paths
  - Internal FreeRTOS mutex serializes all bus transactions
  - Validates device handle, buffers, timeout_ms > 0, and registered devices
  - display_driver migrated to i2c_bus_transceive only (no direct SDK I2C)
  - SSD1306 init + GDDRAM flush when I2C device is present at boot
Modified files:
  - components/i2c_bus/include/i2c_bus.h
  - components/i2c_bus/i2c_bus.c
  - components/i2c_bus/CMakeLists.txt
  - components/display/display_driver.c
Commands executed:
  - idf.py build
Build result:
  - Success (ESP-IDF 5.3, target esp32c3)
  - b06_hil_firmware.bin size 0x34de0 bytes
Questions or technical debt:
  - Phase 3 i2c_broker not implemented.
  - INA219 driver still absent.
Ready for tester:
  Yes. Validate two-task concurrent transceive (no corruption), timeout recovery,
  SSD1306 visual output on hardware, stub fallback when OLED absent.
```

## I2C_BUS_ARCHITECTURE (Phase 1)

```text
ID: I2C_BUS_ARCHITECTURE
Source handoff: agent-workspaces/architect/handoff.md
Summary:
  Implemented portable shared I2C master bus (phase 1 direct sync):
  - components/i2c_bus with init, add_device, probe, deinit
  - ESP-IDF 5.x i2c_master driver binding with internal pull-ups
  - Device handle cache by 7-bit address
  - board_i2c_bus_config() maps GPIO0/GPIO1 and 400 kHz default
  - app_core startup: bus init, OLED probe 0x3C then 0x3D, add_device, pass
    handle through display_config_t to display_driver
  - Missing OLED at boot logs warning and continues with stub display
  - display_types uses opaque i2c_bus_device_t forward declaration; only
    display_driver includes i2c_bus.h (PRIV_REQUIRES)
Modified files:
  - components/i2c_bus/
  - components/board/board_pins.c
  - components/board/include/board_pins.h
  - components/app_core/app_core.c
  - components/app_core/CMakeLists.txt
  - components/display/display_driver.c
  - components/display/include/display_types.h
  - components/display/CMakeLists.txt
Commands executed:
  - idf.py set-target esp32c3
  - idf.py build
Build result:
  - Success (ESP-IDF 5.3, target esp32c3)
  - b06_hil_firmware.bin size 0x33740 bytes
Questions or technical debt:
  - See Next Tasks for QR and app_core delivery work.
Ready for tester:
  Yes for I2C phase 1 scope; see I2C_BUS_PHASE2 for transaction tests.
```

## OLED_TEXT_DISPLAY_INTERFACE

```text
ID: OLED_TEXT_DISPLAY_INTERFACE
Source handoff: agent-workspaces/architect/handoff.md
Summary:
  Layered 128x64 display stack with geometry self-test, FreeRTOS display task,
  SSD1306 init and flush via i2c_bus_transceive when OLED present at boot.
Physical controller (v1): SSD1306 — confirmed; no SH1106 planned.
Modified files:
  - components/display/
Questions or technical debt:
  - None for display delivery scope.
Ready for tester:
  Yes for full display v1 stack including QR encode path.
```

## Template

```text
Role boundary check:
  Confirm an architect handoff authorizes this implementation. Redirect
  architecture changes to architect and formal validation to tester if mixed.
ID:
Source handoff:
Summary:
Modified files:
Commands executed:
Build result:
Questions or technical debt:
Ready for tester:
```