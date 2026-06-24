# Architect Handoff

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting any task in this file, run the **Role Boundary Check**:

- Read `ROLE.md` in this folder, `../../AGENTS.md`, and `../../docs/methodology.md`.
- Confirm the request is **architecture and documentation**, not firmware
  implementation, builds, or formal validation.
- If the task includes code, tests, or flash steps, stop and tell the human that
  the implementer or tester must execute those parts unless explicitly
  redirected.

## Current Status

Active handoffs:

- `OLED_TEXT_DISPLAY_INTERFACE`
- `I2C_BUS_ARCHITECTURE`
- `I2C_BUS_CONCURRENCY`
- `QR_ENCODER_INTERFACE`
- `DISPLAY_DELIVERY_CONTRACT`
- `WIFI_PROVISIONING_ARCHITECTURE`
- `WIFI_PROVISIONING_PORTAL_REACHABILITY`
- `WIFI_PROVISIONING_HTTP_STACK_SAFETY`
- `WIFI_PROVISIONING_HTTP_HEADER_BUDGET`
- `WIFI_PROVISIONING_POST_SUCCESS_COMMIT`
- `WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY`
- `PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID`
- `OLED_PROVISIONING_SETUP_UX`
- `OLED_WIFI_CONNECTED_STATUS`

Pending architect review: none.

I2C pins, OLED address candidates, `INA219` addresses, error LED polarity, and
factory reset switch polarity are recorded in
`docs/esp32_c3_supermini_connections.md`. Generic shared I2C bus structure and
incremental concurrency phases are recorded in
`docs/i2c_bus_architecture.md`. QR encoder rules and `http://IPv4` payload
profile are recorded in `docs/qr_encoder_interface.md`. Display delivery from
producers to the OLED stack is recorded in
`docs/display_delivery_contract.md`. WiFi provisioning AP, HTTP portal, NVS
credential storage, and factory-reset behavior are recorded in
`docs/wifi_provisioning_architecture.md`. The implementer must keep WiFi
provisioning display requests routed through `app_core` and must not couple
display internals to WiFi/network headers.

## TESTER_RETURN_SAVED_BOOT_RELIABILITY

```text
Role boundary check:
  Tester evidence and recommendations only. Architect decides criteria and whether
  to authorize a new implementer handoff. Tester does not change architecture.
ID: TESTER_RETURN_SAVED_BOOT_RELIABILITY
Date: 2026-06-24
Source:
  - agent-workspaces/tester/test_runs.md Run 013
  - agent-workspaces/tester/feedback.md Entries 011, 012, 013
  - agent-workspaces/tester/handoff.md Architect review block
Firmware: b06_hil_firmware.bin 0xdcb20 (WIFI_PROVISIONING_POST_SUCCESS_COMMIT)
Operator network: SSID vitriolina, WPA3-SAE, IP 192.168.80.184 when connected

What PASSES:
  - POST /provision: browser "WiFi connected. You can close this page."
  - AP b06_hil_setup stops after success; OLED WIFI OK on submit.
  - NVS credentials saved; ping to assigned IP works when boot connects.
  - Run 012 split-brain regression (blank page / early OLED OK) resolved.

What FAILS or is QUALIFIED:
  - Saved-credential cold boot reliability: 3/5 reboots fail, 2/5 pass (same NVS).
  - Failure: auth -> init, wifi_prov STA disconnected reason=202 (AUTH_FAIL) ~1 s.
  - OLED: WIFI CONNECTING → after ~30 s WIFI FAILED (locked path per architecture).
  - test_strategy criterion "reboot with saved valid credentials connects STA"
    is not met reliably on this bench.

Root cause analysis (code + serial, not bad credentials):
  1. POST uses WIFI_MODE_APSTA with WiFi already running; saved boot uses STA-only
     cold esp_wifi_start() — different connect context.
  2. wait_for_sta_connected() calls esp_wifi_connect() once, waits 30 s for
     IP_EVENT_STA_GOT_IP; no application retry loop before SAVED_FAILURE_LOCKED.
  3. apply_sta_config sets threshold.authmode = WIFI_AUTH_WPA2_PSK; successful
     connects negotiate WPA3-SAE with operator router — intermittent first-auth
     fail on cold boot is plausible.
  4. After timeout, s_pending_sta_source cleared; late GOT_IP ignored by handler.

Recommendations for architect:
  A. Acceptance criteria: require N consecutive cold-boot successes? Define N.
  B. Supported routers: WPA2-only vs WPA3-SAE / transition mode for v1.
  C. Saved-boot policy: retry N times with backoff before locked FAILED?
  D. Implementation (if authorized): authmode/PMF, post-start delay, explicit
     reconnect policy, document interaction with ESP-IDF auto-reconnect.
  E. Do not weaken POST success commit; address saved-boot path separately.

Open questions:
  - Is intermittent saved-boot failure acceptable in v1 if POST works?
  - Should architecture mandate retry before HOLD RESET locked state?

Evidence files:
  /tmp/b06_hil_reboot_sweep_run013.txt
  /tmp/b06_hil_boot_log_run013_post_human.txt
  /tmp/b06_hil_boot_log_run013_reboot.txt

Architect decision:
  - Intermittent saved-credential boot failure is not acceptable in v1.
  - v1 must support WPA2-Personal, WPA2/WPA3 transition, and WPA3-SAE Personal.
  - A new implementer handoff is authorized:
    WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY.
```

## WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY

```text
Role boundary check:
  This handoff is architect-owned corrective architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
Source evidence:
  - tester/feedback.md Entry 013
  - tester/test_runs.md Run 013
  - tester/handoff.md Architect review block
  - architect/handoff.md TESTER_RETURN_SAVED_BOOT_RELIABILITY
Objective:
  Make saved-credential cold boot reliable after successful provisioning. The same
  known-good NVS credentials must connect consistently across repeated STA-only
  boots, including on WPA2/WPA3/WPA3-SAE Personal routers.
Reason:
  Run 013 POST success path passed: browser success, AP teardown, OLED OK, and ping
  worked when connected. Saved-credential cold boot was intermittent: 2/5 boots got
  IP, 3/5 failed early with `WIFI_REASON_AUTH_FAIL` reason 202. Credentials and RF
  signal are not the root cause because the same saved credentials connect on
  successful boots and after POST provisioning.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/wifi_provisioning/wifi_provisioning.c
  - components/wifi_provisioning/include/wifi_provisioning.h only if event details
    need extension
  - components/app_core/app_core_wifi.c only if saved-boot retry states need clearer
    neutral event display mapping
  - agent-workspaces/implementer/handoff.md for implementation notes
  - agent-workspaces/implementer/backlog.md for task status
Expected changes:
  - Use the same STA config helper for submitted and saved attempts.
  - Configure STA for WPA2/WPA3/WPA3-SAE Personal compatibility on ESP-IDF 5.3:
    PMF capable true, PMF required false for v1 transition compatibility, and SAE
    PWE compatibility mode where available.
  - Log STA auth compatibility once per config application.
  - Add cold-start settle delay of 1000 ms after `esp_wifi_start()` before saved
    boot attempt 1.
  - Replace single saved `esp_wifi_connect()` with application retry policy:
    max_attempts=5, per_attempt_timeout_ms=12000, backoff_ms=[0,1000,3000,5000,8000].
  - End an attempt promptly on early disconnect reason; do not wait full timeout
    after reason 202.
  - Emit `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED` only after all saved attempts fail.
  - Preserve POST success commit, AP teardown, stack safety, and header-budget fixes.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md Saved Boot Reliability Contract.
  - Preserve neutral event and display boundaries.
Detailed behavior:
  - Required logs:
    - `STA config auth_profile=wpa2_wpa3_personal pmf_capable=true pmf_required=false`
    - `saved boot connect policy attempts=5 per_attempt_timeout_ms=12000`
    - `saved boot attempt <n>/5 start`
    - `STA disconnected reason=<reason> source=saved attempt=<n>/5`
    - `saved boot attempt <n>/5 failed reason=<reason> backoff_ms=<ms>`
    - `STA got IP ip=<ipv4> source=saved attempt=<n>/5`
    - `saved boot connected attempt=<n>/5 ip=<ipv4>`
    - `saved boot failed attempts=5 last_reason=<reason>`
  - If ESP-IDF auto-reconnect/failure_retry_cnt is used, it must stay bounded by
    the application policy and must not hide attempt boundaries.
Non-goals:
  - Automatic AP fallback after saved-credential failure.
  - Erasing credentials after saved-credential failure.
  - Enterprise WiFi, captive upstream portals, HTTPS, or AP password changes.
Acceptance criteria:
  - 10 consecutive saved-credential cold boots pass after successful provisioning.
  - 0/10 boots end in locked failure.
  - Reason 202 may appear only if a later attempt within the same boot recovers and
    logs saved GOT_IP before policy exhaustion.
  - At least one accepted sweep is against WPA2/WPA3 transition or WPA3-SAE
    Personal, matching the current bench router profile.
  - Static audits from WiFi Provisioning Criteria remain PASS.
Constraints:
  - Do not log passwords.
  - Do not weaken POST success commit or reopen AP automatically on saved failure.
Validation plan:
  - Implementer documents auth profile config, retry constants, build result, and
    any local retry evidence.
  - Tester provisions once, confirms POST success, then runs 10 cold reboots with
    serial capture summary per boot.
Open questions:
  - None.
```

## PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID

```text
Role boundary check:
  This handoff is architect-owned product architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
Source:
  - Human request 2026-06-23: remove display demo calls and generate AP SSID as
    HIL-06-<last4mac>, with `06` dynamic from the project folder.
Objective:
  Remove historical display demo/test screens from normal product boot and replace
  the fixed provisioning AP SSID with a deterministic board/MAC-derived name.
Reason:
  Display demo content (`QR_SETUP`, `TEXT ONLY`, `DEMO_STEP`, smoke screens) was
  useful to validate the OLED stack but is not product UX. The provisioning AP name
  must identify the HIL board family, board number, and physical device instance
  without being hard-coded to `b06_hil_setup`.
Authorized files:
  - docs/display_visual_demo_protocol.md
  - docs/wifi_provisioning_architecture.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/app_core/CMakeLists.txt
  - components/app_core/Kconfig
  - components/app_core/app_core.c
  - components/app_core/app_core_display_demo.c
  - components/app_core/include/app_core_display_demo.h
  - firmware/sdkconfig.defaults
  - components/board/include/board_pins.h or equivalent board-owned config header
  - components/wifi_provisioning/CMakeLists.txt
  - components/wifi_provisioning/wifi_provisioning.c
  - agent-workspaces/implementer/handoff.md for implementation notes
  - agent-workspaces/implementer/backlog.md for task status
Expected changes:
  Display demo retirement:
    - Remove boot/runtime calls to `app_core_run_visual_demo()` and
      `app_core_run_display_smoke()`.
    - Remove `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` from `sdkconfig.defaults`.
    - Remove demo source/header/Kconfig/CMake references if no longer used.
    - Preserve real product display APIs:
      `app_core_display_show_template()` and `app_core_display_show_qr_setup()`.
    - Product boot must not emit `DEMO_STEP` logs or show demo-only `SCAN/ME` or
      `TEXT/ONLY` content.
  Dynamic AP SSID:
    - Replace fixed `b06_hil_setup` with runtime-generated
      `HIL-<board_number_2_digits>-<last_4_softap_mac_hex>`.
    - For this tree, the board number is `06`, derived from project identifier
      `b06_hil`.
    - The board component owns the board number constant/helper; `wifi_provisioning`
      must not hard-code `06` or parse filesystem paths at runtime.
    - Use the ESP32 WiFi SoftAP MAC address; suffix is the last two MAC bytes as
      uppercase hex with no separators, e.g. `HIL-06-ABCD`.
    - Log the generated SSID at SoftAP startup.
Module boundaries and contracts:
  - Board identity belongs to `components/board`.
  - WiFi provisioning may consume board identity but must continue to avoid display
    and app_core dependencies.
  - Display demo removal must not remove WiFi provisioning QR/status display paths.
Non-goals:
  - Changing portal URL (`http://192.168.4.1/` remains).
  - Adding AP password, captive DNS, HTTPS, or web UI redesign.
  - Removing QR generation used by the real WiFi provisioning setup screen.
Acceptance criteria:
  - Clean build succeeds.
  - No `DEMO_STEP` logs during normal product boot.
  - No product boot call path invokes `app_core_run_visual_demo` or
    `app_core_run_display_smoke`.
  - `sdkconfig.defaults` does not contain `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO`.
  - Fresh provisioning AP SSID matches `HIL-06-[0-9A-F]{4}`.
  - Serial log shows generated SoftAP SSID and no `b06_hil_setup` product string.
  - WiFi provisioning QR/status/success/failure display behavior still works from
    real provisioning events.
Validation plan:
  - Implementer documents removed files/references and generated SSID example.
  - Tester boots fresh-NVS firmware, confirms no demo logs/screens, confirms AP
    name pattern, and confirms portal remains reachable at `http://192.168.4.1/`.
Open questions:
  - None.
```

## OLED_PROVISIONING_SETUP_UX

```text
Role boundary check:
  This handoff is architect-owned product UX architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: OLED_PROVISIONING_SETUP_UX
Source:
  - Human request 2026-06-23: when no WiFi credentials are saved, keep the
    setup QR (http://192.168.4.1) on the OLED left and show the provisioning AP
    SSID plus short numbered instructions on the right.
  - Extends PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID (dynamic SSID already
    generated; not yet shown on OLED).
Objective:
  Replace the generic WIFI/SETUP right-panel text with a four-line provisioning
  guide that shows the full SoftAP SSID (split across two lines) and tells the
  user to join that network then scan the QR.
Reason:
  Users could not identify which AP to join because the OLED did not show the
  HIL-06-XXXX SSID. Generic WIFI/SETUP does not communicate the required
  two-step flow (join AP, then scan portal QR).
Authorized files (implementer only):
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Normative specs (already updated by architect):
  - docs/wifi_provisioning_architecture.md — Display Integration
  - docs/display_delivery_contract.md — Product provisioning QR setup
  - docs/oled_text_display_interface.md — Golden 6
Expected changes (implementer):
  wifi_provisioning.c:
    - In wifi_provisioning_start_ap_portal(), change emit_event for
      WIFI_PROV_EVENT_AP_STARTED and WIFI_PROV_EVENT_PORTAL_READY to pass
      s_ap_ssid as the third argument instead of NULL.
    - No new public APIs. No display includes.
  app_core_wifi.c:
    - Add static helper (name implementer choice, e.g.
      show_provisioning_setup_display) that:
        1. Reads setup_url from event (fallback "http://192.168.4.1").
        2. If info->ssid is NULL or strlen < 8: show_qr_setup(url, WIFI/SETUP, 2).
        3. Else build 4 lines per docs/display_delivery_contract.md table.
        4. Use #define PROV_SETUP_LINE_JOIN "1 JOIN",
           PROV_SETUP_LINE_SCAN_QR "2 SCAN QR", PROV_SETUP_SSID_PREFIX_LEN 7.
        5. Copy prefix with strncpy(ssid, 7); suffix with strncpy(ssid+7, ...).
    - Call helper from on_wifi_prov_event for AP_STARTED and PORTAL_READY.
    - Do not change other event display mappings.
Module boundaries:
  - wifi_provisioning emits neutral events only; app_core owns all display calls.
  - No changes to display stack, setup_url, or public app_core.h APIs.
Non-goals:
  - Spanish OLED strings (v1 English only).
  - WiFi join QR on the right panel.
  - OPEN / no-password indicator line.
  - New layout template or font changes.
  - Portal HTML or captive DNS changes.
Acceptance criteria:
  1. Fresh NVS boot (no credentials): OLED shows QR http://192.168.4.1 left;
     right shows 1 JOIN / HIL-06- / XXXX / 2 SCAN QR where XXXX is the 4-char
     MAC suffix of the real SoftAP SSID.
  2. Lines 1+2 concatenated match the SSID in the phone WiFi list exactly.
  3. No pixel truncation on lines for SSID format HIL-NN-MAC4 (11 chars total).
  4. Saved-credentials boot path unchanged (no provisioning setup screen).
  5. Fallback WIFI/SETUP only when ssid missing (error path).
  6. Clean build; wifi_provisioning still has no app_core or display includes.
Validation plan:
  - Implementer: build log + note example SSID from serial.
  - Tester: erase NVS, boot, photograph OLED, join displayed AP, scan QR, confirm
    portal loads at http://192.168.4.1/.
Open questions:
  - None.
```

## OLED_WIFI_CONNECTED_STATUS

```text
Role boundary check:
  Architect-owned product UX. Firmware code, build, flash, and validation belong
  to implementer/tester.
ID: OLED_WIFI_CONNECTED_STATUS
Source:
  - Human request 2026-06-23: when WiFi connection is OK, show current STA IPv4
    and STA MAC on the OLED, distributed for human readability within 128×64.
  - Replaces legacy SUBMITTED_SUCCESS screen (WIFI / OK two lines) and adds
    display for SAVED_SUCCESS (currently no-op in app_core_wifi.c).
Objective:
  On successful STA connection, show a four-line FULL_FOUR_LINES screen: status,
  IP address, and MAC split into two XX:XX:XX groups.
Reason:
  WIFI / OK gives no network identity. Operators need the assigned IP to reach the
  device and the MAC for bench identification. A single 17-char MAC line risks
  truncation; IP (up to 15 chars) fits alone on one full-width line.
Authorized files (implementer only):
  - components/wifi_provisioning/include/wifi_provisioning.h
  - components/wifi_provisioning/wifi_provisioning.c
  - components/app_core/app_core_wifi.c
Normative specs (architect-updated):
  - docs/wifi_provisioning_architecture.md — WiFi connected OLED screen
  - docs/display_delivery_contract.md — Product WiFi connected screen
  - docs/oled_text_display_interface.md — Golden 7
Expected changes (implementer):
  wifi_provisioning.h:
    - Add sta_ipv4 and sta_mac to wifi_prov_event_info_t (const char * each).
  wifi_provisioning.c:
    - Add static s_sta_ipv4[16], s_event_sta_ipv4[16], s_event_sta_mac[18].
    - On IP_EVENT_STA_GOT_IP for active pending STA source, copy IP to s_sta_ipv4.
    - Before SUBMITTED_SUCCESS and SAVED_SUCCESS emit: format STA MAC with
      esp_wifi_get_mac(WIFI_IF_STA) as "%02X:%02X:%02X:%02X:%02X:%02X".
    - Copy s_sta_ipv4 → s_event_sta_ipv4 and MAC → s_event_sta_mac; pass pointers
      in wifi_prov_event_info_t. Other events: sta_ipv4/sta_mac MUST be NULL.
    - Extend emit_event (or emit_event_ex) to populate new fields.
  app_core_wifi.c:
    - Add show_wifi_connected_display() per architecture doc algorithm.
    - SUBMITTED_SUCCESS → show_wifi_connected_display (not WIFI/OK template).
    - SAVED_SUCCESS → show_wifi_connected_display (not empty break).
    - Fallback FULL_TWO_LINES WIFI/OK if sta_ipv4 or sta_mac missing/invalid.
Exact OLED content (success path):
  Line 0: WIFI OK
  Line 1: <sta_ipv4> e.g. 192.168.1.42
  Line 2: <sta_mac[0..7]> e.g. AA:BB:CC
  Line 3: <sta_mac[9..16]> e.g. DD:EE:FF
Constants (app_core_wifi.c):
  WIFI_OK_LINE_STATUS "WIFI OK"
  WIFI_OK_MAC_PREFIX_LEN 8
  WIFI_OK_MAC_SUFFIX_OFFSET 9
Module boundaries:
  - app_core MUST NOT call esp_netif_* or esp_wifi_* for this screen.
  - wifi_provisioning MUST NOT include display or app_core headers.
Non-goals:
  - QR on connected screen, scrolling, timeout/auto-clear, IPv6, AP MAC display.
Acceptance criteria:
  1. POST provision success: OLED shows WIFI OK + real STA IP + split STA MAC.
  2. Saved-credentials boot success: same screen (SAVED_SUCCESS path).
  3. MAC lines reassemble to uppercase AA:BB:CC:DD:EE:FF matching esp_wifi STA MAC.
  4. IP matches serial log STA got IP / saved boot connected ip= line.
  5. Fallback WIFI/OK only when success event lacks sta_ipv4 or sta_mac.
  6. CONNECTING / FAILED / provisioning QR screens unchanged.
  7. Clean build; no display coupling violations in wifi_provisioning.
Validation plan:
  - Implementer: build + serial log with example IP/MAC.
  - Tester: fresh provision + saved reboot; photograph OLED; compare IP/MAC to router.
Open questions:
  - None.
```

## WIFI_PROVISIONING_PORTAL_REACHABILITY

```text
Role boundary check:
  This handoff is architect-owned corrective architecture. Firmware code, build,
  flash, and hardware validation belong to implementer/tester.
ID: WIFI_PROVISIONING_PORTAL_REACHABILITY
Source evidence:
  - tester/handoff.md Run 007, Run 008, Run 009
  - tester/test_runs.md Run 007, Run 008, Run 009
  - tester/feedback.md Entry 003, Entry 004, Entry 005
Objective:
  Correct the WiFi provisioning portal startup architecture so a phone associated
  to `b06_hil_setup` can reach HTTP handlers deterministically, and so startup
  failures do not cause reboot loops.
Reason:
  Run 007 associated a phone but `GET /` never reached the handler. Run 008
  introduced an AP start event race and reboot loop. Run 009 fixed the race but
  still showed 0x `GET / from client` despite SoftAP, HTTP server, and portal ready
  logs. The architecture must now prescribe ESP-IDF AP netif, DHCP, event-bit,
  HTTP, and diagnostics order tightly enough that implementation cannot guess.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/wifi_provisioning/wifi_provisioning.c
  - components/wifi_provisioning/include/wifi_provisioning.h only if diagnostics
    or neutral event types need extension
  - components/app_core/app_core_wifi.c only to remove abort-on-provisioning-error
    behavior or map new neutral events
  - agent-workspaces/implementer/handoff.md for implementation notes
Expected changes:
  - Configure AP netif IP/DHCP before `esp_wifi_start()`.
  - Do not stop or restart DHCP after `WIFI_EVENT_AP_START`.
  - Clear `WIFI_AP_STARTED_BIT` before `esp_wifi_start()`, never after.
  - Keep startup in `WIFI_MODE_AP`; switch to APSTA only during submitted-credential
    connection attempts.
  - Start HTTP server only after AP start succeeds.
  - Register `GET /`, `GET /health`, `POST /provision`, and 404 handlers.
  - Add stable diagnostics for AP IP, SoftAP start, station join, HTTP start,
    handler registration, portal ready, `GET /health`, `GET /`, and `POST /provision`.
  - Remove any `ESP_ERROR_CHECK` path that turns provisioning startup errors into
    reboot loops.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md sections Corrective Architecture
    From Runs 007-009 and ESP-IDF SoftAP Portal Startup Contract.
  - Preserve neutral event boundary: wifi_provisioning still must not include
    app_core.h or display headers.
Detailed behavior:
  - `GET /health` returns `OK` as `text/plain; charset=utf-8` with
    `Cache-Control: no-store`.
  - Portal ready is logged only after AP IP/DHCP setup, AP start, HTTP start, and
    handler registration have all succeeded.
  - A phone joined to AP must produce `station joined`, then `/health` and `/`
    handler logs when those URLs are opened.
  - If `/health` does not log, the bug is AP/DHCP/routing/httpd reachability, not
    HTML rendering.
Non-goals:
  - Captive DNS, HTTPS, WPA2 AP password, mobile-app-specific behavior, web reset,
    or automatic AP fallback for saved credential failure.
Acceptance criteria:
  - Run 008 reboot loop cannot recur: no AP start timeout after `SoftAP started`
    due to event-bit clearing after start.
  - Run 009 blank page cannot recur: tester can load `http://192.168.4.1/health`
    and `http://192.168.4.1/`, and serial logs show handler entry for both.
  - Reboot loop cannot occur from AP/HTTP provisioning errors.
  - Static audits from WiFi Provisioning Criteria remain PASS.
Constraints:
  - Do not widen dependencies between WiFi provisioning and display/app_core.
  - Do not log passwords.
Validation plan:
  - Implementer documents exact AP/DHCP/HTTP startup log sequence and build result.
  - Tester repeats fresh-NVS phone test with cellular disabled if possible, records
    phone IP, `/health` result, `/` form result, and handler logs.
Open questions:
  - None.
```

## WIFI_PROVISIONING_HTTP_STACK_SAFETY

```text
Role boundary check:
  This handoff is architect-owned corrective architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: WIFI_PROVISIONING_HTTP_STACK_SAFETY
Source evidence:
  - tester/handoff.md Run 010
  - tester/test_runs.md Run 010
  - tester/feedback.md Entry 006
Objective:
  Ensure the WiFi provisioning HTTP handlers can serve `/health`, `/`, and
  `/provision` responses without stack protection faults, Guru Meditation, abort,
  or reboot.
Reason:
  Run 010 fixed AP/DHCP/httpd reachability: phone received 192.168.4.2 and
  `GET / from client` reached the handler. The device then crashed in the httpd
  worker with stack protection fault before HTML reached the browser. Code review
  shows setup/failure page helpers allocate large `char html[2048]` buffers on the
  HTTP worker stack, matching the tester's hypothesis.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/wifi_provisioning/wifi_provisioning_pages.c
  - components/wifi_provisioning/wifi_provisioning_pages.h only if private helper
    signatures change
  - components/wifi_provisioning/wifi_provisioning_form.c
  - components/wifi_provisioning/wifi_provisioning.c only for httpd stack config
    and response-completion diagnostics
  - agent-workspaces/implementer/handoff.md for implementation notes
Expected changes:
  - Remove large HTML response buffers from HTTP handler stack.
  - Remove full-body duplicate `pair` buffers from parser stack, or otherwise keep
    every local array in handler call chains <= 128 bytes.
  - Send setup/failure pages by constant chunks, bounded static/heap buffers, or
    another non-stack bounded strategy.
  - Set `httpd_config_t.stack_size` explicitly to at least 8192 bytes unless code
    review proves smaller is safe.
  - Add response-completion logs for `/health`, `/`, and `/provision` success/fail
    pages.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md HTTP Handler Stack Safety.
  - Preserve neutral event and display boundaries.
Detailed behavior:
  - `GET /health` logs `GET /health from client` and `GET /health response sent`.
  - `GET /` logs `GET / from client` and `GET / response sent`.
  - If a response send fails, log the route and esp_err_t.
  - Passwords are never logged or echoed.
Non-goals:
  - Captive DNS, JavaScript frontend, filesystem assets, HTTPS, AP password, or
    changing the portal UI semantics.
Acceptance criteria:
  - Run 010 stack fault cannot recur: `/` and failure page rendering use no large
    stack buffers.
  - Phone loads `/health` and `/` without reboot; serial logs response-completion
    markers for both.
  - Wrong submitted credentials return the failure page without stack fault.
  - Static audits from WiFi Provisioning Criteria remain PASS.
Constraints:
  - Do not widen dependencies between WiFi provisioning and app_core/display.
  - Do not log passwords.
Validation plan:
  - Implementer documents page rendering memory strategy, httpd stack size, and
    build result.
  - Tester repeats fresh-NVS phone test: `/health`, `/`, bad POST, and serial
    response-completion logs.
Open questions:
  - None.
```

## WIFI_PROVISIONING_HTTP_HEADER_BUDGET

```text
Role boundary check:
  This handoff is architect-owned corrective architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: WIFI_PROVISIONING_HTTP_HEADER_BUDGET
Source evidence:
  - tester/handoff.md Run 011
  - tester/test_runs.md Run 011
  - tester/feedback.md Entry 007
Objective:
  Ensure a normal phone browser can submit the WiFi credential form and reach
  `POST /provision` application code instead of being rejected by ESP-IDF httpd
  with HTTP 431 before handler entry.
Reason:
  Run 011 verified the stack fix: `GET /` response completed, no panic occurred,
  and the human operator saw the setup form. Submitting SSID/password then failed
  with "Header fields are too long"; serial logs showed `httpd_parse` rejected the
  request and no `POST /provision from client` application log occurred.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - sdkconfig.defaults
  - components/wifi_provisioning/wifi_provisioning.c only for POST milestone logs
  - agent-workspaces/implementer/handoff.md for implementation notes
  - agent-workspaces/implementer/backlog.md for task status
Expected changes:
  - Add `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048` to source-controlled
    `sdkconfig.defaults`.
  - Keep the credential form as `method="post" action="/provision"`; do not move
    credentials into URL/query parameters.
  - Add POST milestone logs:
    - `POST /provision from client`
    - `POST /provision parsed form`
    - `POST /provision attempting STA ssid=<redacted-or-ssid>`
    - `POST /provision failure response sent`
    - `POST /provision success response sent`
  - Preserve all HTTP stack safety and portal reachability fixes.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md HTTP Request Header Budget.
  - Preserve neutral event and display boundaries.
Detailed behavior:
  - HTTP 431 before `POST /provision from client` is a header-budget/configuration
    failure, not a form parser or WiFi credential failure.
  - Wrong credentials must reach handler, parse form, attempt STA, return failure
    page, keep AP/HTTP active, and not save credentials.
  - Valid credentials must reach handler, parse form, attempt STA, return success,
    save credentials, then stop AP/HTTP after the success response is sent.
Non-goals:
  - Captive DNS, JavaScript, changing the form method, external assets, HTTPS,
    AP password, or UI redesign.
Acceptance criteria:
  - Clean build generated config has `CONFIG_HTTPD_MAX_REQ_HDR_LEN 2048`.
  - Phone browser POST no longer produces HTTP 431 / `request URI/header too long`.
  - Serial logs include `POST /provision from client` after form submission.
  - Wrong credential test returns failure page with no reboot and logs failure
    response sent.
  - Static audits from WiFi Provisioning Criteria remain PASS.
Constraints:
  - Do not log passwords.
  - Do not widen dependencies between WiFi provisioning and app_core/display.
Validation plan:
  - Implementer documents sdkconfig.defaults value, generated config value, build
    result, and POST milestone logs from local/manual validation if available.
  - Tester repeats Run 011 flow: GET / visible, submit bad credentials from phone,
    verify no HTTP 431, verify handler milestones and failure page.
Open questions:
  - None.
```

## WIFI_PROVISIONING_POST_SUCCESS_COMMIT

```text
Role boundary check:
  This handoff is architect-owned corrective architecture. Firmware code, build,
  flash, and validation belong to implementer/tester.
ID: WIFI_PROVISIONING_POST_SUCCESS_COMMIT
Source evidence:
  - tester/feedback.md Entry 009
  - tester/handoff.md Run 012 status
  - implementer/handoff.md TESTER_RETURN_POST_SUCCESS_UX_MISMATCH
Objective:
  Make submitted-credential success an atomic product transaction across browser
  feedback, OLED feedback, NVS persistence, AP teardown, and saved-credential
  reboot behavior.
Reason:
  Run 012 removed HTTP 431 and allowed the operator to submit the form. The browser
  then showed no success or failure page, OLED showed `WIFI OK`, the AP still
  appeared visible, and a later boot used saved credentials but failed STA auth.
  The architecture must prevent success from being reported on OLED or persisted
  as provisioned unless web feedback and saved STA behavior are consistent.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/wifi_provisioning/wifi_provisioning.c
  - components/wifi_credentials/ only if rollback/clear-provisioned helper is needed
  - components/app_core/app_core_wifi.c only to delay OLED success until neutral
    success event is emitted after web success
  - agent-workspaces/implementer/handoff.md for implementation notes
  - agent-workspaces/implementer/backlog.md for task status
Expected changes:
  - Clear submitted STA event bits before every POST connection attempt.
  - Log fresh submitted `STA got IP ip=<ipv4> source=submitted`.
  - Log saved boot `STA got IP ip=<ipv4> source=saved`.
  - Save credentials only in the submitted success path and log
    `credentials saved source=submitted`.
  - Emit `WIFI_PROV_EVENT_SUBMITTED_SUCCESS` only after
    `POST /provision success response sent`.
  - If success response send fails after credentials are saved, clear/rollback
    saved provisioned state, keep AP/HTTP active, and do not show OLED `WIFI OK`.
  - Defer HTTP/AP teardown by `2000 ms` after success response and log teardown.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md Submitted Success Commit Contract.
  - Preserve neutral event and display boundaries.
Detailed behavior:
  - Required valid-credential success sequence:
    `POST /provision from client` →
    `POST /provision parsed form` →
    `POST /provision attempting STA ssid=<redacted-or-ssid>` →
    `STA got IP ip=<ipv4> source=submitted` →
    `credentials saved source=submitted` →
    `POST /provision success response sent` →
    `WIFI_PROV_EVENT_SUBMITTED_SUCCESS` →
    `success teardown scheduled delay_ms=2000` →
    `HTTP server stopped after provisioning success` →
    `SoftAP stopped after provisioning success` →
    `STA-only mode active after provisioning success`.
  - OLED `WIFI OK` must occur after web success response, never before.
  - AP visibility immediately after submit is acceptable only during the 2000 ms
    grace window; after teardown it must stop beaconing or accepting associations.
Non-goals:
  - Captive DNS, JavaScript, changing form method, AP password, HTTPS, or automatic
    AP fallback after saved-credential boot failure.
Acceptance criteria:
  - Valid credential POST shows browser success page, not blank page.
  - Serial logs prove the full success sequence above.
  - OLED success is ordered after `POST /provision success response sent`.
  - Saved credentials connect on next STA-only boot and log
    `STA got IP ip=<ipv4> source=saved`.
  - If saved-credential reboot auth fails, full WiFi provisioning remains FAIL.
Constraints:
  - Do not log passwords.
  - Do not widen dependencies between WiFi provisioning and app_core/display.
Validation plan:
  - Implementer records build result and, if possible, local serial sequence for a
    valid POST.
  - Tester runs with live serial during POST, verifies browser success/failure,
    OLED ordering, AP teardown logs, and reboot saved-credential behavior.
Open questions:
  - None.
```

## WIFI_PROVISIONING_ARCHITECTURE

```text
Role boundary check:
  This handoff is architect-owned work for docs and contracts. Firmware code,
  builds, flash, and validation belong to implementer/tester.
ID: WIFI_PROVISIONING_ARCHITECTURE
Objective:
  Define and implement the v1 WiFi provisioning flow: open device AP, local HTTP
  credential form, STA connection attempt, NVS persistence, success/failure user
  feedback, and factory-reset recovery.
Reason:
  b06_hil needs a deterministic way for a user to enter target WiFi credentials
  without serial tooling. The flow must preserve existing display boundaries:
  provisioning may request a setup QR through app_core, but display components
  must remain independent from WiFi and network APIs.
Authorized files:
  - docs/wifi_provisioning_architecture.md
  - docs/architecture.md
  - docs/test_strategy.md
  - docs/qr_encoder_interface.md cross-reference only
  - components/wifi_credentials/
  - components/wifi_provisioning/
  - components/app_core/ for boot orchestration, provisioning callbacks, and
    display facade calls only
  - components/board/ for factory reset GPIO helper use only; do not redefine
    pin values outside docs/esp32_c3_supermini_connections.md
  - agent-workspaces/implementer/ for implementation notes
Expected changes:
  - Add wifi_credentials component that owns NVS namespace `wifi_prov` with keys
    `ssid`, `password`, and `provisioned`.
  - Add wifi_provisioning component that owns SoftAP `b06_hil_setup`, HTTP server,
    `/` and `/provision` routes, bounded form parsing, and STA connection attempts.
  - Expose only neutral provisioning lifecycle APIs and `wifi_prov_event_t`
    callbacks from wifi_provisioning; app_core registers the callback and maps
    events to display behavior.
  - Build the provisioning portal as bounded server-rendered HTML inside
    wifi_provisioning, with no JavaScript, no external assets, no filesystem
    dependency, and no public frontend API.
  - Update app_core startup to check GPIO7 factory reset, initialize credential
    storage, load credentials, enter provisioning mode when missing, connect STA
    when present, and remain disconnected after saved credential failure.
  - Route provisioning display states by neutral events into app_core; app_core
    alone calls app_core_display_show_template and app_core_display_show_qr_setup.
  - Keep display component tree free of WiFi, HTTP, esp_netif, NVS credential,
    and provisioning headers.
Module boundaries and contracts:
  - Follow docs/wifi_provisioning_architecture.md as the normative source.
  - app_core owns product orchestration and display calls.
  - wifi_credentials owns NVS read/write/erase/validate only.
  - wifi_provisioning owns SoftAP, HTTP portal, STA connection attempts, and
    reports state/results through `wifi_prov_event_cb_t`.
  - wifi_provisioning must not include app_core.h, display_controller.h, display
    headers, or know display template identifiers.
  - board owns GPIO7 active-low interpretation.
  - display remains downstream of app_core and must not observe WiFi state.
Detailed behavior:
  - Missing credentials start open AP `b06_hil_setup` at `192.168.4.1`.
  - `GET /` returns ASCII setup form with `ssid` and optional `password`.
  - `POST /provision` accepts application/x-www-form-urlencoded only, with max
    request body 256 bytes, SSID 1..32 bytes, password 0..63 bytes.
  - `GET /` page contains one POST form to `/provision` with fields `ssid` and
    `password`, maxlength values 32 and 63, and submit label `Connect`.
  - HTML responses set `Content-Type: text/html; charset=utf-8` and
    `Cache-Control: no-store`.
  - Success page contains `WiFi connected. You can close this page.`
  - Failure page contains `WiFi connection failed. Check SSID and password.` and
    includes a retry form with an empty password field.
  - Any echoed SSID is HTML-escaped; passwords are never echoed in HTML, logs, or
    implementation notes.
  - Submitted credentials are tested with STA connection timeout 30000 ms before
    saving to NVS.
  - Success: save credentials, return success page, stop HTTP/AP, stay connected
    as STA.
  - Submitted failure: do not save, return failure page with retry form, keep AP
    and HTTP active.
  - Saved credential failure at boot: do not erase, do not reopen AP, remain
    disconnected until factory reset.
  - Factory reset: GPIO7 active-low continuously for at least 2000 ms during boot
    erases NVS namespace `wifi_prov` and enters provisioning AP mode.
  - Provisioning AP QR payload is exactly `http://192.168.4.1`, routed through
    a neutral provisioning event that app_core maps to app_core_display_show_qr_setup.
Non-goals:
  - Captive DNS, HTTPS, WPA2 AP password, per-device setup password, web reset UI,
    encrypted NVS, flash encryption requirement, WiFi enterprise, automatic AP
    fallback after saved credential boot failure, automatic credential erase after
    boot failure, display-owned WiFi state, direct WiFi provisioning calls to
    app_core_display_show_*, app_core.h inclusion from wifi_provisioning, or
    direct WiFi provisioning calls to display_controller_*.
Acceptance criteria:
  - Implementation follows docs/wifi_provisioning_architecture.md exactly for
    component layout, NVS keys, AP parameters, page rendering, routes, state
    machine, timeouts, reset behavior, and display boundaries.
  - Fresh or erased credentials expose open AP `b06_hil_setup` and serve
    `http://192.168.4.1/`.
  - Valid credentials are saved only after successful STA connection.
  - Invalid submitted credentials are not saved and leave AP active.
  - Saved credentials are used on reboot without AP.
  - Saved credential failure at boot leaves the device disconnected until factory
    reset.
  - GPIO7 active-low for 2000 ms erases credentials and restarts provisioning.
  - Static audits show display code excludes WiFi/network/provisioning headers
    and wifi_provisioning excludes app_core.h, display_controller.h, and display
    headers.
Constraints:
  - Store credentials in normal NVS for v1; do not claim encryption.
  - All portal and display product copy must be ASCII-only.
  - Do not modify QR product profile beyond root-only http://IPv4.
  - Do not use workstation-specific ESP-IDF absolute paths in committed files.
Validation plan:
  - Implementer runs build and component/host tests for credential validation,
    NVS save/load/erase, and form parser bounds.
  - Tester validates AP, HTTP portal, submitted success/failure, reboot with saved
    success, reboot with saved failure, factory reset erase, and OLED QR scan per
    docs/test_strategy.md.
Open questions:
  - None for v1.
```

## OLED_TEXT_DISPLAY_INTERFACE

```text
ID: OLED_TEXT_DISPLAY_INTERFACE
Objective:
  Implement the initial 0.96 inch I2C OLED text display interface as a
  deterministic visual contract for a monochrome 128x64 display.
Reason:
  The firmware needs an implementation-ready display boundary before any agent
  adds display code. The base behavior must be portable, region-based, and
  testable without depending on one physical driver.
Authorized files:
  - components/display/ or an equivalent ESP-IDF component dedicated to display
    state, rendering, task, and driver boundaries.
  - components/app_core/ only for connecting DisplayController ownership if
    required by the implementation.
  - components/board/ only for isolated board configuration placeholders; do not
    bypass `docs/esp32_c3_supermini_connections.md` values.
  - docs/esp32_c3_supermini_connections.md for the board electrical connection
    map.
  - tests/ for host or component tests.
  - agent-workspaces/implementer/ for implementation notes.
Expected changes:
  - Add data structures that map directly to the canonical `DisplayState`,
    `DisplayLayout`, `DisplayRegion`, `TextRegion`, `TextLine`, `QrRegion`,
    `Rect`, and configuration schema in `docs/oled_text_display_interface.md`.
  - Add a canvas or framebuffer abstraction with clipped primitive drawing.
  - Add a view renderer that implements canonical text, QR, clipping, font
    selection, alignment, truncation, inverted emphasis, and draw-order rules.
  - Add a display task or tick-driven equivalent that owns the latest visual
    state, rate limits refresh, and sends rendered buffers to an isolated driver
    boundary.
  - Add a DisplayController-facing API that accepts states or layouts and does
    not expose pixel drawing to application callers.
Module boundaries and contracts:
  - `DisplayDriver` initializes and writes to hardware only; it must not decide
    content.
  - `Canvas` or `FrameBuffer` owns pixel memory and drawing primitives only.
  - `ViewRenderer` converts immutable or atomically replaced visual state into
    pixels.
  - `DisplayController` owns visual priority and is the only content producer
    for the display task.
  - `DisplayTask` receives update messages, keeps latest state, renders, and
    refreshes only when needed or forced.
Detailed behavior:
  - Follow `docs/oled_text_display_interface.md` as the normative source.
  - Follow `docs/esp32_c3_supermini_connections.md` for board-level electrical
    pins, I2C addresses, and active-low signal polarities.
  - The base display size is exactly 128x64.
  - Layouts are runtime data composed of explicit rectangular regions.
  - The screen is fully dynamic: text-only layouts are normal; QR appears only when
    the active layout includes a QR region (no permanently reserved QR area).
  - v1 display is read-only informational: push status/readings/alerts to screen;
    no menus, navigation, selection UI, or on-display data entry.
  - Occasional-use product: no OLED sleep, dimming, or display power management in v1.
  - Text supports printable ASCII only, controlled truncation without ellipsis,
    LEFT/CENTER/RIGHT horizontal alignment, TOP/CENTER/BOTTOM vertical group
    alignment, and NORMAL/INVERTED emphasis.
  - v1 product copy must not use tildes, accented letters, or non-English scripts;
    sanitize unsupported characters to ? per oled_text_display_interface.md.
  - QR regions generate regular QR Code matrices from sanitized payload text
    using LOW error correction and scale 2 with scale 1 fallback.
  - Null, malformed, out-of-bounds, overlapping, or oversized inputs must follow
    deterministic best-effort behavior.
Non-goals:
  - Do not implement icons, gauges, cursors, arbitrary bitmaps, widgets, Micro
    QR, Data Matrix, Aztec, scrolling text, animations, power management, or a
    platform-specific public visual contract.
  - Do not implement menus, navigation, highlighted selection, or any interactive
    on-display UI for v1; the OLED is informational output only.
  - Do not implement display power saving (sleep, dim, panel off) for v1; occasional
    use does not require 24/7 OLED power policies.
  - Do not treat QR_LEFT_TEXT_RIGHT as a fixed split-screen with permanent QR slot.
  - Do not add UTF-8, Unicode fonts, locale shaping, or multilingual display in v1.
  - Do not let GPIO numbers or I2C addresses leak into renderer, canvas, visual
    state, or DisplayController logic.
Acceptance criteria:
  - The implementation follows the canonical data schema and rendering
    algorithms.
  - It renders the four reference layouts: `FULL_FOUR_LINES`,
    `FULL_TWO_LINES`, `TOP_TWO_BOTTOM_ONE`, and `QR_LEFT_TEXT_RIGHT`.
  - It supports runtime layout/content replacement.
  - It supports inverted text at region and line level.
  - It generates regular QR Code output from short ASCII payloads and skips
    payloads that cannot fit.
  - It separates driver, framebuffer/canvas, renderer, display task, and display
    controller responsibilities.
  - It avoids continuous redraws when state has not changed and respects a
    configurable refresh limit defaulting to 10 Hz.
Constraints:
  - All unresolved hardware values must be marked `TODO(b06-hil):`.
  - The physical display must be treated as 128x64 only in the base version.
  - The public display API must accept visual states or layouts, not arbitrary
    pixel drawing from application modules.
  - Board configuration must use `GPIO0` for SCL, `GPIO1` for SDA, `GPIO8` as an
    active-low error LED, and `GPIO7` as an active-low factory reset switch.
  - The shared I2C bus must reserve `INA219` addresses `0x40`, `0x41`, and
    `0x42`, and allow OLED address `0x3C` or `0x3D`.
  - ESP-IDF is installed on the development computer, but global ESP-IDF
    environment variables are not created and must not be required by committed
    code, scripts, or documentation.
  - Local absolute ESP-IDF tool paths may be discovered and used only while
    running commands on the workstation; those paths must not be written to any
    file intended for the repository.
Validation plan:
  - Add host or component tests for geometry, clipping, layout switching,
    truncation, sanitization, inverted rendering, draw order, refresh policy, and
    QR fit behavior as described in `docs/test_strategy.md`.
  - Run the build using the locally installed ESP-IDF tools without committing
    workstation-specific absolute paths or generated environment files.
  - Add hardware tests after physical OLED validation on the bench.
Open questions:
  - None (SSD1306 confirmed for v1; see agent-workspaces/implementer/handoff.md).
```

## I2C_BUS_ARCHITECTURE

```text
ID: I2C_BUS_ARCHITECTURE
Objective:
  Define and implement a portable shared I2C bus layer so multiple device
  drivers can share one master bus with deterministic structure and behavior.
Reason:
  b06_hil shares one I2C bus between OLED and three INA219 devices, but the
  same bus layer must port to other MCUs and to display-only projects.
Authorized files:
  - components/i2c_bus/
  - components/board/ for board_i2c_bus_config()
  - components/app_core/ for startup orchestration
  - components/display/display_driver.* and display_types.h for hardware handle
    injection at the driver boundary only
  - docs/i2c_bus_architecture.md
  - docs/architecture.md
  - agent-workspaces/implementer/ for implementation notes
Expected changes:
  - Implement the portable public contract in docs/i2c_bus_architecture.md.
  - Keep SDK-specific code inside i2c_bus platform port only.
  - Add board helper that maps board pins to i2c_bus_config_t.
  - Initialize the bus in app_core before display startup.
  - Resolve OLED address using board constants and probe policy 0x3C then 0x3D.
  - Pass i2c_bus_device_t through display_config_t to display_driver only.
Module boundaries and contracts:
  - Follow the three-layer model: board config, portable i2c_bus, platform port.
  - i2c_bus owns one master bus and cached device handles by address.
  - board owns GPIO numbers and expected addresses for the target PCB.
  - display_driver is the only display layer that consumes the I2C device handle.
  - ina219 remains an optional future component; do not require it in app_core.
Detailed behavior:
  - Follow docs/i2c_bus_architecture.md as the normative source.
  - Use fixed component layout, constants, state machine, and caching rules from
    that document without reinterpretation.
  - Bind to ESP-IDF 5.x driver/i2c_master.h in the current target profile.
  - i2c_bus_add_device() returns the same handle for repeated address requests.
  - app_core follows the canonical startup order and OLED address policy.
  - Missing OLED at boot must not crash firmware; stub display may continue.
Non-goals:
  - SSD1306/SH1106 frame transmission.
  - Functional INA219 driver implementation.
  - 10-bit I2C, multiple buses, broker, or observability beyond the active
    authorized phase handoff.
Acceptance criteria:
  - components/i2c_bus matches fixed layout and portable contract.
  - Behavior matches canonical algorithms in docs/i2c_bus_architecture.md.
  - board remains the source of GPIO and expected addresses.
  - app_core initializes the bus before display_start().
  - display_config_t carries i2c_bus_device_t without exposing I2C to renderer or
    controller code.
  - A display-only project reuses i2c_bus unchanged.
  - Another LLM implementer can produce substantially similar code from the doc.
Constraints:
  - Do not write workstation-specific ESP-IDF absolute paths into committed
    files.
  - INA219 addresses 0x40, 0x41, and 0x42 remain board constants for b06_hil.
  - Public headers must not spread SDK driver types beyond opaque i2c_bus_device_t
    except project-wide status aliases such as esp_err_t.
Validation plan:
  - Run idf.py build after implementation using locally discovered ESP-IDF
    tools without committing workstation paths.
  - Add hardware bus tests only after physical OLED driver selection is
    confirmed.
Open questions:
  - None for bus structure. Physical OLED controller/driver remains open under
    OLED_TEXT_DISPLAY_INTERFACE.
```

## I2C_BUS_CONCURRENCY

```text
ID: I2C_BUS_CONCURRENCY
Objective:
  Define an incremental concurrency architecture for shared I2C access so
  multiple application tasks can grow onto the same bus without redesigning
  device drivers.
Reason:
  b06_hil will share one I2C bus between OLED refresh traffic and three INA219
  sensors. Physical driver serialization alone is not enough once multiple tasks
  perform concurrent I2C work.
Authorized files:
  - docs/i2c_bus_architecture.md
  - docs/test_strategy.md
  - docs/architecture.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - components/i2c_bus/ only when a phase-specific implementer handoff is active
Expected changes:
  - Document phases 1 through 4 in docs/i2c_bus_architecture.md.
  - Add i2c_bus_transceive in phase 2 and i2c_broker in phase 3 as normative
    contracts before implementation.
  - Add I2C validation criteria by phase to docs/test_strategy.md.
  - Keep b06_hil priority policy in architecture docs, not in generic bus code.
Module boundaries and contracts:
  - Follow Incremental Concurrency Architecture in docs/i2c_bus_architecture.md.
  - Phase 1: direct sync init/probe/add_device only.
  - Phase 2: i2c_bus_transceive is the only device-driver entry point.
  - Phase 3: i2c_broker is the only application-task entry point.
  - Phase 4: debug counters only unless a future telemetry handoff says otherwise.
  - Priority tables are product configuration in board or app_core.
Detailed behavior:
  - Immutable rules: one bus owner, no SDK direct access, atomic transactions,
    mandatory timeouts, recoverable errors, no long-held bus during app logic.
  - b06_hil priority table: boot/probe 255, diag 200, INA219 150, OLED 100.
  - Driver migration: phase 2 uses transceive; phase 3 uses broker_submit.
  - i2c_transaction_t shape must remain stable between phases 2 and 3.
Non-goals:
  - Implementing all phases in one change set.
  - Per-driver mutex ownership.
  - Product-specific priorities inside generic i2c_bus code.
Acceptance criteria:
  - Concurrency phases and APIs are documented normatively.
  - Each phase has explicit implementer handoff IDs: I2C_BUS_PHASE1..PHASE4.
  - Test strategy defines validation per phase.
  - Another LLM implementer can implement one phase without guessing later phases.
Constraints:
  - Implementer must execute only the active phase handoff.
  - Do not add broker code during phase 1 or 2 handoffs.
Validation plan:
  - Phase 1: build, boot, probe, missing OLED no panic.
  - Phase 2: two-task transaction test without corruption.
  - Phase 3: 60 s concurrent display and INA219 traffic without deadlock.
  - Phase 4: debug counters match observed traffic.
Open questions:
  - None.
```

## QR_ENCODER_INTERFACE

```text
ID: QR_ENCODER_INTERFACE
Objective:
  Define and implement QR matrix generation for local setup screens using the
  http://IPv4 product payload profile and a permissive MIT-licensed encoder.
Reason:
  The display renderer already draws QR regions but the encoder is a stub.
  QR payload strings arrive via display instructions; setup_url validates shape only.
Authorized files:
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md cross-references only
  - docs/test_strategy.md
  - components/qr_encoder/ and vendor Nayuki qrcodegen sources plus LICENSE
  - components/setup_url/
  - components/display/display_qr.c and include/display_qr.h
  - tests/ for encoder and setup_url validation
  - agent-workspaces/implementer/ for implementation notes
Expected changes:
  - Integrate Nayuki qrcodegen (MIT) under components/qr_encoder/vendor/.
  - Implement display_qr_generate using byte mode and LOW error correction.
  - Add setup_url_format_ipv4 and setup_url_validate for http://IPv4 only.
  - Keep DisplayController payload input via display_controller_show_qr_setup.
  - Do not couple display or encoder code to WiFi/network APIs.
Module boundaries and contracts:
  - Follow docs/qr_encoder_interface.md as normative source.
  - Instruction sources supply QR payload in display instructions; setup_url validates.
  - display_qr owns matrix encoding only; renderer owns drawing.
Detailed behavior:
  - Product payloads: http:// plus IPv4 only, implicit root /, length 14 to 22 chars,
    QR version 1 or 2.
  - No path in QR string; redirects to /something are another entity's job.
  - https excluded for v1 (unlikely TLS on small MCU local setup).
  - Version ceiling 2; payloads over 32 bytes after sanitize must fail.
  - Static matrix buffer 25x25 in display_qr; no heap in v1.
  - Only display_task may call display_qr_generate in v1.
  - QR is sporadic: shown only when the active DisplayLayout includes a QR region.
  - The screen is dynamic (text-only layouts are normal; QR layouts are occasional).
  - No permanently reserved QR screen area; layout switches replace the full view.
  - QR is instruction-driven: same delivery path as text; no display-side wait or poll.
  - QR refresh uses the same display update path as any other layout or content change.
Non-goals:
  - https, hostname, port, path in QR payload, IPv6 in v1.
  - Firmware-owned HTTP redirects or subpath routing after QR scan.
  - Hand-rolled QR encoding.
  - Display-owned payload construction from WiFi/network APIs.
  - Architectural coupling between display and connectivity subsystems.
  - A fixed always-on QR panel or split-screen reserved for QR.
  - Display-side waiting screens or polling for pending instructions.
  - QR-specific refresh, debounce, or partial-update policies separate from the
    general display refresh contract.
Acceptance criteria:
  - MIT LICENSE recorded in vendor tree.
  - Canonical payloads encode to width 21 or 25 as documented.
  - Invalid payloads return false and render blank QR regions.
  - setup_url helpers pass tests in docs/test_strategy.md.
  - Renderer remains free of Nayuki includes.
Constraints:
  - Encoder library must be MIT-licensed (Nayuki qrcodegen selected).
  - Payload profile must not expand without a new architect handoff.
Validation plan:
  - Host or component tests for setup_url and display_qr_generate sizes.
  - Hardware scan test of QR_LEFT_TEXT_RIGHT when OLED driver is active.
  - Record measured flash/RAM after first integration.
Open questions:
  - Instruction source identity (may be internal to app_core if centralized).
Closed by implementation review:
  - setup_url component path: components/setup_url/
  - Nayuki vendor path: components/qr_encoder/vendor/qrcodegen/
  - qrcodegen.c blob pin recorded by implementer:
    34f1002501fa2bcb0a054f4311795b8cbb977f5b
```

## DISPLAY_DELIVERY_CONTRACT

```text
ID: DISPLAY_DELIVERY_CONTRACT
Objective:
  Implement and enforce the v1 path for all display instructions (text and QR):
  instruction source calls app_core callbacks; only app_core calls display_controller_* directly.
Reason:
  One orchestration path avoids polling, multi-caller races, special QR channels,
  and any coupling between display and WiFi/network subsystems.
Authorized files:
  - docs/display_delivery_contract.md
  - docs/architecture.md cross-references
  - docs/oled_text_display_interface.md cross-references only
  - docs/qr_encoder_interface.md cross-references only
  - docs/test_strategy.md
  - components/app_core/ for callback entry points and sole display_controller calls
  - agent-workspaces/implementer/ for implementation notes
Expected changes:
  - Add app_core_display_show_template and app_core_display_show_qr_setup callbacks.
  - Route text and QR instructions to matching display_controller_* APIs inside app_core.
  - Validate QR payload shape with setup_url at app_core before display call.
  - Ensure no module outside app_core and display internals directly includes
    display_controller.h in v1.
  - Ensure display tree excludes WiFi/network headers.
Module boundaries and contracts:
  - Follow docs/display_delivery_contract.md as normative source.
  - v1 transport: callback or internal app_core function call only (not esp_event).
  - Instruction source: call app_core entry points; no display API calls.
  - app_core: sole display_controller_* caller; task context only; no polling.
  - Text and QR instructions MUST use identical delivery semantics.
  - WiFi/network stacks MUST remain decoupled from display architecture.
Detailed behavior:
  - Centralized orchestration expected in app_core for v1.
  - No application queue between instruction source and app_core.
  - display_task internal queue remains the only display pipeline queue.
  - Invalid QR payload rejected at app_core without display calls.
  - display_controller_show_template rejects DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT;
    QR layouts use display_controller_show_qr_setup with an explicit payload.
Non-goals:
  - esp_event transport for display instructions in v1.
  - Instruction source direct calls to display_controller_* or display_set_*.
  - Polling for pending instructions.
  - Separate QR delivery channel or display/network coupling.
  - ISR-to-display calls.
Acceptance criteria:
  - Grep audit: only app_core and display component internals directly include
    display_controller.h in v1 firmware.
  - Text and QR instructions both flow callback → app_core → direct API.
  - No display/app_core references to WiFi/network for screen selection.
  - docs/display_delivery_contract.md acceptance criteria satisfied.
Constraints:
  - Do not widen display_controller callers without new architect handoff.
  - Do not add esp_event display path without new architect handoff.
Validation plan:
  - Implementer documents grep results and app_core callback API names.
  - Tester verifies text and QR via app_core callbacks without display bypass.
Open questions:
  - Instruction source identity if not fully internal to app_core.
```

## DISPLAY_VISUAL_DEMO_PROTOCOL

```text
ID: DISPLAY_VISUAL_DEMO_PROTOCOL
Objective:
  Require a short boot-time OLED demo-test (Kconfig-gated) and handoff Demo
  Manifest so the tester and a human operator can validate what appears on the
  physical panel after display-related work.
Reason:
  Run 005 passed encoder and delivery on host/serial but did not exercise QR on
  hardware at boot. Human visual validation was implicit or reused from prior runs.
Authorized files:
  - docs/display_visual_demo_protocol.md
  - docs/test_strategy.md (Display Visual Demo Criteria)
  - docs/methodology.md (Definition Of Ready)
  - AGENTS.md (handoff protocol reference)
  - components/app_core/ (app_core_display_demo.c, Kconfig, app_core.c)
  - sdkconfig.defaults
  - agent-workspaces/implementer/ and agent-workspaces/tester/ templates
Expected changes:
  - CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO (default y in sdkconfig.defaults).
  - app_core_run_visual_demo() with baseline v1 three-step sequence.
  - Serial markers DEMO_STEP i/N name= hold_ms= and DEMO_STEP i/N done.
  - Implementer handoff Demo Manifest block; tester per-step feedback template.
Module boundaries and contracts:
  - Follow docs/display_visual_demo_protocol.md as normative source.
  - Demo steps call app_core_display_show_* only (same delivery path as production).
  - Max 4 steps, max 30 s total hold.
Detailed behavior:
  - Kconfig y: steps FULL_FOUR_LINES (5s), QR_SETUP (8s), FULL_TWO_LINES (5s).
  - Kconfig n: smoke only — single FULL_FOUR_LINES screen, no step cycling.
Non-goals:
  - Serial CLI to trigger demos; menus; automated OCR.
Acceptance criteria:
  - Protocol doc and test_strategy criteria present.
  - Firmware builds with demo wired; serial grep shows DEMO_STEP 1/3 through 3/3.
  - Implementer handoff includes filled Demo Manifest for DISPLAY_VISUAL_DEMO_PROTOCOL.
Constraints:
  - Do not add display_controller callers outside app_core.
Validation plan:
  - Tester Run 006: flash with Kconfig y, human observes each step, feedback.md entry.
Open questions:
  - None.
```

## DISPLAY_V1_IMPLEMENTATION_SNAPSHOT

```text
ID: DISPLAY_V1_IMPLEMENTATION_SNAPSHOT
Objective:
  Align architecture documentation with the current display implementation so
  future LLM implementers reproduce the same component paths, APIs, and QR
  boundaries unless a new handoff changes them.
Reason:
  The implementer concretized app_core display facade, setup_url, Nayuki vendor
  layout, display_qr static-buffer adapter, QR-only helper path, and boot visual demo.
Authorized files:
  - docs/architecture.md
  - docs/display_delivery_contract.md
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/display_visual_demo_protocol.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
Expected changes:
  - Document exact app_core public display API names.
  - Document QR_LEFT_TEXT_RIGHT rejection from text-template path.
  - Document setup_url and qr_encoder component paths.
  - Document qrcodegen call shape and static memory model.
  - Document boot visual demo behavior as implemented.
Explicitly excluded:
  - components/**, main/**, sdkconfig*, build, flash, debug, tester sign-off
Module boundaries and contracts:
  - Future implementers must preserve app_core as display facade.
  - Future implementers must keep qrcodegen isolated behind display_qr.c.
  - Future implementers must not add a second QR/display delivery path.
Acceptance criteria:
  - Normative docs specify exact current paths and APIs.
  - Closed implementation details are moved out of open questions.
  - Remaining open questions are product-level only (instruction source identity,
    future URL extensions).
Validation plan:
  - Architect review only: no build or hardware validation in this role.
Open questions:
  - Product instruction source identity if not fully internal to app_core.
```

## ARCHITECT_ROLE_HARD_STOP

```text
ID: ARCHITECT_ROLE_HARD_STOP
Objective:
  Enforce stricter architect role separation: no firmware edits, no builds, clear
  override hierarchy over plans and generic instructions.
Reason:
  Architect session crossed into components/ and idf.py despite existing role guides.
Authorized files:
  - docs/architect_role_hard_stop.md
  - docs/methodology.md (hard stop references)
  - AGENTS.md (Architect hard stop section)
  - agent-workspaces/architect/ROLE.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/implementer/ROLE.md (one-line cross-reference only)
Expected changes:
  - Canonical hard stop doc with forbidden paths, commands, override table, checklist.
  - ROLE.md hard stop summary at top.
  - AGENTS.md ownership aligned to docs/**; role switch requires named role.
Explicitly excluded (implementer / tester):
  - components/**, main/**, sdkconfig*, idf.py, flash, formal test runs
Module boundaries and contracts:
  - docs/architect_role_hard_stop.md is normative for architect sessions.
  - Mixed plans: architect slice = docs + handoff only.
Acceptance criteria:
  - Hard stop doc exists and is linked from ROLE.md, AGENTS.md, methodology.md.
  - Decision recorded in decisions.md.
  - No firmware or build changes in this handoff execution.
Validation plan:
  - Reviewer confirms architect agent can cite hard stop when asked to implement code.
Open questions:
  - None.
```

## ARCHITECT_MISSION_PILLAR

```text
ID: ARCHITECT_MISSION_PILLAR
Objective:
  Record immutable mission: architect task ends with architecture/documentation
  files; finishing the task means closing docs/handoff, not firmware or toolchain.
Reason:
  Generic "complete the task" and mixed plans were misread as permission to code.
Authorized files:
  - docs/architect_role_hard_stop.md (§ Immutable mission pillar)
  - agent-workspaces/architect/ROLE.md
  - AGENTS.md (Architect mission pillar section)
  - docs/methodology.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/architect/handoff.md
Expected changes:
  - Mission, definition of done, immutable scope, debug toolchain forbidden list.
  - Instruction priority row 0; checklist step 0 (do not code to finish).
Explicitly excluded:
  - components/**, main/**, sdkconfig*, idf.py, gdb, openocd, flash, builds
Acceptance criteria:
  - Pillar section at top of hard stop doc; cross-refs in ROLE, AGENTS, methodology.
  - Decision recorded; no firmware changes in this execution.
Validation plan:
  - Architect can state task is DONE when handoff/docs are complete without build.
Open questions:
  - None.
```

## Template

```text
Role boundary check:
  Confirm this handoff is architect-owned work (docs + contracts). Redirect code,
  builds, and validation to implementer/tester if the human request mixes roles.
ID:
Objective:
Reason:
Authorized files:
Expected changes:
Module boundaries and contracts:
Detailed behavior:
Non-goals:
Acceptance criteria:
Constraints:
Validation plan:
Open questions:
```
