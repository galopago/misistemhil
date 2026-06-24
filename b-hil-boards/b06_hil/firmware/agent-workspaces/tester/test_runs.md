# Test Runs

Record each manual or automated validation.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Template

```text
Date:
Firmware:
ESP-IDF:
Hardware:
Commands:
Result:
Logs:
Conclusion:
```

## Runs

### Run 001 — OLED_TEXT_DISPLAY_INTERFACE first hardware flash

```text
Date: 2026-06-20
Firmware: commit 2f7ffa317dff64ecf74ee3c19512b21e543b1d6d (OLED_TEXT_DISPLAY_INTERFACE base)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 (QFN32) revision v0.4, 4MB flash, USB-Serial/JTAG on /dev/ttyACM0
          MAC 8c:d0:b2:a9:24:cd; SuperMini connected via USB (OLED not validated visually)
Commands:
  source ~/esp/esp-idf/export.sh
  cd firmware
  idf.py set-target esp32c3
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  python3 serial capture (115200, 10s, with RTS reset) -> /tmp/b06_hil_boot_log.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0x2dd60 bytes)
  Flash: PASS (esptool v4.9.0, chip ESP32-C3)
  Boot + geometry self-test: PASS
  display_task startup: FAIL — reboot loop
  OLED physical output: BLOCKED (driver stub; crash before stable runtime)
Logs:
  I (281) b06_hil: Starting b06_hil firmware
  I (291) app_core: Board: b06_hil
  I (291) app_core: I2C SCL GPIO0 SDA GPIO1 (driver not enabled yet)
  I (301) display_geom: Display geometry self-test passed
  I (301) display_driver: Display driver stub initialized (TODO(b06-hil): SSD1306/SH1106 I2C driver)
  Guru Meditation Error: Core  0 panic'ed (Stack protection fault).
  Detected in task "display_task" at 0x42008fe6
  Stack pointer: 0x3fc965f0
  Stack bounds: 0x3fc99e80 - 0x3fc9ae70
  (15 reboot cycles observed in 10s capture; "display_task: Display task started" never logged)
Conclusion:
  Partial pass. Build/flash and on-device geometry self-test succeed.
  Firmware is not stable: display_task stack protection fault causes continuous reboot loop
  immediately after display_driver_init during display_start().
  OLED visual demo (FULL_FOUR_LINES) cannot be validated on hardware in this iteration.
```

### Run 002 — display_task stack overflow fix re-test

```text
Date: 2026-06-20
Firmware: commit 2f7ffa317dff64ecf74ee3c19512b21e543b1d6d-dirty (display_task mailbox + 8192 stack fix)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 (QFN32) revision v0.4, USB-Serial/JTAG on /dev/ttyACM0
          MAC 8c:d0:b2:a9:24:cd; SuperMini connected via USB
Commands:
  source ~/esp/esp-idf/export.sh
  cd firmware
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  python3 serial capture (115200, 12s, with RTS reset) -> /tmp/b06_hil_boot_log_run002.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0x2da30 bytes)
  Flash: PASS
  Boot + geometry self-test: PASS
  display_task startup: PASS (stable, no reboot loop in 12s)
  OLED physical output: BLOCKED (driver stub; framebuffer not sent to panel)
Logs:
  I (281) b06_hil: Starting b06_hil firmware
  I (291) app_core: Board: b06_hil
  I (291) app_core: I2C SCL GPIO0 SDA GPIO1 (driver not enabled yet)
  I (301) display_geom: Display geometry self-test passed
  I (301) display_driver: Display driver stub initialized (TODO(b06-hil): SSD1306/SH1106 I2C driver)
  I (311) display_task: Display task running
  I (321) display_task: Display task started
  I (321) main_task: Returned from app_main()
  (single boot in 12s capture; no Guru Meditation / panic)
Conclusion:
  PASS for build, flash, stable boot, geometry self-test, and display_task lifecycle.
  Demo FULL_FOUR_LINES runs in internal framebuffer only; OLED visual validation remains
  blocked until SSD1306/SH1106 I2C driver is implemented.
```

### Run 003 — I2C_BUS_ARCHITECTURE phase 1 hardware validation

```text
Date: 2026-06-20
Firmware: commit 2f7ffa317dff64ecf74ee3c19512b21e543b1d6d-dirty (I2C_BUS_ARCHITECTURE phase 1)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 (QFN32) revision v0.4, USB-Serial/JTAG on /dev/ttyACM0
          MAC 8c:d0:b2:a9:24:cd; SuperMini with I2C OLED present at 0x3C
Commands:
  source ~/esp/esp-idf/export.sh
  cd firmware
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  python3 serial capture (115200, 12s, with RTS reset) -> /tmp/b06_hil_boot_log_run003.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0x33740 bytes)
  Flash: PASS
  I2C bus init (GPIO0/GPIO1, 400 kHz, pull-ups): PASS
  OLED probe 0x3C then register device: PASS (detected at 0x3C)
  Boot + geometry self-test: PASS
  display_task startup: PASS (stable, no reboot loop in 12s)
  OLED frame transmission: BLOCKED (display_driver flush still stub)
  OLED-absent boot path: NOT EXERCISED (OLED connected on test bench)
Logs:
  I (295) app_core: I2C SCL GPIO0 SDA GPIO1
  I (305/315) gpio: GPIO[1]/GPIO[0] OpenDrain Pullup configured
  I (325) i2c_bus: I2C bus ready on SCL GPIO0 SDA GPIO1 at 400000 Hz
  I (325) display_geom: Display geometry self-test passed
  I (335) app_core: OLED detected at I2C address 0x3C
  I (335) i2c_bus: Registered I2C device 0x3C
  I (345) display_driver: Display driver stub initialized with I2C device 0x3C (...)
  I (355) display_task: Display task running
  I (355) display_task: Display task started
  I (365) main_task: Returned from app_main()
  (single boot in 12s capture; no Guru Meditation / panic / I2C errors)
Conclusion:
  PASS for I2C_BUS_ARCHITECTURE phase 1 on hardware with OLED present.
  Shared bus initializes on confirmed pins, probes 0x3C successfully, registers
  device handle, and passes it to display_driver without destabilizing runtime.
  Visual OLED output and OLED-absent warning path remain unverified on this bench.
```

### Run 004 — I2C_BUS_PHASE2 transaction executor + SSD1306 hardware validation

```text
Date: 2026-06-20
Firmware: commit 2f7ffa317dff64ecf74ee3c19512b21e543b1d6d-dirty (I2C_BUS_PHASE2)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 (QFN32) revision v0.4, USB-Serial/JTAG on /dev/ttyACM0
          MAC 8c:d0:b2:a9:24:cd; SuperMini with I2C OLED at 0x3C (SSD1306 assumed)
Commands:
  source ~/esp/esp-idf/export.sh
  cd firmware
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  python3 serial capture (115200, 15s, with RTS reset) -> /tmp/b06_hil_boot_log_run004.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0x34de0 bytes, matches implementer handoff)
  Flash: PASS
  I2C phase 1 (bus init, probe, register @ 0x3C): PASS
  SSD1306 init via i2c_bus_transceive: PASS
  Stable boot / display_task (15s, single boot): PASS
  SSD1306 flush errors in log: NONE
  OLED physical traffic: ACTIVE (init succeeded; demo flush path exercised silently)
  Visual FULL_FOUR_LINES on panel: PASS (human confirmation: 3 lines "b06_hil", "READY", "v0.1")
  Two-task concurrent transceive test: NOT EXERCISED (no second I2C consumer in firmware)
  Transaction timeout recovery test: NOT EXERCISED (no dedicated harness)
  OLED-absent stub fallback: NOT EXERCISED (OLED connected)
Logs:
  I (326) i2c_bus: I2C bus ready on SCL GPIO0 SDA GPIO1 at 400000 Hz
  I (326) display_geom: Display geometry self-test passed
  I (336) app_core: OLED detected at I2C address 0x3C
  I (336) i2c_bus: Registered I2C device 0x3C
  I (346) display_driver: SSD1306 initialized at I2C address 0x3C
  I (356) display_task: Display task running
  I (356) display_task: Display task started
  I (366) main_task: Returned from app_main()
  (no Guru Meditation, panic, SSD1306 init/flush warnings, or I2C errors)
Conclusion:
  PASS for I2C_BUS_PHASE2 on current bench including human visual sign-off.
  Operator confirmed demo text on physical OLED (3 visible lines; 4th line empty).
  Deferred: concurrent two-task contention, timeout recovery, OLED-absent path.
```

### Run 005 — DISPLAY_DELIVERY_CONTRACT + QR_ENCODER_INTERFACE validation

```text
Date: 2026-06-22
Firmware: commit 18452bfa478918151bc0589d2401fde9850b6013-dirty
Handoffs: DISPLAY_DELIVERY_CONTRACT, QR_ENCODER_INTERFACE
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED at 0x3C
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Host test: gcc /tmp/qr_host_test (setup_url + display_qr + Nayuki qrcodegen)
  Serial capture 115200 / 15s -> /tmp/b06_hil_boot_log_run005.txt
  Static audit: grep display_controller.h, esp_wifi in app_core/display
Result:
  Build: PASS (b06_hil_firmware.bin 0x39f00 bytes, matches implementer handoff)
  Flash: PASS
  On-device setup_url_self_test: PASS
  On-device display_geometry_self_test: PASS
  SSD1306 init + stable display_task: PASS
  Host setup_url + display_qr matrix tests: PASS (width 25/21, reject https/path/null)
  Caller discipline grep: PASS (display_controller.h only in app_core + display/)
  No WiFi/esp_event in app_core or display: PASS
  No hardcoded setup URL in display_controller: PASS
  app_core uses app_core_display_show_template at boot: PASS (delivery callback path)
  QR_LEFT_TEXT_RIGHT via show_template: PASS (code rejects ESP_ERR_INVALID_ARG)
  QR hardware scan (QR_LEFT_TEXT_RIGHT on panel): NOT EXERCISED (boot shows text demo only)
  Invalid QR rejection on device: NOT EXERCISED (no runtime trigger in boot image)
  Layout templates FULL_TWO_LINES / TOP_TWO_BOTTOM_ONE on hardware: NOT EXERCISED
Logs:
  I (330) display_geom: Display geometry self-test passed
  I (340) setup_url: self-test passed
  I (340) app_core: OLED detected at I2C address 0x3C
  I (350) display_driver: SSD1306 initialized at I2C address 0x3C
  I (360) display_task: Display task started
  App version: 18452bf-dirty (Jun 22 2026 09:38:10)
  (no panic / reboot loop in 15s)
Host test output:
  PASS setup_url_self_test
  PASS qr 192.168.4.1 (width 25)
  PASS qr 1.1.1.1 (width 21)
  PASS reject null, empty, https, path
Nayuki pin: qrcodegen.c sha 34f1002501fa2bcb0a054f4311795b8cbb977f5b (per implementer handoff)
Conclusion:
  PASS for QR encoder, setup_url, display delivery contract (static + on-device self-tests),
  and regression of SSD1306 text demo boot path.
  QR scannable hardware test and alternate layout templates require a boot demo hook or
  manual invocation of app_core_display_show_qr_setup — not present in current app_core_start.
```

### Run 006 — DISPLAY_VISUAL_DEMO_PROTOCOL (boot 3-step visual demo)

```text
Date: 2026-06-22
Firmware: commit 3cb2283e78dbbd0b3b78d4671e0f30894afdb6ec-dirty
Handoff: DISPLAY_VISUAL_DEMO_PROTOCOL
Kconfig: CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 115200 / 35s (covers full 18s hold + boot) -> /tmp/b06_hil_boot_log_run006.txt
Manifest (implementer):
  Step 1 FULL_FOUR_LINES 5s — b06_hil, READY, v0.1, ""
  Step 2 QR_SETUP 8s — http://192.168.4.1, SCAN, ME
  Step 3 FULL_TWO_LINES 5s — TEXT, ONLY
Result:
  Build: PASS (b06_hil_firmware.bin 0x3a2e0 bytes)
  Flash: PASS
  Boot stable / no panic: PASS
  Serial DEMO_STEP sequence: PASS (1/3, 2/3, 3/3 in order, no demo step errors)
  Hold timing (from log timestamps): ~5010ms, ~8000ms, ~5000ms (matches manifest)
  Visual step 1 FULL_FOUR_LINES: PASS (Entry 001 + Run 006)
  Visual step 2 QR_SETUP: PASS (QR + SCAN/ME; phone scans URL)
  Visual step 3 FULL_TWO_LINES: PASS (TEXT, ONLY; no residual QR)
Logs:
  I (370) app_core_demo: DEMO_STEP 1/3 name=FULL_FOUR_LINES hold_ms=5000
  I (5380) app_core_demo: DEMO_STEP 1/3 done
  I (5410) app_core_demo: DEMO_STEP 2/3 name=QR_SETUP hold_ms=8000
  I (13410) app_core_demo: DEMO_STEP 2/3 done
  I (13410) app_core_demo: DEMO_STEP 3/3 name=FULL_TWO_LINES hold_ms=5000
  I (18410) app_core_demo: DEMO_STEP 3/3 done
  I (18410) main_task: Returned from app_main()
Conclusion:
  PASS for DISPLAY_VISUAL_DEMO_PROTOCOL (serial, timing, and human visual all 3 steps).
  Operator confirmed QR scannable at http://192.168.4.1 and clean transition to TEXT/ONLY.
```

### Run 007 — WIFI_PROVISIONING_ARCHITECTURE (partial: build + static + serial boot)

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_ARCHITECTURE
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS state at boot: fresh / no saved credentials (AP provisioning path)
CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO: y (demo skipped while portal active — expected)
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Static audits (grep display/wifi boundaries, HTML pages review)
  Serial capture 115200 / 22s -> /tmp/b06_hil_boot_log_run007.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdacc0 bytes, matches implementer handoff)
  Flash: PASS
  NVS namespace wifi_prov keys ssid/password/provisioned: PASS (code review)
  wifi_credentials_validate bounds 1..32 / 0..63: PASS (code review)
  Form parser max body 256 bytes + percent-decode rejection: PASS (code review)
  HTML pages: form post /provision, maxlength 32/63, Connect label: PASS (code review)
  Success/failure page strings per architecture: PASS (code review)
  No JS/CSS/CDN/external assets in pages: PASS (code review)
  Password never echoed in failure retry form HTML: PASS (code review)
  Display tree no WiFi/HTTP/NVS includes: PASS (grep)
  wifi_provisioning no app_core/display includes: PASS (grep)
  wifi_provisioning.h neutral API only: PASS (header review)
  display_controller.h only app_core + display: PASS (grep)
  Boot stable / no panic: PASS
  SoftAP + DHCP 192.168.4.1 on boot (no saved creds): PASS (serial)
  DEMO_STEP demo skipped during AP mode: PASS (no DEMO_STEP in log; app_core_wifi gate)
  AP SSID b06_hil_setup visible on bench: NOT EXERCISED (host has no WiFi radio)
  GET http://192.168.4.1/ from client: NOT EXERCISED
  POST /provision bad/valid credentials: NOT EXERCISED
  OLED QR WIFI SETUP during provisioning: NOT EXERCISED (human visual)
  Reboot with saved credentials: NOT EXERCISED
  Saved-credential failure locked path: NOT EXERCISED
  GPIO7 factory reset 2000 ms: NOT EXERCISED
Logs:
  I (635) wifi:mode : sta (...) + softAP (...)
  I (645) esp_netif_lwip: DHCP server started on interface WIFI_AP_DEF with IP: 192.168.4.1
  I (685) main_task: Returned from app_main()
  (no panic / reboot loop in 22s capture)
  Human portal test (phone): FAIL — AP visible and joinable; QR opens http://192.168.4.1
    but browser shows blank page (no form, no content). See feedback.md Entry 003.
  Phone L2 join in serial: PASS (46:34:48:c3:db:03)
  GET / HTTP response observed on client: FAIL
Logs (portal failure session):
  /tmp/b06_hil_boot_log_run007b.txt
  I (645) esp_netif_lwip: DHCP server started on interface WIFI_AP_DEF with IP: 192.168.4.1
  I (1425) wifi:station: 46:34:48:c3:db:03 join, AID=1, bgn, 20
  (no httpd GET logs — wifi_provisioning has no request-level logging)
Conclusion:
  FAIL for WIFI_PROVISIONING_ARCHITECTURE hardware acceptance.
  Static/component criteria PASS; fresh provisioning portal criterion FAIL on phone.
  Return to implementer with Entry 003 evidence. POST/reboot/factory-reset remain blocked.
```

### Run 008 — WIFI_PROVISIONING_ARCHITECTURE re-test (Run 007 fix regression)

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty (Run 007 fix in wifi_provisioning.c)
Handoff: WIFI_PROVISIONING_ARCHITECTURE (Run 007 portal fix)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS state at boot: fresh / no saved credentials
Implementer fix summary:
  - Wait WIFI_EVENT_AP_START before httpd; WIFI_MODE_AP at boot; apply_sta_config on POST
  - Diagnostic logs in wifi_prov
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 25s -> /tmp/b06_hil_boot_log_run008.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdb2a0 bytes, matches implementer handoff)
  Flash: PASS
  Static audits: PASS (unchanged boundaries)
  WIFI_MODE_AP at boot (not APSTA): PASS — I (634) wifi:mode : softAP
  wifi_prov SoftAP started log: PASS (event fires ~634 ms)
  wait_for_ap_started: FAIL — E (10645) SoftAP start timeout (10 s)
  HTTP server started / portal ready logs: FAIL (never emitted)
  Stable boot: FAIL — ESP_ERROR_CHECK(app_core_wifi_start) abort → reboot loop
  Phone GET / form: NOT EXERCISED (blocked by crash)
Logs:
  I (634) wifi_prov: SoftAP started
  I (644) esp_netif_lwip: DHCP server started on interface WIFI_AP_DEF with IP: 192.168.4.1
  E (10645) wifi_prov: SoftAP start timeout
  ESP_ERROR_CHECK failed: esp_err_t 0x107 (ESP_ERR_TIMEOUT) at app_core.c:72
  abort() was called at PC 0x40386711 on core 0
  (pattern repeats on every boot)
Conclusion:
  FAIL for Run 007 fix. Regression: Run 007 booted and served AP (blank HTTP); Run 008
  crashes before HTTP starts due to race in wait_for_ap_started(). Return to implementer.
  See feedback.md Entry 004.
```

### Run 009 — WIFI_PROVISIONING_ARCHITECTURE re-test (Run 008 race fix)

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_ARCHITECTURE (Run 008 race fix)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS state at boot: fresh / no saved credentials
Implementer fix:
  xEventGroupClearBits(WIFI_AP_STARTED_BIT) before esp_wifi_start();
  wait_for_ap_started() waits only (no clear after start)
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 22s -> /tmp/b06_hil_boot_log_run009.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdb290 bytes)
  Flash: PASS
  Run 008 reboot loop regression: PASS (fixed — 1 boot in 22s, no abort)
  wifi:mode softAP (not APSTA at boot): PASS
  wifi_prov SoftAP started: PASS (~645 ms)
  wifi_prov HTTP server started on port 80: PASS (~695 ms)
  wifi_prov HTTP handlers registered: PASS
  wifi_prov provisioning portal ready at http://192.168.4.1: PASS
  main_task returned stable: PASS
  Phone GET / setup form loads: FAIL (human: blank page; serial: 0× GET / from client)
  POST /provision, reboot, GPIO7: BLOCKED
Logs (Run 009c — phone connected during capture):
  /tmp/b06_hil_boot_log_run009c.txt
  I (705) wifi_prov: provisioning portal ready at http://192.168.4.1
  I (2445) wifi:station: 46:34:48:c3:db:03 join, AID=1
  (no "GET / from client" despite station join)
Conclusion:
  FAIL for WIFI_PROVISIONING_ARCHITECTURE portal acceptance.
  Run 008 race fix PASS; Run 007 original blank-page issue NOT fixed.
  HTTP server starts but client traffic does not reach handler. Return to implementer.
  See feedback.md Entry 005.
```

### Run 010 — WIFI_PROVISIONING_PORTAL_REACHABILITY

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_PORTAL_REACHABILITY
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS state at boot: fresh / no saved credentials
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 45s -> /tmp/b06_hil_boot_log_run010.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdb770 bytes, matches implementer handoff)
  Flash: PASS
  AP netif before esp_wifi_start: PASS
  Expected boot log sequence: PASS (all 6 implementer markers)
  Boot stable until HTTP request: PASS then FAIL (panic on GET /)
  app_core no ESP_ERROR_CHECK on wifi start: PASS (code review)
  Station joined diagnostic: PASS (mac=46:34:48:c3:db:03 aid=1)
  DHCP lease to client 192.168.4.2: PASS (fixes Run 009 L3 gap)
  GET /health: NOT EXERCISED (0× in log)
  GET / from client logged: PASS (2× — HTTP reaches handler)
  GET / completes HTML to phone: FAIL (stack protection fault → reboot)
  Human blank page: FAIL (consistent with crash mid-response)
Logs:
  I (585) wifi_prov: AP netif ip=192.168.4.1 gw=192.168.4.1 netmask=255.255.255.0
  I (1405) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.4.2
  I (36935) wifi_prov: GET / from client
  Guru Meditation Error: Core 0 panic'ed (Stack protection fault)
  (3 boots in 45s capture after repeated GET / crashes)
Conclusion:
  PARTIAL progress: architectural portal reachability contract verified (DHCP + handler entry).
  FAIL overall: httpd handler stack overflow on GET / prevents HTML delivery.
  Return to implementer. See feedback.md Entry 006.
```

### Run 011 — WIFI_PROVISIONING_HTTP_STACK_SAFETY

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_HTTP_STACK_SAFETY
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS state at boot: fresh / no saved credentials
Implementer fix:
  - httpd stack_size 8192; HTML via send_chunk (no html[2048] on stack)
  - static s_form_body; in-place form parser
  - completion logs: GET / response sent, etc.
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 50s -> /tmp/b06_hil_boot_log_run011.txt
  Static review: no html[2048], stack_size 8192, send_chunk pages
Result:
  Build: PASS (b06_hil_firmware.bin 0xdc430 bytes, matches implementer handoff)
  Flash: PASS
  Portal startup sequence: PASS
  Boot stable (single boot, 50s): PASS
  Stack/panic regression Run 010: PASS (no Guru / stack fault)
  Station joined: PASS
  GET / from client: PASS (1×)
  GET / response sent: PASS (1×)
  Human GET / form visible: PASS (operator confirmed)
  GET /health: NOT EXERCISED
  POST /provision from client handler: FAIL (blocked at httpd parse)
  POST /provision human: FAIL — "Header fields are too long" (HTTP 431)
  POST /provision success/failure page: NOT REACHED
  Reboot, GPIO7: NOT EXERCISED
Logs:
  I (8156) wifi_prov: GET / from client
  I (8156) wifi_prov: GET / response sent
  W httpd_parse: parse_block: request URI/header too long
  W httpd_txrx: httpd_resp_send_err: 431 Request Header Fields Too Large - Header fields are too long
  (no POST /provision from client; sdkconfig CONFIG_HTTPD_MAX_REQ_HDR_LEN=512)
Conclusion:
  PARTIAL PASS. HTTP stack safety + GET / criteria PASS (serial + human).
  POST /provision FAIL: phone browser headers exceed httpd 512-byte limit.
  Return to implementer to raise CONFIG_HTTPD_MAX_REQ_HDR_LEN; re-test POST path.
  See feedback.md Entry 007.
```

### Run 021 — WIFI_FACTORY_RESET_RUNTIME

```text
Date: 2026-06-23
Firmware: commit 0aeb6a541928167223fc95bb2e1952003c318eeb-dirty
Handoff: WIFI_FACTORY_RESET_RUNTIME (docs/wifi_factory_reset_architecture.md)
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, GPIO7 factory reset (active-low)
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Static audit FACTORY_RESET_HOLD_MS=10000, wifi_fr_mon, factory_reset_to_portal
  Phase A: 70s serial portal mode (operator hold GPIO7) -> run021_factory_reset.txt
  Phase B: fake NVS inject + connect cycle + 70s serial (no reset press) -> run021_cycle_*.txt
Result:
  Build: PASS (0xde220)
  Flash: PASS
  FACTORY_RESET_HOLD_MS 10000: PASS (code)
  wifi_provisioning_factory_reset_to_portal API + abort flag: PASS (code)
  wifi_fr_mon monitor task: PASS (code + binary strings)
  Phase A runtime reset from portal (idempotent 2x): PASS
    factory reset requested (runtime)
    factory reset abort connect cycle / stopping wifi
    factory reset complete portal_active=true
    provisioning portal ready (after each reset)
  Phase B connect cycle + runtime reset: PASS (operator session, factory_reset_user_test.txt)
    abort connect cycle during fake-creds cycle; portal restored
  Runtime reset from portal (idempotent): PASS (serial Phase A + operator session)
  Portal QR + setup OLED after reset: PASS (human Entry 024)
  Reprovision vitriolina after reset: PASS (human Entry 024)
  Boot hold >=10s at cold boot: NOT EXERCISED
  Boot short press <10s no erase: NOT EXERCISED
  Runtime reset from WIFI OK while connected only: NOT EXERCISED (reset from cycle/portal)
Logs (operator connect-cycle reset):
  W app_core_wifi: factory reset requested (runtime)
  I wifi_prov: factory reset abort connect cycle
  I app_core_wifi: factory reset complete portal_active=true
Conclusion:
  PASS for WIFI_FACTORY_RESET_RUNTIME operator-critical path: GPIO7 10s erases NVS,
  aborts connect cycle, portal + QR OK, reprovision vitriolina OK (Entries 024).
  Deferred: boot-hold, short-press negative, reset-only-from-stable-WIFI-OK screen.
```

### Run 020 — WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2

```text
Date: 2026-06-23
Firmware: commit 0aeb6a541928167223fc95bb2e1952003c318eeb-dirty
Handoff: WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS phases:
  - Erased (operator request) -> portal provision vitriolina (human PASS Entry 021)
  - Operator-authorized inject: NONEXISTENT_AP_XYZ / badpass12345
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Static audit LED v2 table, no SAVED_FAILURE_LOCKED emit, no HOLD RESET in bin
  Phase A: NVS erase + portal boot -> phaseA_fresh.txt
  Operator portal POST vitriolina (human)
  Phase B: authorized fake NVS inject + serial 100s -> phaseB_cycle.txt
Result:
  Build: PASS (0xddc10)
  Flash: PASS
  LED v2 mapping ON/slow/fast/off: PASS (code review)
  No boot lock / no HOLD RESET in firmware: PASS
  Phase A fresh NVS portal (no connect cycle): PASS
  Portal provision + LED solid ON no creds: PASS (human Entry 021)
  Phase B fake creds connect cycle serial:
    STA mode, policy log, 5 attempts, alert 15s, 2nd+ round: PASS
    3 rounds / 15 attempts / 3 alerts in 100s capture: PASS
    no saved boot failed attempts=5: PASS
  OLED/LED slow -> fast 15s -> slow during outage: PASS (human Entry 022)
  OLED stays CONNECTING, no HOLD RESET: PASS (human Entry 022)
  AP restore -> CONNECT_RESTORED: NOT EXERCISED
Logs (Phase B):
  I (1642) wifi_prov: connect cycle policy round_attempts=5 ...
  I (1642) wifi_prov: connect cycle round start source=boot
  I (23212) wifi_prov: connect cycle alert_phase_ms=15000 source=boot
  I (38222) wifi_prov: connect cycle round start source=boot
Conclusion:
  PASS for WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2 boot + outage cycle criteria (1–4, 6–7).
  Portal + no-creds LED ON (Entry 021); fake-creds slow/fast infinite cycle (Entry 022).
  AP restore (criterion 4) and factory reset (criterion 5) not exercised this run.
  Device has fake NVS; erase + re-provision vitriolina to restore normal operation.
```

### Run 019 — ERROR_LED_RUNTIME_LINK_STATUS + WIFI_RUNTIME_RECONNECT

```text
Date: 2026-06-23
Firmware: commit 0aeb6a541928167223fc95bb2e1952003c318eeb-dirty
Handoffs: ERROR_LED_RUNTIME_LINK_STATUS, WIFI_RUNTIME_RECONNECT
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS: invalid saved creds (NONEXISTENT_AP_XYZ) from prior tester injection
Implementer changes:
  - WIFI_PROV_EVENT_LINK_STATUS_CHANGED (LED-only)
  - publish_link_status / handle_runtime_link_loss hooks
  - WIFI_PROV_EVENT_RUNTIME_RECONNECTING / RUNTIME_RESTORED
  - runtime_reconnect_task unbounded backoff; run_sta_connect_attempt shared
  - app_core_wifi: LINK_STATUS_CHANGED no OLED; RUNTIME_* OLED handlers
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Static audits + strings grep runtime log markers
  Serial boot lock regression 38s -> /tmp/b06_hil_boot_log_run019_lock_regression.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xde060, matches implementer)
  Flash: PASS
  Module boundaries: PASS
  New events in wifi_provisioning.h: PASS
  app_core_wifi adapter (LINK_STATUS_CHANGED break, RUNTIME OLED): PASS
  Binary runtime serial markers present: PASS
  Boot lock bad creds still 5 attempts -> SAVED_FAILURE_LOCKED: PASS
  No source=runtime / runtime reconnect during boot lock: PASS
  Connected -> AP off -> LED ON + RUNTIME_RECONNECTING: NOT EXERCISED
  AP restore -> RUNTIME_RESTORED + LED off: NOT EXERCISED
  Long outage no HOLD RESET / boot lock: NOT EXERCISED
  ERROR_LED runtime loss/restore visual: NOT EXERCISED
Logs (lock regression):
  E (...) wifi_prov: saved boot failed attempts=5 last_reason=201
  (no runtime reconnect attempt / source=runtime in capture)
Conclusion:
  PARTIAL PASS. Build, static, and boot-lock regression (WIFI_RUNTIME_RECONNECT
  criterion 4) satisfied. Runtime reconnect + runtime LED criteria require valid NVS
  and operator AP power-cycle on vitriolina network (host cannot control remote AP).
  Device remains in boot-locked state; erase NVS + re-provision before runtime test.
```

### Run 018 — ERROR_LED_WIFI_LINK_STATUS

```text
Date: 2026-06-23
Firmware: commit 0aeb6a541928167223fc95bb2e1952003c318eeb-dirty
Handoff: ERROR_LED_WIFI_LINK_STATUS
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, MAC 8c:d0:b2:a9:24:cd, OLED @ 0x3C
Implementer changes:
  - error_led component: GPIO8 active-low, esp_timer blink (500/500 slow, 125/125 fast)
  - wifi_link_status_t + link_status on wifi_prov_event_info_t
  - app_core_wifi apply_wifi_link_status_led + boot gap
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Static audit: module boundaries, timing constants, init order
  Phase B: saved boot serial 22s -> /tmp/b06_hil_boot_log_run018_phaseB_saved.txt
  Phase A: esptool erase_region 0x9000 0x6000 + serial 18s -> phaseA_fresh.txt
  Phase C: nvs_partition_gen bad creds + write_flash 0x9000 + serial 55s -> phaseC_locked.txt
  Post-test: NVS erase (device left in portal mode)
Result:
  Build: PASS (b06_hil_firmware.bin 0xdd7d0, matches implementer handoff)
  Flash: PASS
  error_led no WiFi/provisioning headers: PASS
  wifi_provisioning no error_led references: PASS
  error_led_init before app_core_wifi_start: PASS (app_core.c:71 before :73)
  Timing 500/500 slow, 125/125 fast in error_led.c only: PASS
  GPIO8 output configured at boot: PASS — I (472) gpio: GPIO[8]| OutputEn: 1
  Phase B saved boot CONNECTING -> CONNECTED: PASS
    STA got IP 192.168.80.184 attempt 3/5; LED inferred ON then OFF
  Phase A fresh NVS portal UNPROVISIONED: PASS
    SoftAP HIL-06-24CE, portal ready; no saved boot; LED inferred SLOW_BLINK
  Phase C bad creds SAVED_FAILURE_LOCKED: PASS
    saved boot failed attempts=5 last_reason=201; LED inferred FAST_BLINK after lock
  Boot stable all phases: PASS (no panic/abort)
  POST SUBMITTED_SUCCESS LED off: NOT EXERCISED (saved-boot path only)
  LED physical slow blink (fresh NVS / portal): PASS (human — Entry 017)
  LED physical solid ON while connecting (saved boot): PASS (human — Entry 018)
  LED physical off after connect (saved boot): PASS (human — Entry 019)
  LED physical fast blink after lock (invalid saved creds): PASS (human — Entry 020)
Logs (Phase B):
  I (472) gpio: GPIO[8]| InputEn: 0| OutputEn: 1| ...
  I (656) wifi_prov: saved boot connect policy attempts=5 ...
  I (10636) wifi_prov: saved boot connected attempt=3/5 ip=192.168.80.184
Logs (Phase A):
  I (598) wifi_prov: provisioning AP SSID generated ssid=HIL-06-24CE
  I (708) wifi_prov: provisioning portal ready at http://192.168.4.1
Logs (Phase C):
  E (23202) wifi_prov: saved boot failed attempts=5 last_reason=201
Conclusion:
  PASS for ERROR_LED_WIFI_LINK_STATUS (serial, static, human visual criteria 1–4).
  Entries 017–020: slow blink, solid ON, off after connect, fast blink after lock.
  POST SUBMITTED_SUCCESS not exercised; same CONNECTED → OFF as SAVED_SUCCESS.
  Device may be in locked state; recover via procedures.md NVS erase + re-provision.
```

### Run 017 — OLED_WIFI_CONNECTED_STATUS

```text
Date: 2026-06-24
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: OLED_WIFI_CONNECTED_STATUS
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS: saved credentials (not erased; operator session from Run 015)
Implementer changes:
  - wifi_prov_event_info_t: sta_ipv4, sta_mac on success events
  - prepare_sta_success_event_payload() + emit_sta_success_event()
  - app_core show_wifi_connected_display(): FULL_FOUR_LINES
    WIFI OK / IP / mac[0..7] / mac[9..16]
  - SUBMITTED_SUCCESS and SAVED_SUCCESS use connected screen (not legacy WIFI/OK 2-line)
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  Reboot serial -> /tmp/b06_hil_boot_log_run017.txt
Result:
  Build: PASS (0xdd250)
  Flash: PASS
  Static/code audit connected display path: PASS
  Saved boot connect + GOT_IP: PASS — 192.168.80.184 attempt 3/5
  Expected OLED after SAVED_SUCCESS:
    Line0 WIFI OK
    Line1 192.168.80.184
    Line2 8C:D0:B2
    Line3 A9:24:CD
  OLED physical + match router IP/MAC: PASS (human — Entry 016)
  POST SUBMITTED_SUCCESS connected screen: NOT EXERCISED (saved boot + operator confirm)
Logs:
  I (7748) wifi_prov: STA got IP ip=192.168.80.184 source=saved attempt=3/5
  I (7748) wifi_prov: saved boot connected attempt=3/5 ip=192.168.80.184
Conclusion:
  PASS for OLED_WIFI_CONNECTED_STATUS. Human confirmed 4-line connected screen.
  See feedback Entry 016.
```

### Run 016 — OLED_PROVISIONING_SETUP_UX

```text
Date: 2026-06-24
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: OLED_PROVISIONING_SETUP_UX
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS: erased via host (fresh provisioning)
Implementer changes:
  - wifi_provisioning emits AP_STARTED/PORTAL_READY with s_ap_ssid in event info
  - app_core_wifi show_provisioning_setup_display(): 4-line right panel
    "1 JOIN", ssid[0..6], ssid[7..], "2 SCAN QR"
  - Fallback WIFI/SETUP when ssid NULL or strlen < 8
Commands:
  idf.py build && idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  Serial -> /tmp/b06_hil_boot_log_run016.txt
  strings firmware.bin | grep JOIN/SCAN
Result:
  Build: PASS (0xdd060)
  Flash + NVS erase: PASS
  Portal boot HIL-06-24CE: PASS
  wifi_provisioning boundary (no display headers): PASS
  Static: PROV_SETUP strings in binary: PASS (1 JOIN, 2 SCAN QR)
  Code audit 4-line split for HIL-06-24CE (len 11): PASS
    Expected OLED right: 1 JOIN / HIL-06- / 24CE / 2 SCAN QR
    Expected OLED left: QR http://192.168.4.1
  OLED physical layout: PASS (human — Entry 015)
  QR camera scan + phone join AP: PASS (implied by prior Run 015 POST success)
Logs:
  I (598) wifi_prov: provisioning AP SSID generated ssid=HIL-06-24CE
  I (708) wifi_prov: provisioning portal ready at http://192.168.4.1
Conclusion:
  PASS for OLED_PROVISIONING_SETUP_UX. Human confirmed provisioning OLED layout.
  See feedback Entry 015.
```

### Run 015 — PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID

```text
Date: 2026-06-24
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C, SoftAP MAC 8c:d0:b2:a9:24:ce
NVS: erased via host (fresh provisioning)
Implementer changes:
  - Dynamic AP SSID HIL-<board>-<MAC4> via BOARD_HIL_NUMBER_STRING "06"
  - Display visual demo retired from product boot (no DEMO_STEP)
  - Removed CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO / app_core_display_demo
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  strings build/b06_hil_firmware.bin | grep forbidden
  Serial capture -> /tmp/b06_hil_boot_log_run015.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdcfb0)
  Flash: PASS
  No b06_hil_setup in binary: PASS
  No DEMO_STEP in binary or serial: PASS
  SSID format HIL-06-[0-9A-F]{4}: PASS — HIL-06-24CE matches MAC bytes 24:ce
  Logs provisioning AP SSID generated + starting SoftAP: PASS
  Portal ready http://192.168.4.1: PASS
  HTTP GET/POST from phone: NOT EXERCISED (host ethernet only)
  Phone observes HIL-06-24CE on WiFi list: PASS (human — Entry 014)
  POST form submit + home WiFi connect: PASS (human — Entry 014)
  Reboot STA got IP after POST: PASS — 192.168.80.184 attempt=3/5 (serial)
Logs:
  I (598) wifi_prov: provisioning AP SSID generated ssid=HIL-06-24CE
  I (608) wifi_prov: starting SoftAP ssid=HIL-06-24CE mode=AP
  I (658) wifi:mode : softAP (8c:d0:b2:a9:24:ce)
  I (708) wifi_prov: provisioning portal ready at http://192.168.4.1
  (post-POST reboot /tmp/b06_hil_boot_log_run015_reboot.txt)
  I (10628) wifi_prov: STA got IP ip=192.168.80.184 source=saved attempt=3/5
Conclusion:
  PASS for PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID including human POST on new SSID.
  Dynamic AP + portal + saved boot after provision validated. See feedback Entry 014.
```

### Run 014 — WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY

```text
Date: 2026-06-24
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS: saved credentials from prior Run 013 POST (SSID vitriolina, not erased)
Implementer fix:
  - WPA2/WPA3 auth profile, PMF capable, SAE PWE both
  - saved boot settle 1000 ms, 5 attempts, backoff [0,1k,3k,5k,8k]s
  - early disconnect ends attempt; locked only after 5 failures
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  10 consecutive cold reboot sweep (esptool run) -> /tmp/b06_hil_run014_sweep_summary.json
  Per-boot logs -> /tmp/b06_hil_boot_log_run014_boot01.txt .. boot10.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdd160, matches implementer handoff)
  Flash: PASS
  auth_profile log on saved boot: PASS (all 10 boots)
  saved boot connect policy log: PASS (all 10 boots)
  10-boot cold reboot sweep: PASS — 10/10 STA got IP source=saved
  Locked failures (saved boot failed attempts=5): PASS — 0/10
  Recovery from reason 202: PASS — attempt 1 failed 202 on all boots; success on attempt 3/5
  IP assigned: 192.168.80.184 (consistent across boots)
  GPIO7 factory reset: NOT EXERCISED
Sweep summary (/tmp/b06_hil_run014_sweep_summary.json):
  pass=10 locked=0 timeout=0
  Every boot: attempt=3/5, disc202=1 (recovered)
Sample logs (boot01):
  I (638) wifi_prov: STA config auth_profile=wpa2_wpa3_personal pmf_capable=true pmf_required=false
  I (648) wifi_prov: saved boot connect policy attempts=5 per_attempt_timeout_ms=12000
  I (1648) wifi_prov: saved boot attempt 1/5 start
  W (2088) wifi_prov: STA disconnected reason=202 source=saved attempt=1/5
  W (2098) wifi_prov: saved boot attempt 1/5 failed reason=202 backoff_ms=0
  I (5618) wifi_prov: saved boot attempt 3/5 start
  I (7648) wifi_prov: STA got IP ip=192.168.80.184 source=saved attempt=3/5
  I (7648) wifi_prov: saved boot connected attempt=3/5 ip=192.168.80.184
Conclusion:
  PASS for WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY. Run 013 intermittent reboot
  issue resolved (was 2/5 pass without retry policy). Architect acceptance criterion
  10/10 cold boots met. GPIO7 factory reset still pending.
```

### Run 013 — WIFI_PROVISIONING_POST_SUCCESS_COMMIT

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_POST_SUCCESS_COMMIT
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
NVS: erased via host (esptool erase_region 0x9000 0x6000) per procedures.md
Implementer fix (code review + build):
  - SUBMITTED_SUCCESS after POST /provision success response sent
  - credentials rollback if success page send fails
  - deferred teardown 2000 ms with milestone logs
  - STA got IP ip=<ipv4> source=submitted|saved
  - threshold.authmode = WIFI_AUTH_WPA2_PSK
  - fresh STA attempt: disconnect + clear event bits before connect
Commands:
  idf.py build
  idf.py -p /dev/ttyACM0 flash
  esptool.py erase_region 0x9000 0x6000
  Serial capture 15s -> /tmp/b06_hil_boot_log_run013_full.txt
  Operator POST (human) + reboot capture 35s -> /tmp/b06_hil_boot_log_run013_reboot.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdcb20, matches implementer handoff)
  Flash: PASS
  CONFIG_HTTPD_MAX_REQ_HDR_LEN: PASS (2048)
  Success commit static audit: PASS
  Portal boot after NVS erase: PASS
  POST browser success page (human): PASS — "WiFi connected. You can close this page."
  AP b06_hil_setup gone after POST (human): PASS
  OLED WIFI OK after POST (human): PASS
  POST serial success sequence during submit: NOT CAPTURED (no monitor during form)
  Reboot STA-only boot (no AP): PASS (when connect succeeds)
  Reboot STA got IP source=saved: INTERMITTENT — PASS in 2/5 cold boots; 3/5 auth fail
    reason=202 → OLED WIFI FAILED after ~30 s (Entry 013)
  STA ping from operator LAN (human): PASS when boot connected (192.168.80.184)
  Saved-boot 5-reboot sweep: FAIL reliability (3/5 fail, 2/5 pass)
  GPIO7 factory reset: NOT EXERCISED
Logs (/tmp/b06_hil_boot_log_run013_reboot.txt — successful reboot):
  I (638) wifi:mode : sta (8c:d0:b2:a9:24:cd)
  I (1868) wifi:connected with vitriolina, aid = 16, channel 3, 40U
  I (5898) wifi_prov: STA got IP ip=192.168.80.184 source=saved
Logs (/tmp/b06_hil_reboot_sweep_run013.txt — sweep summary):
  Boots 1,3,5: auth -> init, reason=202, no IP
  Boots 2,4: STA got IP 192.168.80.184 source=saved
Conclusion:
  PASS for WIFI_PROVISIONING_POST_SUCCESS_COMMIT (POST path, human UX, Run 012 fix).
  QUALIFIED / FAIL for saved-credential cold-boot reliability (intermittent auth).
  Architect review required: see feedback Entry 013 root cause + recommendations;
  tester/handoff.md Architect review block; architect/handoff.md
  TESTER_RETURN_SAVED_BOOT_RELIABILITY.
  GPIO7 factory reset still not tested.
  See feedback.md Entries 011–013.
```

### Run 012 — WIFI_PROVISIONING_HTTP_HEADER_BUDGET

```text
Date: 2026-06-23
Firmware: commit c31c917d6503d89b3082052e0cfa4efa54f42324-dirty
Handoff: WIFI_PROVISIONING_HTTP_HEADER_BUDGET
ESP-IDF: v5.3.4
Hardware: ESP32-C3 SuperMini on /dev/ttyACM0, OLED @ 0x3C
Implementer fix:
  - sdkconfig.defaults CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048
  - POST milestone logs: parsed form, attempting STA ssid=...
Commands:
  idf.py reconfigure && idf.py build
  idf.py -p /dev/ttyACM0 flash
  Serial capture 60s -> /tmp/b06_hil_boot_log_run012.txt
Result:
  Build: PASS (b06_hil_firmware.bin 0xdc5a0, matches handoff)
  CONFIG_HTTPD_MAX_REQ_HDR_LEN in sdkconfig.h: PASS (2048)
  Flash: PASS
  Portal boot sequence: PASS
  DHCP 192.168.4.2: PASS
  GET / + response sent: PASS (no panic)
  HTTP 431 / Header fields too long in log: PASS (0 real occurrences; note: timestamp
    I (43186) is not an HTTP 431 error)
  POST /provision handler milestones: NOT EXERCISED (no form submit during capture)
  POST human outcome: PENDING — operator must submit form on flashed firmware
Logs:
  I (42446) esp_netif_lwip: DHCP server assigned IP to a client, IP is: 192.168.4.2
  I (53526) wifi_prov: GET / from client
  I (53526) wifi_prov: GET / response sent
Conclusion:
  PARTIAL PASS. Header budget fix present in build; GET / regression clear.
  POST /provision requires operator submit to confirm no 431 and handler logs
  (POST /provision from client → parsed form → attempting STA → success/failure).
```
