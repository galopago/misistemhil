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
  - Display-owned URL formatting from netif — rejected; payload comes from outside.
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
  - External URL producer module and delivery contract for draw-QR instructions.
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
  - DisplayController applies instructions; it does not observe network readiness.
Non-goals:
  - WAITING screens, IP polling, or implicit setup states in the display stack.
Affected files:
  - docs/qr_encoder_interface.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Text-only operation with no QR instruction is valid default behavior.
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
