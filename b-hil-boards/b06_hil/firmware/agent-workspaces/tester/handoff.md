# Tester Handoff

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting any task in this file, run the **Role Boundary Check**:

- Read `ROLE.md` in this folder, `../../AGENTS.md`, and `../../docs/methodology.md`.
- Confirm the task is **validation and evidence**, not firmware implementation
  or architecture redefinition.
- If the task requires code fixes or changes to architect handoff criteria, stop
  and tell the human to redirect to implementer or architect unless explicitly
  confirmed.

## Current Status

**Host procedures:** see `procedures.md` (NVS credential erase via `esptool`).

Run 017: **PASS** — `OLED_WIFI_CONNECTED_STATUS` (Entry 016).

Run 016: **PASS** — `OLED_PROVISIONING_SETUP_UX` (Entry 015).

Run 014: **PASS** — saved boot 10/10 (`WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY`).

Run 013: **PASS (POST)** — superseded por Run 014/015 donde aplica.

Run 012: **PARTIAL PASS** — superseded by Run 013 for POST UX.

Run 011: **PARTIAL PASS** — GET / formulario OK; POST falló con 431 (512 bytes). Corregido en Run 012 config.

Run 006: **PASS** completo para `DISPLAY_VISUAL_DEMO_PROTOCOL` (demo retired).

## OLED_WIFI_CONNECTED_STATUS — Run 017

```text
ID: OLED_WIFI_CONNECTED_STATUS (Run 017)
Source: agent-workspaces/implementer/handoff.md
NVS: saved credentials (vitriolina) from prior operator session
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Reboot serial capture -> /tmp/b06_hil_boot_log_run017.txt
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
  STA MAC: 8c:d0:b2:a9:24:cd
Result:
  Build/flash 0xdd250: PASS
  wifi_prov_event_info sta_ipv4/sta_mac fields: PASS (header review)
  emit_sta_success_event on SAVED_SUCCESS: PASS (code + serial connect)
  Saved boot STA got IP: PASS — 192.168.80.184 attempt 3/5
  Binary string WIFI OK: PASS
  Expected OLED 4-line connected screen:
    WIFI OK / 192.168.80.184 / 8C:D0:B2 / A9:24:CD
  OLED physical + match router IP/MAC: PASS — Entry 016
  POST SUBMITTED_SUCCESS connected screen: NOT EXERCISED (saved boot path; same handler)
Evidence:
  agent-workspaces/tester/test_runs.md Run 017
  agent-workspaces/tester/feedback.md Entry 016
  /tmp/b06_hil_boot_log_run017.txt
Recommendation:
  Accept OLED_WIFI_CONNECTED_STATUS.
```

## OLED_PROVISIONING_SETUP_UX — Run 016

```text
ID: OLED_PROVISIONING_SETUP_UX (Run 016)
Source: agent-workspaces/implementer/handoff.md
NVS: erased via host (procedures.md)
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  Serial capture -> /tmp/b06_hil_boot_log_run016.txt
  strings grep on firmware binary
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Result:
  Build/flash 0xdd060: PASS
  wifi_provisioning no display includes: PASS
  AP SSID HIL-06-24CE + portal 192.168.4.1: PASS
  Binary contains 1 JOIN / 2 SCAN QR strings: PASS
  No DEMO_STEP on boot: PASS
  show_provisioning_setup_display code audit (4-line split): PASS
  OLED visual (QR left, 1 JOIN / HIL-06- / 24CE / 2 SCAN QR right): PASS — Entry 015
  Phone join AP + scan QR + portal: PASS (operator, prior sessions)
Evidence:
  agent-workspaces/tester/test_runs.md Run 016
  agent-workspaces/tester/feedback.md Entry 015
  /tmp/b06_hil_boot_log_run016.txt
Recommendation:
  Accept OLED_PROVISIONING_SETUP_UX.
```

## PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID — Run 015

```text
ID: PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID (Run 015)
Source: agent-workspaces/implementer/handoff.md
NVS: erased via host (procedures.md) — fresh provisioning boot
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  Serial capture 15s -> /tmp/b06_hil_boot_log_run015.txt
  strings grep on b06_hil_firmware.bin
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
  SoftAP MAC: 8c:d0:b2:a9:24:ce
Result:
  Build/flash 0xdcfb0: PASS (matches implementer handoff)
  Binary grep b06_hil_setup / DEMO_STEP / demo symbols: PASS (absent)
  Dynamic SSID generated: PASS — HIL-06-24CE (MAC bytes 24:ce)
  starting SoftAP uses same SSID: PASS
  Portal http://192.168.4.1: PASS
  Dynamic SSID + portal: PASS
  POST human (form, home WiFi): PASS — Entry 014
  Reboot after POST: PASS — STA got IP source=saved
Evidence:
  agent-workspaces/tester/test_runs.md Run 015
  agent-workspaces/tester/feedback.md Entry 014
  /tmp/b06_hil_boot_log_run015_reboot.txt
Recommendation:
  Accept PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID (full run including human POST).
```

## WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY — Run 014

```text
ID: WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY (Run 014)
Source: agent-workspaces/implementer/handoff.md
NVS: saved credentials from Run 013 (vitriolina)
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  10-boot cold reboot sweep -> /tmp/b06_hil_run014_sweep_summary.json
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Result:
  Build/flash 0xdd160: PASS
  10/10 saved boot STA got IP source=saved: PASS
  0/10 locked failures: PASS
  reason 202 recovered on attempt 3/5 (all boots): PASS
  GPIO7 factory reset: NOT EXERCISED
Evidence:
  agent-workspaces/tester/test_runs.md Run 014
  /tmp/b06_hil_run014_sweep_summary.json
  /tmp/b06_hil_boot_log_run014_boot01.txt (sample)
Recommendation:
  Accept WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY. Close Run 013 saved-boot gap.
  Remaining: GPIO7 factory reset (not implemented).
```

## WIFI_PROVISIONING_POST_SUCCESS_COMMIT — Run 013

```text
ID: WIFI_PROVISIONING_POST_SUCCESS_COMMIT (Run 013)
Source: agent-workspaces/implementer/handoff.md
NVS at boot: erased via host (procedures.md)
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  Serial capture -> /tmp/b06_hil_boot_log_run013_full.txt
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
  Test host: ethernet only (no WiFi client for AP join)
Result:
  Build/flash 0xdcb20: PASS
  POST human UX (browser, AP teardown, OLED): PASS — Entry 011
  STA ping when connected: PASS — Entry 012
  Reboot STA got IP source=saved: INTERMITTENT — 2/5 pass, 3/5 auth fail 202 — Entry 013
  POST serial during submit: NOT CAPTURED
  GPIO7 factory reset: NOT EXERCISED
Evidence:
  agent-workspaces/tester/test_runs.md Run 013
  agent-workspaces/tester/feedback.md Entries 011–013
  /tmp/b06_hil_boot_log_run013_reboot.txt
  /tmp/b06_hil_reboot_sweep_run013.txt
Recommendation:
  Accept WIFI_PROVISIONING_POST_SUCCESS_COMMIT for POST/submitted path.
  Do NOT close full WIFI_PROVISIONING_ARCHITECTURE saved-boot reliability until
  architect decides criteria and implementer addresses intermittent auth.
  Forward to architect: TESTER_RETURN_SAVED_BOOT_RELIABILITY.
```

## Architect review — Run 013 saved-boot intermittency

```text
ID: TESTER_RETURN_SAVED_BOOT_RELIABILITY
Date: 2026-06-24
Source: Run 013 validation, feedback Entry 013, test_runs.md Run 013
Firmware: 0xdcb20, WIFI_PROVISIONING_POST_SUCCESS_COMMIT
Handoff under review: WIFI_PROVISIONING_ARCHITECTURE (reboot / saved credentials)

Summary:
  POST provisioning path PASSES (browser success, AP teardown, OLED WIFI OK, NVS save).
  Saved-credential cold boot is INTERMITTENT: 3/5 reboots fail with
  WIFI_REASON_AUTH_FAIL (202) at ~1 s; 2/5 obtain STA got IP source=saved.
  Operator saw WIFI CONNECTING then WIFI FAILED on failed boots (expected locked path).
  Same credentials work when boot succeeds; ping OK when connected.

Why credentials are not the problem:
  - NVS unchanged between pass/fail boots.
  - Successful boots: wifi:connected with vitriolina, WPA3-SAE, IP 192.168.80.184.
  - Failed boots: auth -> init before association; reason=202, not timeout alone.

Code factors (wifi_provisioning.c):
  - wifi_provisioning_connect_saved: STA-only, esp_wifi_start(), one wait_for_sta_connected.
  - POST path: APSTA with warm stack — more reliable in testing.
  - wait_for_sta_connected: single esp_wifi_connect(), 30 s wait for GOT_IP only.
  - apply_sta_config: threshold.authmode = WIFI_AUTH_WPA2_PSK; operator AP uses WPA3-SAE.
  - No app retry before SAVED_FAILURE_LOCKED / OLED WIFI FAILED HOLD RESET.

Tester recommendations for architect decision:
  1. Define v1 acceptance: single successful reboot vs N consecutive cold boots.
  2. Define supported home-router security (WPA2 only vs WPA3-SAE explicit).
  3. Decide if saved-boot should retry before locked FAILED state.
  4. If criteria tighten, authorize implementer handoff for retry/authmode/timing.

Tester does NOT change architecture or firmware. Await architect handoff.

Evidence:
  agent-workspaces/tester/feedback.md Entry 013 (full analysis)
  /tmp/b06_hil_reboot_sweep_run013.txt
  /tmp/b06_hil_boot_log_run013_post_human.txt
```

## WIFI_PROVISIONING_ARCHITECTURE

```text
ID: WIFI_PROVISIONING_ARCHITECTURE
Source: agent-workspaces/implementer/handoff.md, docs/wifi_provisioning_architecture.md,
        docs/test_strategy.md WiFi Provisioning Criteria
NVS at boot: fresh (no saved credentials)
CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO: y (demo gated off while portal active)
Commands executed:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Static audits + HTML/page code review
  Serial capture 22s -> /tmp/b06_hil_boot_log_run007.txt
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Result:
  Build/flash: PASS (0xdacc0)
  Static/component criteria: PASS
  Boot AP + DHCP 192.168.4.1: PASS (serial)
  Phone sees and joins b06_hil_setup: PASS (human)
  OLED QR → http://192.168.4.1 in browser: PASS (human)
  GET / setup form loads in browser: FAIL (blank page)
  POST /provision, reboot, GPIO7 reset: BLOCKED (portal failure)
Reproducible failures:
  1. Join b06_hil_setup from phone → open http://192.168.4.1 → blank page, no HTML.
Reproducible failure steps:
  1. Flash Run 007 firmware with fresh/erased wifi_prov credentials.
  2. Connect phone to open AP b06_hil_setup.
  3. Open http://192.168.4.1 (QR or manual).
  4. Observe: page does not load (blank).
Evidence:
  agent-workspaces/tester/test_runs.md Run 007
  agent-workspaces/tester/feedback.md Entry 003
  /tmp/b06_hil_boot_log_run007.txt
  /tmp/b06_hil_boot_log_run007b.txt
Recommendation:
  Return to implementer. Fix portal HTTP before re-test. Suggested: log httpd lifecycle,
  defer httpd_start until AP ready (WIFI_EVENT_AP_START), verify DHCP lease on phone.
```

## WIFI_PROVISIONING_ARCHITECTURE — Run 008 re-test (fix regression)

```text
ID: WIFI_PROVISIONING_ARCHITECTURE (Run 008 re-test)
Source: implementer handoff Run 007 fix, feedback Entry 003
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Serial capture 25s -> /tmp/b06_hil_boot_log_run008.txt
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Result:
  Build: PASS (0xdb2a0)
  Flash: PASS
  Boot stable: FAIL — reboot loop (ESP_ERROR_CHECK abort on wifi start timeout)
  SoftAP event logged: PASS (wifi_prov: SoftAP started at ~634 ms)
  wait_for_ap_started: FAIL — E (10645) SoftAP start timeout
  HTTP server started / portal ready: FAIL (never reached)
  GET / from phone: NOT EXERCISED (device aborts before portal ready)
Reproducible failures:
  1. Flash Run 008 firmware → boot → abort after 10 s → reboot loop.
Root cause (tester analysis):
  Race in wait_for_ap_started(): WIFI_EVENT_AP_START fires and sets bit during
  esp_wifi_start(), then wait_for_ap_started() clears the bit and waits for a
  second AP_START that never comes → ESP_ERR_TIMEOUT → ESP_ERROR_CHECK in app_core.
Evidence:
  agent-workspaces/tester/test_runs.md Run 008
  agent-workspaces/tester/feedback.md Entry 004
  /tmp/b06_hil_boot_log_run008.txt
Recommendation:
  Return to implementer. Fix: clear WIFI_AP_STARTED_BIT before esp_wifi_start(), or
  do not clear if event already received; avoid ESP_ERROR_CHECK abort on recoverable
  provisioning errors.
```

## WIFI_PROVISIONING_ARCHITECTURE — Run 009 re-test (Run 008 race fix)

```text
ID: WIFI_PROVISIONING_ARCHITECTURE (Run 009)
Source: implementer handoff Run 008 race fix, feedback Entry 004
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Serial capture 22s -> /tmp/b06_hil_boot_log_run009.txt
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Result:
  Build: PASS (0xdb290)
  Flash: PASS
  Boot stable: PASS (no reboot loop)
  Portal startup serial sequence: PASS (SoftAP → HTTP → portal ready)
  Phone GET / setup form: FAIL (human blank page; 0× GET / in serial with station joined)
Evidence:
  agent-workspaces/tester/test_runs.md Run 009
  agent-workspaces/tester/feedback.md Entry 005
  /tmp/b06_hil_boot_log_run009.txt
  /tmp/b06_hil_boot_log_run009c.txt
Recommendation:
  Return to implementer. HTTP does not reach handler despite portal ready + phone join.
  Investigate DHCP lease, httpd bind on AP netif, dhcps restart in configure_ap_netif_ip.
```

## WIFI_PROVISIONING_PORTAL_REACHABILITY — Run 010

```text
ID: WIFI_PROVISIONING_PORTAL_REACHABILITY (Run 010)
Source: implementer handoff, architect handoff, docs/test_strategy.md
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Serial capture 45s -> /tmp/b06_hil_boot_log_run010.txt
Result:
  Build/flash: PASS (0xdb770)
  Startup contract + DHCP 192.168.4.2: PASS
  GET / handler invoked: PASS
  GET / HTML delivered without panic: FAIL (stack protection fault)
  Human portal form visible: FAIL
Evidence:
  test_runs.md Run 010, feedback.md Entry 006, /tmp/b06_hil_boot_log_run010.txt
Recommendation:
  Implementer: fix httpd stack / page buffer; re-test /health then /.
```

## WIFI_PROVISIONING_HTTP_STACK_SAFETY — Run 011

```text
ID: WIFI_PROVISIONING_HTTP_STACK_SAFETY (Run 011)
Commands: idf.py build && flash; serial 50s -> /tmp/b06_hil_boot_log_run011.txt
Result:
  Build/flash: PASS (0xdc430)
  GET / response sent without panic: PASS
  Human form visible: PASS
  POST /provision: FAIL (HTTP 431 header too long; CONFIG_HTTPD_MAX_REQ_HDR_LEN=512)
Evidence: test_runs.md Run 011, feedback.md Entry 007, /tmp/b06_hil_boot_log_run011b.txt
Recommendation: Implementer raise HTTPD_MAX_REQ_HDR_LEN; re-test POST. GET / accepted.
```

## WIFI_PROVISIONING_HTTP_HEADER_BUDGET — Run 012

```text
ID: WIFI_PROVISIONING_HTTP_HEADER_BUDGET (Run 012)
Commands: reconfigure, build, flash; serial 60s -> /tmp/b06_hil_boot_log_run012.txt
Result:
  CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048: PASS (build/sdkconfig.h)
  Build/flash: PASS (0xdc5a0)
  GET / regression: PASS; no HTTP 431 in log
  POST /provision handler: NOT EXERCISED (no submit during capture)
Evidence: test_runs.md Run 012, feedback.md Entry 008
Recommendation: Operator submit form; expect handler logs without 431.
```

## DISPLAY_VISUAL_DEMO_PROTOCOL

```text
ID: DISPLAY_VISUAL_DEMO_PROTOCOL
Source: docs/display_visual_demo_protocol.md, implementer handoff
Kconfig: CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
Commands executed:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Serial capture 35s after reset
Hardware used:
  ESP32-C3 SuperMini, OLED @ 0x3C
Result:
  Build/flash: PASS (0x3a2e0)
  DEMO_STEP 1/3 FULL_FOUR_LINES: PASS (serial + visual)
  DEMO_STEP 2/3 QR_SETUP: PASS (serial + visual; phone scan OK)
  DEMO_STEP 3/3 FULL_TWO_LINES: PASS (serial + visual)
  QR phone scan: yes (http://192.168.4.1)
Reproducible failures:
  None in Run 006.
Evidence:
  agent-workspaces/tester/test_runs.md Run 006
  agent-workspaces/tester/feedback.md Entry 002
  /tmp/b06_hil_boot_log_run006.txt
Recommendation:
  Accept DISPLAY_VISUAL_DEMO_PROTOCOL and display v1 demo on hardware.
```

## DISPLAY_DELIVERY_CONTRACT + QR_ENCODER_INTERFACE

```text
ID: DISPLAY_DELIVERY_CONTRACT, QR_ENCODER_INTERFACE
Source: agent-workspaces/implementer/handoff.md, docs/test_strategy.md
Commands executed:
  idf.py build && idf.py flash
  Host qr_host_test (setup_url + display_qr)
  Serial capture + grep audit
Hardware used:
  ESP32-C3 SuperMini, OLED @ 0x3C
Result:
  Build/flash/boot: PASS (0x39f00)
  setup_url on-device self-test: PASS
  display_qr host matrix tests: PASS
  Delivery caller grep: PASS
  SSD1306 + four-line demo boot: PASS (serial; visual from Run 004)
  QR scan on physical OLED: NOT EXERCISED
  Invalid QR at runtime: NOT EXERCISED
Reproducible failures:
  None in Run 005.
Evidence:
  agent-workspaces/tester/test_runs.md Run 005
  /tmp/b06_hil_boot_log_run005.txt
Recommendation:
  Accept encoder and delivery contract for v1. Run 006: visual demo protocol
  per docs/display_visual_demo_protocol.md (QR on hardware).
```

## I2C_BUS_PHASE2

```text
ID: I2C_BUS_PHASE2
Source: agent-workspaces/implementer/handoff.md, docs/test_strategy.md
Commands executed:
  source ~/esp/esp-idf/export.sh
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 115200 / 15s after flash
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0 with I2C OLED at 0x3C
Result:
  Build: PASS (0x34de0 bytes)
  Flash: PASS
  i2c_bus_transceive / SSD1306 init: PASS
  Stable boot / display_task: PASS
  OLED I2C traffic: ACTIVE
  Two-task concurrent transceive: NOT EXERCISED
  Timeout recovery: NOT EXERCISED
  OLED-absent stub fallback: NOT EXERCISED
  Human visual FULL_FOUR_LINES: PASS (operator: b06_hil, READY, v0.1 on panel)
Reproducible failures:
  None in Run 004.
Evidence:
  agent-workspaces/tester/test_runs.md Run 004
  agent-workspaces/tester/feedback.md Entry 001
  /tmp/b06_hil_boot_log_run004.txt
Recommendation:
  Accept phase 2 for SSD1306 display path with OLED present. Before phase 3,
  add harness or INA219 for concurrent transceive and timeout tests per
  test_strategy.md.
```

## I2C_BUS_ARCHITECTURE

```text
ID: I2C_BUS_ARCHITECTURE
Source: agent-workspaces/implementer/handoff.md
Commands executed:
  source ~/esp/esp-idf/export.sh
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 115200 / 12s after flash
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0 with I2C OLED responding at 0x3C
Result:
  Build: PASS (0x33740 bytes)
  Flash: PASS
  i2c_bus_init on GPIO0/GPIO1 @ 400 kHz: PASS
  OLED probe/register at 0x3C: PASS
  Stable boot / display_task: PASS
  OLED-absent warning path: NOT EXERCISED (OLED connected)
  OLED frame flush / visual: BLOCKED (driver stub)
Reproducible failures:
  None in Run 003.
Evidence:
  agent-workspaces/tester/test_runs.md Run 003
  /tmp/b06_hil_boot_log_run003.txt
Recommendation:
  Accept I2C phase 1 for present-OLED startup. Optionally re-test OLED-absent
  bench (expect app_core warning + stub without I2C device). Next: SSD1306/SH1106
  driver in display_driver to validate physical FULL_FOUR_LINES output.
```

## OLED_TEXT_DISPLAY_INTERFACE

```text
ID: OLED_TEXT_DISPLAY_INTERFACE
Source: agent-workspaces/implementer/handoff.md (stack fix after Run 001)
Commands executed:
  source ~/esp/esp-idf/export.sh
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 115200 / 12s after flash
Hardware used:
  ESP32-C3 SuperMini on /dev/ttyACM0 (USB-Serial/JTAG)
Result:
  Build: PASS
  Flash: PASS
  display_geometry_self_test on device: PASS
  Stable boot / display_task: PASS
  OLED physical output: PASS (SSD1306 via i2c_bus_transceive; see Run 004)
Reproducible failures:
  None in Run 002. Run 001 stack protection fault resolved.
Evidence:
  agent-workspaces/tester/test_runs.md Run 002, Run 004
  /tmp/b06_hil_boot_log_run002.txt
Recommendation:
  Accept display stack for boot/runtime. Visual demo validated at driver level in
  Run 004; human panel confirmation still recommended.
```

## Template

```text
Role boundary check:
  Confirm this work is tester-owned validation. Redirect code fixes to
  implementer and criteria or design changes to architect if mixed.
ID:
Source:
Commands executed:
Hardware used:
CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO: y | n
Visual demo steps (per manifest):
  Step 1 name= / Expected / Observed / PASS|FAIL|BLOCKED
  Step 2 ...
Result:
Reproducible failures:
Evidence:
  feedback.md entry:
  test_runs.md run:
Recommendation:
```
