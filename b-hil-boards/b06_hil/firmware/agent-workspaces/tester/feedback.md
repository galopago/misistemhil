# Human Feedback

Record human observations here before turning them into bugs, requirements, or
improvements.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Entries

### Entry 023 — Run 020 post-recovery portal OK (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2 / Run 020, after NVS erase and re-provision

Human observation:
  - Device restored to portal mode after NVS erase; full flow works correctly — PASS

Classification:
  - Post-test recovery / normal provisioning path: PASS (human)
```

### Entry 022 — Run 020 connect cycle LED slow/fast repeat (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2 / Run 020, fake NVS creds NONEXISTENT_AP_XYZ

Human observation:
  - LED shows slow blink during connect attempts — PASS
  - After round failure, fast blink phase — PASS
  - Cycle repeats indefinitely (slow again, then fast, etc.) — PASS
  - OLED stays WIFI/CONNECTING; no HOLD RESET — implied by prior context

Classification:
  - v2 connect cycle LED criteria 2–4 (slow / fast 15s / repeat): PASS (human)
  - v2 no terminal boot lock during creds outage: PASS (human)
```

### Entry 021 — Run 020 portal provision + LED v2 no-creds (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2 / Run 020, after NVS erase

Human observation:
  - Portal provisioning flow successful (POST vitriolina) — PASS
  - LED behavior with no credentials / portal (solid ON, v2) — PASS

Classification:
  - v2 criterion 1 (empty NVS / portal LED ON): PASS (human)
  - Portal provisioning UX: PASS (human)
```

### Entry 020 — Run 018 error LED fast blink after saved-boot lock (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: ERROR_LED_WIFI_LINK_STATUS / Run 018, injected invalid NVS credentials

Human observation:
  - After 5 failed STA attempts with saved (invalid) credentials, error LED fast blink — PASS
  - Matches expected DISCONNECTED / ERROR_LED_PATTERN_FAST_BLINK (125 ms) after SAVED_FAILURE_LOCKED

Classification:
  - ERROR_LED_WIFI_LINK_STATUS Phase C (locked) human visual: PASS
  - ERROR_LED_WIFI_LINK_STATUS all operator-visible criteria: PASS
```

### Entry 019 — Run 018 error LED off after saved-credentials connect (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: ERROR_LED_WIFI_LINK_STATUS / Run 018, saved-credentials boot success

Human observation:
  - After STA connects with saved credentials, error LED turns off — PASS
  - Matches expected CONNECTED / ERROR_LED_PATTERN_OFF (SAVED_SUCCESS)

Classification:
  - ERROR_LED_WIFI_LINK_STATUS Phase B (connected) human visual: PASS
```

### Entry 018 — Run 018 error LED solid ON while connecting (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: ERROR_LED_WIFI_LINK_STATUS / Run 018, saved-credentials boot

Human observation:
  - With saved WiFi credentials, error LED stays solid ON while STA is connecting — PASS
  - Matches expected CONNECTING / ERROR_LED_PATTERN_ON

Classification:
  - ERROR_LED_WIFI_LINK_STATUS Phase B (connecting) human visual: PASS
```

### Entry 017 — Run 018 error LED slow blink without credentials (operator confirmed)

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: ERROR_LED_WIFI_LINK_STATUS / Run 018, fresh NVS / portal mode

Human observation:
  - Without saved WiFi credentials, error LED (GPIO8) shows slow blink — PASS
  - Matches expected UNPROVISIONED / ERROR_LED_PATTERN_SLOW_BLINK (500 ms)

Classification:
  - ERROR_LED_WIFI_LINK_STATUS Phase A human visual: PASS
```

### Entry 016 — Run 017 WiFi connected OLED (operator confirmed)

```text
Date: 2026-06-24
Person: Human operator (project owner)
Context: OLED_WIFI_CONNECTED_STATUS / Run 017, firmware 0xdd250

Human observation:
  - OLED after connect/reboot matches expected 4-line connected screen — PASS
  - Line 0: WIFI OK
  - Line 1: 192.168.80.184 (matches serial STA got IP)
  - Line 2: 8C:D0:B2
  - Line 3: A9:24:CD
  - Operator confirms layout matches tester expectation

Classification:
  - OLED_WIFI_CONNECTED_STATUS human visual: PASS
  - Saved-boot connected display: PASS
```

### Entry 015 — Run 016 OLED provisioning UX (operator confirmed)

```text
Date: 2026-06-24
Person: Human operator (project owner)
Context: OLED_PROVISIONING_SETUP_UX / Run 016

Human observation:
  - Provisioning screen matches expected layout — PASS
  - Left: QR http://192.168.4.1
  - Right: 1 JOIN / HIL-06- / MAC suffix (24CE) / 2 SCAN QR

Classification:
  - OLED_PROVISIONING_SETUP_UX human visual: PASS
```

### Entry 014 — Run 015 operator: dynamic AP + POST success

```text
Date: 2026-06-24
Person: Human operator (project owner)
Context: Run 015 firmware (PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID, 0xdcfb0)

Human observation:
  - Provisioning AP showed correct dynamic name at start (HIL-06-xxxx) — PASS
  - Filled portal form at http://192.168.4.1 — PASS
  - Device connected to home WiFi; operator reports all good so far — PASS

Tester serial follow-up (reboot after operator session):
  - Boot STA-only; no provisioning AP (HIL-06) — PASS (credentials saved)
  - saved boot retry policy active; connected attempt 3/5 — PASS
  - STA got IP ip=192.168.80.184 source=saved — PASS
  - Network vitriolina (WPA3-SAE)

Classification:
  - Dynamic AP SSID visible to operator: PASS
  - POST provisioning end-to-end (human): PASS
  - Saved credential boot after POST: PASS (1/1 sample)

Evidence:
  /tmp/b06_hil_boot_log_run015_reboot.txt
```

### Entry 013 — Reboot intermittent WIFI CONNECTING → WIFI FAILED

```text
Date: 2026-06-24
Person: Human operator + tester serial sweep
Context: After Run 013 POST success; saved credentials in NVS; SSID vitriolina (WPA3-SAE)

Human observation:
  - Rebooted board
  - OLED showed WIFI / CONNECTING
  - After a while: WIFI / FAILED (operator description)

Firmware behavior (expected on saved boot failure):
  - SAVED_CONNECTING → OLED "WIFI" / "CONNECTING"
  - wait_for_sta_connected timeout 30s without GOT_IP → SAVED_FAILURE_LOCKED
  - OLED should show four lines: WIFI / FAILED / HOLD / RESET (operator may have
    noticed only "WIFI FAILED")

Serial evidence — single failed boot (/tmp/b06_hil_boot_log_run013_post_human.txt):
  I (1108) wifi:state: auth -> init (0x600)
  W (1118) wifi_prov: STA disconnected reason=202 source=saved
  (reason 202 = WIFI_REASON_AUTH_FAIL; no STA got IP within capture)

Serial evidence — 5 reboot sweep (/tmp/b06_hil_reboot_sweep_run013.txt):
  Boot 1: FAIL — auth fail reason=202, no IP
  Boot 2: PASS — STA got IP 192.168.80.184 source=saved
  Boot 3: FAIL — auth fail reason=202, no IP
  Boot 4: PASS — STA got IP 192.168.80.184 source=saved
  Boot 5: FAIL — auth fail reason=202, no IP
  → 3/5 boots failed saved STA connect; 2/5 succeeded (intermittent)

Note: POST provisioning path succeeded (human + ping). Saved boot path is flaky.
POST used APSTA; saved boot uses STA-only single esp_wifi_connect + 30s wait.

Classification:
  - Saved-credential boot reliability: FAIL (intermittent, ~60% fail in 5-sample sweep)
  - OLED failure UX matches architecture locked-disconnected path: PASS (behavior)
  - Run 013 POST acceptance: PASS
  - Run 013 saved-credential reboot criterion: FAIL / qualified (intermittent)

Root cause analysis (tester, code + serial — for architect review):

  Credentials are NOT wrong:
    - Same NVS credentials connect when boot succeeds (vitriolina, 192.168.80.184).
    - POST provisioning with same password succeeded; ping OK when connected.

  POST path vs saved-boot path differ:
    - POST: WIFI_MODE_APSTA, WiFi stack already running (portal AP warm).
    - Saved boot: WIFI_MODE_STA, esp_wifi_start() cold, single connect attempt.
    - See wifi_provisioning.c provision_post_handler (APSTA) vs
      wifi_provisioning_connect_saved (STA-only).

  wait_for_sta_connected() behavior (wifi_provisioning.c):
    - One esp_wifi_connect() per attempt; no application-level retry loop.
    - Success bit set only on IP_EVENT_STA_GOT_IP (not merely WiFi association).
    - 30 s timeout (WIFI_PROV_STA_TIMEOUT_MS); then s_pending_sta_source cleared.
    - If GOT_IP arrives after timeout, handler ignores it (pending == NONE).
    - STA_DISCONNECTED clears success bit; auth fail at ~1 s leaves app waiting
      full timeout unless stack reconnects and gets IP in window.

  Failure signature when boot fails:
    - wifi:state: auth -> init
    - wifi_prov: STA disconnected reason=202 source=saved
    - 202 = WIFI_REASON_AUTH_FAIL (handshake/auth, not “wrong password in NVS”).
    - Never reaches wifi:connected with <ssid> on failed boots.

  Success signature when boot succeeds:
    - wifi:connected with vitriolina, security WPA3-SAE
    - wifi_prov: STA got IP ip=192.168.80.184 source=saved (~5–6 s after boot)

  apply_sta_config sets threshold.authmode = WIFI_AUTH_WPA2_PSK while operator AP
  negotiates WPA3-SAE on success. Intermittent first-handshake auth fail on cold
  STA boot is plausible (timing, PHY warm-up, WPA2/WPA3 transition).

Recommendations for architect / implementer (tester does not authorize changes):

  1. Saved-boot connect policy: explicit retry loop (N attempts or backoff) before
     SAVED_FAILURE_LOCKED; architecture currently allows locked state but not
     whether single-attempt is acceptable for WPA3 home routers.
  2. Auth mode: evaluate WIFI_AUTH_WPA3_PSK / PMF requirements for target networks;
     document v1 supported router security profiles in architecture.
  3. Timing: delay after esp_wifi_start() before first esp_wifi_connect() on cold boot.
  4. Success detection: clarify whether acceptance is GOT_IP only or association;
     consider not clearing s_pending_sta_source until retries exhausted.
  5. ESP-IDF reconnect: document or configure failure_retry_cnt / auto-reconnect
     interaction with wait_for_sta_connected.
  6. Acceptance criteria: test_strategy “reboot with saved valid credentials” may
     need “reliable across N consecutive cold boots” vs single lucky boot.
  7. Operator recovery unchanged: GPIO7 factory reset or host NVS erase per
     procedures.md when locked FAILED.

Open questions for architect:
  - Is intermittent saved-boot failure acceptable in v1 if POST path works?
  - Should v1 mandate WPA2-only routers or explicitly support WPA3-SAE cold boot?
  - Should saved-boot failure retry before showing HOLD RESET?

Evidence paths:
  /tmp/b06_hil_boot_log_run013_post_human.txt (failed boot sample)
  /tmp/b06_hil_boot_log_run013_reboot.txt (successful reboot sample)
  /tmp/b06_hil_reboot_sweep_run013.txt (5-boot sweep)
  agent-workspaces/tester/handoff.md — Architect review block Run 013
  agent-workspaces/architect/handoff.md — TESTER_RETURN_SAVED_BOOT_RELIABILITY

Forwarded to: architect (criteria decision) and implementer (after architect handoff)
```

### Entry 012 — Run 013 STA ping reachable (operator)

```text
Date: 2026-06-24
Person: Human operator (project owner)
Context: Run 013 — device connected STA after provisioning; IP from serial 192.168.80.184

Observation:
  - Ping to 192.168.80.184 from operator network — responds (ICMP reachable)

Classification:
  - STA L3 connectivity on home LAN: PASS (supplements STA got IP source=saved)

Evidence:
  - Serial: /tmp/b06_hil_boot_log_run013_reboot.txt (ip=192.168.80.184)
  - Human: ping reply from same subnet
```

### Entry 011 — Run 013 operator POST success (human + serial follow-up)

```text
Date: 2026-06-24
Person: Human operator (project owner)
Context: Run 013 firmware (WIFI_PROVISIONING_POST_SUCCESS_COMMIT, 0xdcb20)

Human observation (POST submit):
  - Browser showed "WiFi connected. You can close this page." — PASS
  - Phone WiFi list: b06_hil_setup no longer visible — PASS (AP teardown)
  - OLED showed WIFI / OK — PASS

Tester serial follow-up (reboot after operator session):
  - Boot mode STA only (no b06_hil_setup AP) — PASS
  - wifi:connected with vitriolina — PASS
  - wifi_prov: STA got IP ip=192.168.80.184 source=saved — PASS
  - No auth failure loop on full 35s capture — PASS
  - POST milestone serial during submit: NOT CAPTURED (no monitor during form)

Classification vs Run 012 regression:
  - Browser success page: PASS (was FAIL blank)
  - AP gone after success: PASS (was FAIL still visible)
  - OLED WIFI OK with UX: PASS (consistent with success; order not verified in serial)
  - Reboot saved-credentials STA: PASS (was FAIL auth -> init)

Evidence:
  /tmp/b06_hil_boot_log_run013_reboot.txt
```

### Entry 010 — Run 013 automated validation (POST blocked on host)

```text
Date: 2026-06-23
Person: Tester (automated)
Context: WIFI_PROVISIONING_POST_SUCCESS_COMMIT / Run 013

Observation:
  - Build 0xdcb20 matches implementer handoff
  - NVS erased; device boots provisioning AP b06_hil_setup correctly
  - Test host has no WiFi interface (ethernet only) — cannot join AP for HTTP/POST
  - POST success path, browser/OLED/AP teardown, reboot STA not exercised

Classification:
  - Build/flash/NVS-fresh boot: PASS
  - Static success-commit code audit: PASS
  - POST /provision + human UX + reboot: BLOCKED (needs phone + idf.py monitor)

Next step for operator:
  1. idf.py -p /dev/ttyACM0 monitor
  2. Phone joins b06_hil_setup, open http://192.168.4.1
  3. Submit valid home WiFi credentials
  4. Confirm browser shows "WiFi connected. You can close this page."
  5. Confirm OLED WIFI OK only after success response (serial)
  6. After ~2s confirm AP gone; reboot and check STA got IP source=saved
```

### Entry 009 — Operator POST session: OLED WIFI OK, blank web, AP still visible

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: Run 012 firmware after HTTP header budget fix; provisioning form submit

Observation:
  - Submitted WiFi SSID and password on http://192.168.4.1/
  - No HTTP 431 / "Header fields are too long" (improvement vs Run 011)
  - Browser showed neither success nor failure message (blank / no feedback)
  - OLED showed WIFI / OK
  - Phone WiFi settings still listed b06_hil_setup (device AP)

Tester serial follow-up (device reset after operator session):
  - Boot uses saved credentials (STA mode, no b06_hil_setup AP at boot)
  - STA authentication fails on reboot; no STA got IP logged
  - Credentials were persisted to NVS despite reboot failure

Expected (architecture):
  - Success page: "WiFi connected. You can close this page."
  - AP b06_hil_setup stops; phone should not rely on device AP after success
  - Reboot with saved credentials connects STA without reopening AP

Classification:
  - HTTP 431 regression: PASS (fixed)
  - POST web feedback: FAIL
  - OLED vs web consistency: FAIL (OK on panel, blank in browser)
  - AP teardown after success: FAIL or UNVERIFIED (operator saw AP)
  - Reboot saved-credentials STA: FAIL

Forwarded to: agent-workspaces/implementer/handoff.md
  section TESTER_RETURN_POST_SUCCESS_UX_MISMATCH
```

### Entry 008 — Run 012 HTTP header budget (automated + POST pending)

```text
Date: 2026-06-23
Person: Tester serial automation
Context: Run 012 — WIFI_PROVISIONING_HTTP_HEADER_BUDGET
Firmware: 0xdc5a0, CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048, flashed on bench

Automated verification:
  - Build/sdkconfig.h shows 2048 (was 512 in Run 011)
  - Portal ready, station joined, DHCP 192.168.4.2
  - GET / response sent — no HTTP 431, no panic
  - No POST /provision in 60s capture window

Operator action required:
  1. Join b06_hil_setup, open http://192.168.4.1
  2. Submit WiFi credentials (same flow that showed "Header fields are too long")
  3. Expected: failure or success page (not HTTP 431 plain text)
  4. Serial should show POST /provision from client → parsed form → attempting STA

Result (automated): PARTIAL PASS — config fix deployed; POST path not yet re-tested.
Result (human POST): PENDING

Evidence: /tmp/b06_hil_boot_log_run012.txt
```

### Entry 007 — Run 011: HTTP stack fix; GET / response sent without panic

```text
Date: 2026-06-23
Person: Tester serial (phone auto-connected during 50s capture)
Context: Run 011 — WIFI_PROVISIONING_HTTP_STACK_SAFETY
Firmware: 0xdc430, flashed on bench

Fix verified:
  - No Guru Meditation / Stack protection fault / abort (0 in 50s, single boot)
  - I (8156) wifi_prov: GET / from client
  - I (8156) wifi_prov: GET / response sent
  - Station joined mac=46:34:48:c3:db:03 aid=1
  - Portal boot sequence intact (AP netif before start, portal ready)
  - Browser favicon.ico 404 (expected); no crash

Not exercised in capture:
  - GET /health (0× in log)
  - DHCP assigned IP log line (phone may have reused lease; GET succeeded)
  - POST /provision, reboot, factory reset

Human portal form: serial proves HTML response completed; operator should confirm
  form visible on phone (Run 010 symptom was crash-before-send; now fixed in log).

Classification: PASS for Run 011 / HTTP stack safety and GET / delivery criterion.

Human update (same session): Operator confirms setup form visible on phone — GET / PASS.

POST /provision failure (operator): After submitting WiFi SSID/password, browser shows
  "Header fields are too long".

Serial confirmation (/tmp/b06_hil_boot_log_run011b.txt):
  W httpd_parse: parse_block: request URI/header too long
  W httpd_txrx: httpd_resp_send_err: 431 Request Header Fields Too Large - Header fields are too long
  → POST never reaches provision_post_handler (no "POST /provision from client" log).

Root cause (tester): CONFIG_HTTPD_MAX_REQ_HDR_LEN=512 in sdkconfig; mobile browser POST
  headers exceed limit before body is parsed.

Classification POST: FAIL — blocked provisioning submit path.

Evidence: /tmp/b06_hil_boot_log_run011.txt, /tmp/b06_hil_boot_log_run011b.txt

Next action: Implementer increase CONFIG_HTTPD_MAX_REQ_HDR_LEN (e.g. 1024–2048) or
  architecture/sdkconfig.defaults fix; re-test POST /provision.
```

### Entry 006 — Run 010: reachability fixed, GET / crashes (stack fault)

```text
Date: 2026-06-23
Person: Human operator + tester serial (phone auto-reconnected during capture)
Context: Run 010 — WIFI_PROVISIONING_PORTAL_REACHABILITY — architect + implementer fix
Firmware: 0xdb770, flashed on bench

Architectural fix verified (serial):
  I (585) wifi_prov: AP netif ip=192.168.4.1 gw=192.168.4.1 netmask=255.255.255.0
  I (595) wifi_prov: starting SoftAP ssid=b06_hil_setup mode=AP
  I (665) wifi_prov: SoftAP started
  I (695) wifi_prov: provisioning portal ready at http://192.168.4.1
  I (1245) wifi_prov: station joined mac=46:34:48:c3:db:03 aid=1
  I (1405) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.4.2
  → L3/DHCP issue from Run 009 appears FIXED.

HTTP reachability:
  GET / from client: logged (2× in capture)
  GET /health from client: 0 (not exercised)

Failure on first GET /:
  I (36935) wifi_prov: GET / from client
  Guru Meditation Error: Core 0 panic'ed (Stack protection fault)
  Detected in task (httpd worker) at 0x4209e7a6
  → Device reboots; browser shows blank page (connection dies mid-request).

Human observation (operator): still blank page after connect + open URL (consistent with crash).

Classification: FAIL for portal acceptance. Reachability milestone PASS; handler crash FAIL.

Implementer hints:
  - httpd task default stack likely too small for wifi_prov_send_setup_page (html[2048] on stack).
  - Increase httpd_config.stack_size and/or move HTML buffer static.
  - Re-test /health (small response) to isolate stack vs HTML size.

Evidence: /tmp/b06_hil_boot_log_run010.txt

Next action: Return to implementer; do not accept until GET / returns HTML without panic.
```

### Entry 005 — Run 009 human GET / still blank; serial shows no HTTP reach ESP

```text
Date: 2026-06-23
Person: Human operator (project owner) + tester serial capture
Context: Run 009 — WIFI_PROVISIONING_ARCHITECTURE — after Run 008 race fix
Firmware: c31c917-dirty, 0xdb290, flashed on bench

Human observation:
  Connects to b06_hil_setup, opens http://192.168.4.1 (QR or browser).
  Page remains blank — no form, no content (same as Entry 003).

Serial evidence (/tmp/b06_hil_boot_log_run009c.txt, 40s capture incl. phone session):
  I (655) wifi_prov: SoftAP started
  I (695) wifi_prov: HTTP server started on port 80
  I (705) wifi_prov: provisioning portal ready at http://192.168.4.1
  I (2445) wifi:station: 46:34:48:c3:db:03 join, AID=1, bgn, 20
  GET / from client: 0 occurrences in entire capture
  No abort, no reboot loop, no SoftAP timeout

Interpretation:
  L2 WiFi association works (phone MAC joins AP).
  HTTP handler root_get_handler is NEVER invoked while operator reports blank page.
  Failure is NOT "empty HTML response" — TCP/HTTP does not reach httpd (or phone never
  sends to 192.168.4.1 on WiFi interface).

Likely implementer investigation areas:
  1. DHCP: confirm phone receives 192.168.4.x (log esp_netif_dhcps lease / AP_STACONNECTED).
  2. configure_ap_netif_ip() stops/restarts dhcps after esp_wifi_start — may disrupt AP L3.
  3. httpd may not accept on softAP netif (bind_addr / netif attachment / LWIP routing).
  4. Phone may route http://192.168.4.1 via mobile data — operator should disable cellular.
  5. Optional: captive-portal probe handlers (generate_204, hotspot-detect) — v1 has no
     captive DNS but phones may not hit / without lease.

Classification: FAIL — GET / portal criterion not met; Run 009 PARTIAL PASS withdrawn.

Next action: Return to implementer with log file; add DHCP + HTTP accept diagnostics.
```

### Entry 004 — Run 008 re-test: fix causes reboot loop (AP start race)

```text
Date: 2026-06-23
Person: Tester (automated serial) — human portal re-test blocked by crash
Context: Run 008 — WIFI_PROVISIONING_ARCHITECTURE — implementer Run 007 portal fix
Prior: Entry 003 FAIL (blank page on GET /)

Steps exercised:
  1. Build + flash implementer fix (0xdb2a0) — PASS
  2. Boot and provisioning portal startup — FAIL
  3. Phone GET http://192.168.4.1/ — NOT EXERCISED (firmware reboot loop)

Observed (serial):
  I (634) wifi_prov: SoftAP started
  I (644) esp_netif_lwip: DHCP server started ... IP: 192.168.4.1
  E (10645) wifi_prov: SoftAP start timeout
  ESP_ERROR_CHECK failed: ESP_ERR_TIMEOUT at app_core.c:72 app_core_wifi_start()
  abort() → continuous reboot loop

Expected result:
  After SoftAP started: HTTP server started, portal ready, stable runtime.
  Phone can load GET / form (test_strategy.md).

Classification: FAIL — regression vs Run 007 (which at least booted and served AP join).

Root cause hypothesis (for implementer):
  wait_for_ap_started() calls xEventGroupClearBits(WIFI_AP_STARTED_BIT) AFTER
  esp_wifi_start(), but WIFI_EVENT_AP_START already fired in the handler and set the
  bit. Clearing it then waiting 10 s for a second AP_START always times out.

Suggested fix:
  - Clear AP_STARTED bit before esp_wifi_start(), then wait; OR
  - If bit already set after esp_wifi_start(), skip wait; OR
  - Register handler and wait before start (standard ESP-IDF pattern).

Evidence: /tmp/b06_hil_boot_log_run008.txt

Next action: Return to implementer; do not accept Run 007 fix.
```

### Entry 003 — Run 007 WiFi portal GET / fails on phone

```text
Date: 2026-06-23
Person: Human operator (project owner)
Context: Run 007 — WIFI_PROVISIONING_ARCHITECTURE — fresh NVS / provisioning AP
Handoff criterion: Fresh provisioning flow — GET http://192.168.4.1/ serves setup form

Steps exercised (only step reachable on bench):
  1. Phone sees open WiFi AP b06_hil_setup — PASS
  2. Phone joins AP — PASS (operator)
  3. OLED QR scans to http://192.168.4.1 — PASS (operator; browser opens URL)
  4. Browser loads setup page at 192.168.4.1 — FAIL

Observed (human):
  After joining b06_hil_setup and opening http://192.168.4.1 (via QR or manual URL),
  the browser shows a blank page. No HTML form, no error text, nothing loads.
  Remaining flow (POST /provision, reboot, factory reset) NOT EXERCISED — blocked.

Expected result:
  GET / returns ASCII HTML form with ssid and password fields (test_strategy.md).

Serial evidence (tester, same session):
  - DHCP server on AP: I (645) esp_netif_lwip: DHCP server started ... IP: 192.168.4.1
  - Phone associated: I (1425) wifi:station: 46:34:48:c3:db:03 join, AID=1
  - No httpd / wifi_prov application logs on GET (component has no request logging)
  - No panic or reboot loop during association window

Classification: FAIL — portal HTTP not usable from client; blocks all provisioning criteria
  that depend on the web form.

Hypotheses for implementer (not validated):
  1. httpd_start() runs immediately after esp_wifi_start() without waiting for
     WIFI_EVENT_AP_START (architecture § Start_open_AP_and_HTTP says "after SoftAP is ready").
  2. No diagnostic logs — cannot confirm whether TCP reaches root_get_handler.
  3. Phone may lack DHCP lease or route to 192.168.4.1 (check IP on phone: expect 192.168.4.x).
  4. APSTA mode without captive handlers: some phones probe external URLs first; direct
     http://192.168.4.1 should still work if L3 routing is correct.

Suggested implementer actions:
  - Add ESP_LOGI in wifi_prov for httpd_start result, AP_START event, and GET / handler entry.
  - Defer httpd_start until WIFI_EVENT_AP_START (or IP ready on s_ap_netif).
  - Re-test with phone showing assigned IP and mobile data disabled.
  - Optional: log esp_netif_dhcps_get_clients or lease events.

Next action: Return to implementer; do not accept WIFI_PROVISIONING_ARCHITECTURE.
```

### Entry 002 — Run 006 visual demo per-step

```text
Date: 2026-06-22
Person: Human operator (project owner) + tester serial
Context: Run 006 — DISPLAY_VISUAL_DEMO_PROTOCOL — CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
Handoff manifest steps: 3

Step 1 — name=FULL_FOUR_LINES
  Expected: Three lines b06_hil, READY, v0.1 (5s hold)
  Observed (human): Same as Entry 001 (prior run); serial step completed
  Result: PASS

Step 2 — name=QR_SETUP
  Expected: QR left, text SCAN/ME right; scans http://192.168.4.1 (8s hold)
  Observed (human): QR visible with text "SCAN" / "ME" on the right; phone camera
    recognizes the URL (http://192.168.4.1)
  QR scanned on phone: yes
  Result: PASS

Step 3 — name=FULL_TWO_LINES
  Expected: TEXT and ONLY only; no QR pixels (5s hold)
  Observed (human): Two lines visible: "TEXT" and "ONLY"; no residual QR
  Result: PASS

Serial evidence: grep DEMO_STEP in /tmp/b06_hil_boot_log_run006.txt — all 3/3 done
Technical conclusion: PASS (serial/protocol + human visual all steps)
Next action: Close Run 006 visual demo; no display bug filed
```

### Entry 001 — Run 004 visual OLED confirmation

```text
Date: 2026-06-20
Person: Human operator (project owner)
Context: Run 004 I2C_BUS_PHASE2 / SSD1306 demo after flash on ESP32-C3 SuperMini
Observation: Physical OLED shows three left-aligned text lines top to bottom:
  line 1 "b06_hil", line 2 "READY", line 3 "v0.1". Fourth demo line not visible
  (expected: demo array includes empty fourth string).
Expected result: FULL_FOUR_LINES demo from app_core (b06_hil, READY, v0.1, "")
Steps to reproduce: Flash Run 004 firmware; observe OLED after boot
Evidence: Human visual inspection on test bench; serial log Run 004
Classification: Pass — matches expected demo content and layout
Next action: Close Run 004 visual criterion; no display bug filed
```

### Entry 002 — Visual demo per-step template

Use one entry per test run (or one per step if steps fail independently).

```text
Date:
Person:
Context: Run NNN — handoff ID — CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y|n
Handoff manifest steps: N

Step 1 — name=FULL_FOUR_LINES
  Expected:
  Observed (human):
  Result: PASS | FAIL | BLOCKED

Step 2 — name=QR_SETUP
  Expected:
  Observed (human):
  QR scanned on phone: yes | no | not tried
  Result: PASS | FAIL | BLOCKED

Step 3 — name=FULL_TWO_LINES
  Expected:
  Observed (human):
  Result: PASS | FAIL | BLOCKED

Serial evidence: grep DEMO_STEP in <log path>
Technical conclusion: PASS | FAIL | BLOCKED (whole demo)
Next action:
```

## Template

```text
Date:
Person:
Context:
Observation:
Expected result:
Steps to reproduce:
Evidence:
Classification:
Next action:
```
