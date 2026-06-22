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

I2C pins, OLED address candidates, `INA219` addresses, error LED polarity, and
factory reset switch polarity are recorded in
`docs/esp32_c3_supermini_connections.md`. Generic shared I2C bus structure and
incremental concurrency phases are recorded in
`docs/i2c_bus_architecture.md`. QR encoder rules and `http://IPv4` payload
profile are recorded in `docs/qr_encoder_interface.md`. Display delivery from
producers to the OLED stack is recorded in
`docs/display_delivery_contract.md`. The implementer must still keep physical
OLED driver/controller selection isolated until confirmed.

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
  - Confirm components/setup_url/ and vendor path at implementation time.
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
  - Ensure no module other than app_core includes display_controller.h in v1.
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
Non-goals:
  - esp_event transport for display instructions in v1.
  - Instruction source direct calls to display_controller_* or display_set_*.
  - Polling for pending instructions.
  - Separate QR delivery channel or display/network coupling.
  - ISR-to-display calls.
Acceptance criteria:
  - Grep audit: only app_core includes display_controller.h in v1 firmware.
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
