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
