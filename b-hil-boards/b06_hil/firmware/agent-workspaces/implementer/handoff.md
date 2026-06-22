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
- `DISPLAY_VISUAL_DEMO_PROTOCOL`: **documented** (protocol + handoff); **firmware pending implementer**.
- Physical OLED controller for v1: **SSD1306** (128x64 I2C). No SH1106 work unless
  hardware changes.

Normative docs:

- `docs/display_delivery_contract.md`
- `docs/qr_encoder_interface.md`
- `docs/oled_text_display_interface.md`
- `docs/test_strategy.md`
- `docs/display_visual_demo_protocol.md`

## Display Visual Demo (mandatory for display handoffs)

Copy this block into every display-related handoff completion. Do not set
`Ready for tester: Yes` without a filled manifest.

```text
Handoff ID:
Kconfig: CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
Total hold (s):
Commands: idf.py build && idf.py flash && idf.py monitor
Observation window (s): total hold + ~5

Step | name= | API | Payload / lines | Hold (s) | Human expects on OLED
1    |       |     |                 |          |
```

## Next Tasks (simple order)

Tasks 1–5 below are **complete**. Do not start `I2C_BUS_PHASE3` unless a new
architect handoff activates it.

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

## DISPLAY_VISUAL_DEMO_PROTOCOL (pending — implementer)

Architect handoff and `docs/display_visual_demo_protocol.md` are ready. **Do not**
mark complete until firmware exists.

**Implementer tasks:**

1. Add `components/app_core/Kconfig` → `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` (default `y`).
2. Add `app_core_display_demo.c` / `.h` with `app_core_run_visual_demo()` and smoke helper.
3. Wire `app_core_start()` per protocol (`#if CONFIG_...`).
4. Set `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y` in `sdkconfig.defaults`.
5. Fill **Display Visual Demo** manifest below; set `Ready for tester: Yes` only after build.

### Display Visual Demo (target manifest — copy when implemented)

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

Ready for tester: **No** (firmware not implemented).

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