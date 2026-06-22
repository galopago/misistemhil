# Architecture Decisions

Record decisions that change the shape of the firmware here.

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Template

```text
Date:
Decision:
Context:
Alternatives considered:
Implementation contract:
Expected behavior:
Non-goals:
Consequences:
Affected files:
Validation expectations:
Open questions:
```

## Initial Decisions

- The firmware is organized as a conventional ESP-IDF project.
- `main/` remains the system entry point and delegates to components.
- `components/board/` centralizes board-specific details.
- Pins remain pending until confirmed against the schematic.

## 2026-06-20 — Mandatory Role Boundary Check

```text
Date: 2026-06-20
Decision: Every agent must verify role ownership before editing files or running
  commands, and must ask the human when a request belongs to another role.
Context: An architect session implemented firmware and attempted builds despite
  role ownership defined in AGENTS.md and methodology.md.
Alternatives considered:
  - Rely on AGENTS.md ownership list only.
  - Add role checks only to architect documentation.
Implementation contract:
  - Add Role Boundary Check to AGENTS.md and docs/methodology.md.
  - Add ROLE.md guides under architect/, implementer/, and tester/ workspaces.
  - Each agent stops and questions the human when a task crosses role boundaries.
Expected behavior:
  - Plans that mix documentation and code are split by role at execution time.
  - Generic verbs such as "implement" do not override ownership rules.
Non-goals:
  - Changing the three-role workflow itself.
Consequences:
  - Agents may ask the human to confirm or redirect more often.
  - Reduced cross-role edits and conflicting handoffs.
Affected files:
  - firmware/AGENTS.md
  - firmware/docs/methodology.md
  - firmware/agent-workspaces/architect/ROLE.md
  - firmware/agent-workspaces/implementer/ROLE.md
  - firmware/agent-workspaces/tester/ROLE.md
Validation expectations:
  - Each role guide lists owned paths, forbidden paths, and stop conditions.
Open questions:
  - None.
```

## 2026-06-20 — Portable I2C Bus Contract

```text
Date: 2026-06-20
Decision: Define I2C bus architecture as a portable logical contract first, with
  ESP-IDF as the current platform profile.
Context: The bus must produce similar implementations across LLM implementers and
  port to other microcontrollers without redesigning device drivers.
Alternatives considered:
  - ESP-IDF-only documentation tied to gpio_num_t and i2c_master types in the
    public contract.
  - Fully abstract interface with runtime plugin loading.
Implementation contract:
  - docs/i2c_bus_architecture.md defines portable API, constants, algorithms,
    startup order, forbidden dependencies, and ESP-IDF binding separately.
Expected behavior:
  - i2c_bus remains device-agnostic and unchanged in display-only projects.
  - Only the platform port inside i2c_bus changes when moving to another MCU.
Non-goals:
  - Multi-bus support and 10-bit addressing in the base version.
Consequences:
  - Implementers must follow normative algorithms verbatim.
  - Port work is localized to i2c_bus platform code and board pin headers.
Affected files:
  - docs/i2c_bus_architecture.md
  - docs/architecture.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Independent implementers produce the same module layout and init order.
Open questions:
  - None.
```

## 2026-06-20 — Incremental I2C Concurrency Phases

```text
Date: 2026-06-20
Decision: Add phased concurrency architecture for shared I2C: direct sync,
  transaction executor, priority broker, optional observability.
Context: b06_hil will have concurrent display and INA219 traffic on one bus.
  Platform driver serialization alone does not define policy or protect logical
  transaction boundaries.
Alternatives considered:
  - Rely only on ESP-IDF master-driver serialization.
  - Implement a broker immediately in the first I2C change set.
Implementation contract:
  - docs/i2c_bus_architecture.md defines phases, APIs, priorities, anti-patterns,
    and driver migration rules.
  - docs/test_strategy.md defines validation per phase.
  - Handoff I2C_BUS_CONCURRENCY authorizes documentation; implementer uses
    I2C_BUS_PHASE1..PHASE4 one at a time.
Expected behavior:
  - Phase 2 introduces i2c_bus_transceive as the only driver entry point.
  - Phase 3 introduces i2c_broker_submit for application tasks.
  - b06_hil priority table lives in product config, not generic bus code.
Non-goals:
  - Implementing all phases at once.
  - Per-driver mutex ownership.
Consequences:
  - Future INA219 and OLED drivers can grow onto the same bus safely.
  - Testers validate only the active phase unless re-test is authorized.
Affected files:
  - docs/i2c_bus_architecture.md
  - docs/test_strategy.md
  - docs/architecture.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Phase 3 includes 60 s contention test without deadlock.
Open questions:
  - None.
```

## 2026-06-20 — QR Encoder Product Profile

```text
Date: 2026-06-20
Decision: Define QR encoder around http://IPv4 setup URLs, Nayuki qrcodegen (MIT),
  external payload ownership, and shared setup_url validation.
Context: Setup screens need scannable local URLs without coupling display to
  network stack or restrictive encoder licenses.
Alternatives considered:
  - libqrencode (LGPL) — rejected for license restrictiveness.
  - Display-owned URL formatting from netif — rejected; payload arrives with instruction.
  - Validation only in external module — rejected; user chose shared setup_url.
Implementation contract:
  - docs/qr_encoder_interface.md is normative for encoder behavior.
  - Payload profile v1: http://aaa.bbb.ccc.ddd only, QR version 1 or 2, byte mode,
    EC LOW.
  - Library: Nayuki qrcodegen, MIT, under components/qr_encoder/vendor/.
  - setup_url shared helpers for format and validate.
Expected behavior:
  - display_controller_show_qr_setup receives externally supplied payload.
  - display_qr_generate produces 21x21 or 25x25 matrices for canonical URLs.
Non-goals:
  - https, hostname, port, path, IPv6 in v1 product profile.
Consequences:
  - Open question on encoder library closed in OLED handoff.
  - Implementer handoff QR_ENCODER_INTERFACE authorizes integration work.
Affected files:
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/test_strategy.md
  - docs/architecture.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Tests in docs/test_strategy.md QR Encoder Criteria section.
Open questions:
  - Instruction source identity for display instructions.
```

## 2026-06-20 — Display Delivery via app_core Orchestration

```text
Date: 2026-06-20
Decision: v1 display instructions use centralized app_core orchestration. Instruction
  sources call app_core via callback; only app_core calls display_controller_*
  directly. Text and QR share the same path. No display/network coupling.
Context: User selected centralized orchestration with callback transport (not esp_event).
Implementation contract:
  - docs/display_delivery_contract.md is normative.
Expected behavior:
  - instruction source --callback--> app_core --display_controller_*--> display_task --> OLED
Non-goals:
  - esp_event display transport in v1.
  - Instruction source calls display_controller_* or display_set_*.
  - Application-level display command queue.
  - Polling for pending instructions.
  - Display architecture references to WiFi/network.
Affected files:
  - docs/display_delivery_contract.md
  - docs/architecture.md
  - docs/oled_text_display_interface.md
  - docs/qr_encoder_interface.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - docs/test_strategy.md Display Delivery Criteria section.
Open questions:
  - Instruction source identity for display instructions.
```

## 2026-06-20 — Callback Transport for Display Instructions (v1)

```text
Date: 2026-06-20
Decision: v1 display instructions use callback (or internal app_core function calls)
  into app_core. esp_event is not the v1 transport.
Context: User expects centralized orchestration in app_core; callback keeps the
  path simple and debuggable.
Implementation contract:
  - docs/display_delivery_contract.md section Selected transport (v1): callback.
  - app_core exposes app_core_display_show_template and app_core_display_show_qr_setup
    (names illustrative) and calls display_controller_* internally.
Non-goals:
  - esp_event posting for display instructions in v1 without a new handoff.
Affected files:
  - docs/display_delivery_contract.md
  - docs/architecture.md
  - docs/qr_encoder_interface.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Tests call app_core callbacks; grep shows no esp_event display handlers in v1.
Open questions:
  - None.
```

## 2026-06-20 — Display Decoupled from WiFi/Network

```text
Date: 2026-06-20
Decision: Display architecture MUST NOT reference, assume, or depend on WiFi or
  network subsystems. QR instructions use the same delivery path as text instructions.
Context: User clarified that whatever draws QR is the same as what draws text; any
  connectivity stack must remain fully decoupled from display docs and code.
Implementation contract:
  - docs/display_delivery_contract.md Core Rule: QR Equals Text (Delivery).
  - docs/qr_encoder_interface.md and docs/architecture.md remove network coupling.
  - setup_url validates payload shape only; it does not link display to network.
Non-goals:
  - URL producer, network module, or WiFi-triggered display paths in architecture.
  - Separate QR delivery channel.
Affected files:
  - docs/display_delivery_contract.md
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Grep: no WiFi/network headers in display component; display docs omit network coupling.
Open questions:
  - None.
```

## 2026-06-20 — Unified QR Refresh Policy

```text
Date: 2026-06-20
Decision: Refreshing a QR uses the same display update path as refreshing text or
  any other layout or content change. No QR-specific debounce or partial update.
Context: User clarified that QR refresh is not a separate concern from normal
  display refresh.
Implementation contract:
  - docs/qr_encoder_interface.md section QR Refresh Policy.
  - docs/oled_text_display_interface.md Refresh Policy includes QR payload changes.
Non-goals:
  - QR-only debounce, partial framebuffer updates, or special refresh queues.
Affected files:
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Changing QR payload via a new layout update behaves like any other SET_CONTENT.
Open questions:
  - None.
```

## 2026-06-20 — Instruction-Driven QR (No Wait For IP)

```text
Date: 2026-06-20
Decision: QR screens appear only when an explicit draw-QR instruction arrives with
  a URL payload. The display does not wait, poll, or show placeholders for IP
  availability.
Context: User clarified that absence of a QR instruction is normal; there is no
  need to anticipate "waiting for valid IP" UX in the display contract.
Implementation contract:
  - docs/qr_encoder_interface.md section Instruction-Driven QR Display.
  - DisplayController applies instructions; it does not observe connectivity state.
Non-goals:
  - WAITING screens, polling, or implicit setup states in the display stack.
  - References to WiFi/network in display architecture.
Affected files:
  - docs/qr_encoder_interface.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Text-only operation with no QR instruction is valid default behavior.
Open questions:
  - None.
```

## 2026-06-20 — No Display Power Management in v1

```text
Date: 2026-06-20
Decision: OLED sleep, dimming, and display power-saving policies are out of scope
  for v1. The product is occasional-use, not 24/7 continuous operation.
Context: User confirmed energy saving for the display is not needed at this stage.
Implementation contract:
  - docs/oled_text_display_interface.md section Display Power Policy (v1 Product).
  - Refresh rate limiting remains a performance rule, not power management.
Expected behavior:
  - Panel may stay on with last content for a power-on session; no sleep/wake UI.
Non-goals:
  - OLED sleep, dim, panel off, wake-on-event display policies in v1 architecture.
Affected files:
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - No tests shall require display power-management behavior in v1.
Open questions:
  - None.
```

## 2026-06-20 — Read-Only Informational Display (No Menu v1)

```text
Date: 2026-06-20
Decision: v1 OLED is one-way informational output only. No menus, navigation,
  selection UI, cursors, or on-display data entry in the architecture.
Context: User confirmed the screen shows status and information; it is not an input
  surface for the user in this product phase.
Implementation contract:
  - docs/oled_text_display_interface.md section Display Interaction Model (v1 Product).
  - DisplayController pushes informational layouts; no menu state or navigation model.
Expected behavior:
  - Multi-line layouts present readings, states, alerts, or setup QR when instructed.
  - INVERTED emphasis may mark alerts; it does not imply menu selection.
Non-goals:
  - Menu screens, scrollable options, cursor, touch/button-driven UI on the OLED.
Affected files:
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - No acceptance criteria shall require menu or interactive display behavior in v1.
Open questions:
  - None.
```

## 2026-06-20 — Root-Only HTTP URL and No HTTPS in QR v1

```text
Date: 2026-06-20
Decision: v1 QR payloads are http://IPv4 with implicit root / only. No path in the
  QR string. Redirects to /something are handled by another entity, not firmware.
  https is out of scope for v1 because TLS is unlikely on small MCUs like ESP32-C3
  for local setup in this product.
Context: User confirmed the scanned page is the site root; subpath routing and
  HTTPS termination are outside the display/encoder stack.
Implementation contract:
  - docs/qr_encoder_interface.md sections Root page and redirects, Why not HTTPS.
  - setup_url_validate rejects paths and https schemes.
Expected behavior:
  - QR encodes e.g. http://192.168.4.1 (browser opens /).
  - Server or portal may redirect after scan; firmware does not encode /setup etc.
Non-goals:
  - https in QR, path suffixes in QR, firmware redirect logic.
Affected files:
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - setup_url_validate rejects http://ip/path and https://ip.
Open questions:
  - None.
```

## 2026-06-20 — Printable ASCII Only (v1 Character Set)

```text
Date: 2026-06-20
Decision: v1 on-screen text and QR payloads use printable ASCII only. No tildes,
  accented letters, special symbols outside ASCII, or other languages/scripts in
  product copy.
Context: User confirmed Spanish accents and multilingual display are not needed
  for now. Unsupported characters sanitize to ? at render time.
Implementation contract:
  - docs/oled_text_display_interface.md section Character Set Policy (v1 Product).
  - sanitize_ascii replaces non-printable and non-ASCII input with ?.
Expected behavior:
  - Callers should supply ASCII-only strings (e.g. Espana not Espana-with-tilde).
  - If non-ASCII is passed, deterministic ? replacement (e.g. cafe-with-accent -> caf?).
Non-goals:
  - UTF-8 decoding, Unicode fonts, locale shaping, RTL, emoji, custom glyph packs.
Affected files:
  - docs/oled_text_display_interface.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Host tests verify sanitize / render replaces accented input with ?.
Open questions:
  - None.
```

## 2026-06-20 — Sporadic QR on Dynamic Layouts

```text
Date: 2026-06-20
Decision: QR is occasional content on a fully dynamic display, not a permanent
  reserved screen area.
Context: The product alternates between text-only screens and rare setup screens
  that include a QR region.
Implementation contract:
  - docs/qr_encoder_interface.md and docs/oled_text_display_interface.md state
    that layouts replace atomically at runtime.
  - QR_LEFT_TEXT_RIGHT is a reference template only, not a fixed split-screen mode.
  - Encoder runs only when the active layout contains a QR region.
Expected behavior:
  - Text-only layouts are normal operating mode.
  - Switching away from a QR layout leaves no QR artifacts on screen.
Non-goals:
  - Always-visible QR panel or dedicated QR real estate on every screen.
Affected files:
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/test_strategy.md
  - docs/architecture.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Tester verifies layout switch QR to text-only clears QR pixels.
Open questions:
  - None.
```

## 2026-06-20 — SSD1306 OLED Controller for v1

```text
Date: 2026-06-20
Decision: Use SSD1306 as the sole OLED controller driver for v1 firmware.
Context: Bench hardware with the existing display_driver SSD1306 path already
  produces correct 128x64 I2C output. No controller change is planned.
Implementation contract:
  - display_driver.c remains SSD1306-only.
  - Do not implement SH1106 column offset or alternate init sequences unless
    physical hardware changes.
Expected behavior:
  - OLED at 0x3C or 0x3D initializes and flushes via i2c_bus_transceive.
Non-goals:
  - SH1106 support, controller auto-detection, dual-driver abstraction.
Affected files:
  - components/display/display_driver.c
  - docs/esp32_c3_supermini_connections.md
  - agent-workspaces/implementer/handoff.md
Validation expectations:
  - Tester confirms visual output on current SuperMini + 0.96" I2C module.
Open questions:
  - None.
```

## 2026-06-20 — Display Visual Demo Protocol

```text
Date: 2026-06-20
Decision: Boot-time OLED demo-test gated by CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO.
Context: Display work was validated on host and serial but QR hardware scan and
  per-step human observation were not repeatable. Tester Run 005 noted the gap.
Implementation contract:
  - docs/display_visual_demo_protocol.md is normative.
  - app_core_run_visual_demo() in app_core_display_demo.c; max 4 steps, 30 s hold.
  - Serial markers DEMO_STEP i/N for grep-friendly evidence.
  - Implementer copies Demo Manifest into handoff; tester records per-step feedback.
Expected behavior:
  - Kconfig y: baseline v1 sequence (four lines, QR setup, two lines).
  - Kconfig n: smoke-only single four-line screen at boot.
Non-goals:
  - Serial CLI demo trigger; automated camera OCR.
Affected files:
  - docs/display_visual_demo_protocol.md
  - components/app_core/
  - sdkconfig.defaults
  - agent-workspaces/implementer/handoff.md
  - agent-workspaces/tester/feedback.md
Validation expectations:
  - Tester Run 006 with human QR scan and per-step PASS/FAIL in feedback.md.
Open questions:
  - None.
```

## 2026-06-20 — Architect Role Hard Stop

```text
Date: 2026-06-20
Decision: Architect must never edit firmware or run builds; override hierarchy is
  documented and outranks plans, todos, and generic user instructions.
Context: An architect session edited components/ and ran idf.py while executing a
  mixed plan that included implementer code todos, despite prior role boundary rules.
Alternatives considered:
  - Rely on existing ROLE.md questions only.
  - Add repo-root .cursor/rules (rejected; firmware-only scope).
Implementation contract:
  - docs/architect_role_hard_stop.md is the canonical hard stop.
  - architect/ROLE.md, AGENTS.md, and methodology.md reference it.
  - Plans and "complete all todos" do not authorize firmware for architect.
  - Role switch requires explicit named role (implementer / tester).
Expected behavior:
  - Architect completes docs and handoffs only from mixed plans.
  - Code tasks are recorded as pending for implementer, not executed.
Non-goals:
  - Changing implementer or tester role boundaries in this decision.
Consequences:
  - Architect may refuse code/build steps without asking permission to violate rules.
Affected files:
  - docs/architect_role_hard_stop.md
  - agent-workspaces/architect/ROLE.md
  - firmware/AGENTS.md
  - docs/methodology.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - No architect session edits components/ or runs idf.py unless human names role switch.
Open questions:
  - None.
```
