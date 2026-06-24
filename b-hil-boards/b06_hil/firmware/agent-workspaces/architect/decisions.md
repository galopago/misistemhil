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

## 2026-06-23 — Display v1 Implementation Snapshot

```text
Date: 2026-06-23
Decision: Document the current implemented display v1 structure as the reference
  shape future implementers should reproduce unless a new architect handoff changes it.
Context: The implementer concretized display delivery, setup_url, QR encoding,
  and visual demo code. Architecture docs must now reduce variation for future
  LLM implementers.
Implementation contract:
  - app_core exposes app_core_display_show_template and
    app_core_display_show_qr_setup as the public display facade.
  - app_core validates QR payloads with setup_url_validate before calling
    display_controller_show_qr_setup.
  - display_controller_show_template rejects DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT;
    QR setup uses display_controller_show_qr_setup with explicit payload.
  - setup_url component path is components/setup_url/ with format, validate,
    and self-test helpers.
  - Nayuki qrcodegen is vendored at components/qr_encoder/vendor/qrcodegen/.
  - Current qrcodegen.c blob pin:
    34f1002501fa2bcb0a054f4311795b8cbb977f5b.
  - display_qr.c is the only qrcodegen adapter; it uses static version-2 buffers,
    byte mode, LOW ECC, Mask_AUTO, and boostEcl=false.
  - Boot visual demo lives in components/app_core/app_core_display_demo.c and
    runs through app_core_display_show_* APIs only.
Expected behavior:
  - Text and QR delivery stay on one app_core → DisplayController path.
  - No hardcoded QR URL exists in display_controller_show_template.
  - Future QR payload expansion, wrapper components, or alternate transport
    require a new architect handoff.
Non-goals:
  - Approving implementation quality, build results, or tester sign-off.
  - Moving template enums to a neutral header in this decision.
Consequences:
  - Docs now describe exact v1 component paths and public API names.
Affected files:
  - docs/architecture.md
  - docs/display_delivery_contract.md
  - docs/qr_encoder_interface.md
  - docs/oled_text_display_interface.md
  - docs/display_visual_demo_protocol.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
Validation expectations:
  - A future implementer following docs should create the same components and
    helper boundaries without adding an alternate QR/display path.
Open questions:
  - Product instruction source identity remains open if not internal to app_core.
```

## 2026-06-23 — WiFi Provisioning v1 Architecture

```text
Date: 2026-06-23
Decision: WiFi provisioning v1 uses an open device SoftAP at `192.168.4.1`,
  a root HTTP form at `http://192.168.4.1/`, normal NVS
  credential persistence, and GPIO7 active-low factory reset as the recovery path.
Supersedence note:
  - The original fixed SSID `b06_hil_setup` is superseded by the 2026-06-23
    dynamic SSID decision: `HIL-<board_number_2_digits>-<last_4_softap_mac_hex>`.
Context: The product needs user-entered WiFi credentials without serial tooling.
  The existing display/QR architecture already supports `http://IPv4` setup QR
  payloads but forbids display/network coupling. The provisioning architecture
  must use that path without making display components observe WiFi state.
Alternatives considered:
  - WPA2 provisioning AP password — rejected for v1; user selected open AP.
  - Per-device provisioning password — deferred; requires manufacturing identity
    and label/serial policy.
  - Captive DNS portal — rejected for v1; user selected direct root URL/QR.
  - Encrypted NVS from v1 — rejected for v1 simplicity; normal NVS accepted.
  - Automatic AP fallback after saved credential boot failure — rejected; user
    selected disconnected-until-factory-reset behavior.
  - Web reset/change credentials UI — rejected for v1; GPIO7 factory reset is the
    recovery path.
Implementation contract:
  - docs/wifi_provisioning_architecture.md is normative.
  - components/wifi_credentials/ owns namespace `wifi_prov` and keys `ssid`,
    `password`, and `provisioned`.
  - components/wifi_provisioning/ owns SoftAP, HTTP server, `/` and `/provision`,
    bounded form parsing, and STA connection attempts.
  - wifi_provisioning exposes neutral provisioning events only; it must not
    include app_core.h, display_controller.h, display headers, or know display
    template identifiers.
  - app_core owns boot orchestration and all display calls.
  - Missing credentials or factory reset enter provisioning AP mode.
  - Submitted credentials are saved only after successful STA connection.
  - Saved credential failure at boot does not erase credentials and does not start
    AP; recovery requires GPIO7 factory reset.
  - Provisioning QR is `http://192.168.4.1` through
    app_core_display_show_qr_setup after app_core maps a neutral provisioning event.
Expected behavior:
  - User joins open AP `HIL-06-<MAC4>` for this board, opens/scans `http://192.168.4.1/`,
    submits SSID/password, sees success or failure, and can retry on failure.
  - On success, AP/HTTP stop and device remains connected as STA.
  - Reboot with valid saved credentials connects without opening AP.
  - Holding GPIO7 active-low for 2000 ms at boot erases credentials and restarts
    provisioning.
Non-goals:
  - Captive DNS, HTTPS, WPA2 provisioning AP, encrypted NVS requirement, web reset
    UI, WiFi enterprise, automatic credential erase after boot failure, automatic
    AP fallback after boot failure, app_core.h inclusion from wifi_provisioning,
    or direct display/app_core display calls from WiFi provisioning.
Consequences:
  - v1 provisioning is easy to use but not private against nearby clients during
    provisioning.
  - Tester must validate the disconnected-until-reset failure path explicitly.
  - Future security hardening requires a new architect handoff.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/architecture.md
  - docs/test_strategy.md
  - docs/qr_encoder_interface.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
Validation expectations:
  - Tests in docs/test_strategy.md WiFi Provisioning Criteria section.
  - Static audits confirm display tree excludes WiFi/network/provisioning headers
    and wifi_provisioning excludes app_core.h, display_controller.h, and display
    headers.
Open questions:
  - None.
```

## 2026-06-23 — WiFi Portal Reachability Corrective Contract

```text
Date: 2026-06-23
Decision: Tighten the WiFi provisioning SoftAP/HTTP startup contract after Runs
  007-009: configure AP IP/DHCP before WiFi start, never restart DHCP after AP
  start, clear AP-start event bits before esp_wifi_start, add /health and stable
  diagnostics, and forbid reboot loops on recoverable provisioning startup errors.
Context:
  - Run 007: phone joined AP but GET / returned blank page and no handler log.
  - Run 008: implementer fix cleared WIFI_AP_STARTED_BIT after esp_wifi_start,
    losing the AP_START event and causing timeout/reboot loop.
  - Run 009: SoftAP and HTTP logged ready, phone joined AP, but still 0x GET /
    handler entries. AP association alone is not sufficient evidence of portal
    reachability.
Alternatives considered:
  - Ask implementer to continue ad hoc debugging — rejected; prior attempts
    produced a race regression and still missed L3/httpd evidence.
  - Add captive DNS — rejected as out of v1 scope and not a fix for direct
    http://192.168.4.1 reachability.
  - Treat blank page as HTML rendering — rejected by evidence: handler was never
    invoked.
Implementation contract:
  - docs/wifi_provisioning_architecture.md sections Corrective Architecture From
    Runs 007-009 and ESP-IDF SoftAP Portal Startup Contract are normative.
  - AP netif IP/DHCP setup happens before esp_wifi_start.
  - DHCP is not stopped/restarted after WIFI_EVENT_AP_START during portal startup.
  - WIFI_AP_STARTED_BIT is cleared immediately before esp_wifi_start, never after.
  - HTTP server starts only after AP_START succeeds.
  - GET /health is a required plaintext reachability route.
  - Portal ready log is emitted only after AP IP, AP start, HTTP start, and route
    registration all succeed.
  - Provisioning startup errors return to app_core without ESP_ERROR_CHECK aborts.
Expected behavior:
  - A phone associated to b06_hil_setup can load /health and /.
  - Serial evidence distinguishes AP association, HTTP reachability, and HTML
    rendering failures.
  - SoftAP/HTTP failures leave firmware stable for logs and OLED error display.
Non-goals:
  - Captive DNS, HTTPS, AP password, web reset, changing QR profile, or widening
    WiFi/display dependencies.
Consequences:
  - Implementer must provide a deterministic startup log sequence before asking
    tester to retry.
  - Tester can reject any build where /health fails or handler logs are absent.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
Validation expectations:
  - Tester Run 010 or next run records phone IP, /health result, / form result,
    and serial handler logs.
Open questions:
  - None.
```

## 2026-06-23 — WiFi HTTP Handler Stack Safety

```text
Date: 2026-06-23
Decision: WiFi provisioning HTTP handlers must not build large HTML/form buffers
  on the HTTP worker stack. Setup and failure pages must use chunked/static or
  otherwise bounded non-stack output, and httpd stack size must be explicitly
  budgeted.
Context:
  - Run 010 fixed AP/DHCP/httpd reachability: phone got 192.168.4.2 and `GET /`
    reached the handler.
  - The device then crashed with stack protection fault in the httpd worker before
    HTML reached the browser.
  - Code review found `char html[2048]` in setup/failure page helpers and other
    stack buffers in the handler call chain.
Alternatives considered:
  - Increase httpd stack only — insufficient alone because future page/form edits
    could reintroduce large stack frames.
  - Keep one big snprintf HTML buffer — rejected; it already matches observed
    stack fault.
  - Move to filesystem-hosted assets — rejected as out of v1 scope.
Implementation contract:
  - docs/wifi_provisioning_architecture.md HTTP Handler Stack Safety is normative.
  - Every local array in HTTP handler synchronous call chains is <= 128 bytes.
  - Setup/failure pages are streamed in chunks or use bounded non-stack storage.
  - Form parsing avoids whole-body pair duplication on stack.
  - httpd_config_t.stack_size is explicitly set to at least 8192 unless smaller is
    proven safe by code review.
  - Response-completion logs distinguish handler entry from successful response
    delivery.
Expected behavior:
  - `/health`, `/`, and `/provision` responses complete without stack faults,
    panics, aborts, or reboots.
  - Tester sees both handler-entry and response-sent logs.
Non-goals:
  - JavaScript frontend, external assets, captive DNS, HTTPS, or UI redesign.
Consequences:
  - Implementer must treat memory layout of page rendering as acceptance-critical,
    not an optimization detail.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
Validation expectations:
  - Tester next run confirms `/health` and `/` response-sent logs and no reboot.
Open questions:
  - None.
```

## 2026-06-23 — WiFi HTTP Request Header Budget

```text
Date: 2026-06-23
Decision: The WiFi provisioning portal must set
  `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048` in source-controlled `sdkconfig.defaults`
  so common phone browsers can submit `POST /provision` without ESP-IDF httpd
  rejecting the request with HTTP 431 before application handler entry.
Context:
  - Run 011 verified GET / delivery after the HTTP stack safety fix.
  - Human operator confirmed the setup form was visible on the phone.
  - Submitting SSID/password failed with "Header fields are too long".
  - Serial logs showed `httpd_parse: parse_block: request URI/header too long`
    and HTTP 431, with no `POST /provision from client` application log.
  - Generated config had `CONFIG_HTTPD_MAX_REQ_HDR_LEN=512`.
Alternatives considered:
  - Keep 512 and tell tester to use a smaller client — rejected; product must work
    with common phone browsers.
  - Change the form to GET or put credentials in the URL — rejected because it
    leaks credentials and violates the HTTP portal contract.
  - Use JavaScript/fetch with custom headers — rejected because the v1 portal is
    no-JavaScript and server-rendered.
  - Raise to 1024 — possible but less robust; choose 2048 to cover normal mobile
    browser header variance with modest memory cost.
Implementation contract:
  - docs/wifi_provisioning_architecture.md HTTP Request Header Budget is normative.
  - `sdkconfig.defaults` contains `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048`.
  - `CONFIG_HTTPD_MAX_URI_LEN` may remain 512.
  - POST acceptance requires `POST /provision from client` in serial logs.
  - POST path must log parsed-form and STA-attempt milestones without passwords.
Expected behavior:
  - Phone browser POST reaches `provision_post_handler`.
  - HTTP 431 / `request URI/header too long` does not occur for normal form submit.
Non-goals:
  - Captive DNS, JavaScript, changing form method, external assets, HTTPS, or UI
    redesign.
Consequences:
  - Implementer must treat build-time httpd Kconfig as part of the portal
    architecture, not an incidental generated setting.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
  - agent-workspaces/implementer/backlog.md
Validation expectations:
  - Tester next run confirms no HTTP 431 and serial logs show POST handler entry.
Open questions:
  - None.
```

## 2026-06-23 — WiFi Submitted Success Commit Contract

```text
Date: 2026-06-23
Decision: Submitted WiFi provisioning success is committed only after fresh
  submitted STA `GOT_IP`, credential persistence, successful browser success-page
  delivery, delayed AP/HTTP teardown scheduling, and neutral success event emission
  in that order. OLED `WIFI OK` must be downstream of the web success response,
  not earlier.
Context:
  - Run 012 fixed the HTTP 431 header limit and GET / remained stable.
  - Human operator submitted credentials and saw no success or failure page.
  - OLED displayed `WIFI OK`.
  - The AP still appeared visible to the phone after submit.
  - Reboot used saved credentials but STA authentication failed and no IP was logged.
  - Existing code notes showed SUBMITTED_SUCCESS could be emitted before the HTTP
    success response was sent.
Alternatives considered:
  - Treat OLED success and NVS persistence as sufficient — rejected because the
    user-facing web flow failed and reboot STA failed.
  - Stop AP/HTTP immediately after queuing success page — rejected because it can
    drop the browser transport before visible feedback.
  - Accept AP visibility after submit as success — rejected unless bounded by a
    logged teardown grace window.
Implementation contract:
  - docs/wifi_provisioning_architecture.md Submitted Success Commit Contract is
    normative.
  - Each submitted attempt clears stale STA event bits and requires fresh
    `STA got IP ip=<ipv4> source=submitted`.
  - Credentials saved after submitted success are logged as
    `credentials saved source=submitted`.
  - `WIFI_PROV_EVENT_SUBMITTED_SUCCESS` occurs only after
    `POST /provision success response sent`.
  - Success response send failure after save triggers credential rollback/clear
    and keeps AP/HTTP active.
  - AP/HTTP teardown is deferred by 2000 ms and logged.
Expected behavior:
  - Browser shows success or failure page for every POST outcome.
  - OLED success cannot contradict blank web feedback.
  - Saved credentials connect on later STA-only boot and log
    `STA got IP ip=<ipv4> source=saved`.
Non-goals:
  - Captive DNS, JavaScript, AP password, HTTPS, or automatic AP fallback after
    saved-credential boot failure.
Consequences:
  - Implementer must treat web feedback, display feedback, persistence, teardown,
    and reboot behavior as one transaction rather than independent side effects.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
  - agent-workspaces/implementer/backlog.md
Validation expectations:
  - Tester next run records live serial during POST and reboot saved-credential
    STA-only boot.
Open questions:
  - None.
```

## 2026-06-24 — WiFi Saved Boot Reliability Contract

```text
Date: 2026-06-24
Decision: Intermittent saved-credential cold boot failure is not acceptable in v1.
  Saved boot must use WPA2/WPA3/WPA3-SAE-compatible STA config and a bounded
  application retry policy before entering locked disconnected state.
Context:
  - Run 013 POST success path passed: browser success page, AP teardown, OLED OK,
    and ping to STA IP when connected.
  - Same saved NVS credentials connected on boots 2 and 4 but failed on boots 1, 3,
    and 5 with `WIFI_REASON_AUTH_FAIL` reason 202.
  - Credentials are known good and signal is not the suspected cause.
  - Current code uses one saved `esp_wifi_connect()` attempt after cold STA start.
  - Current STA config sets `threshold.authmode = WIFI_AUTH_WPA2_PSK`, while the
    successful bench connection negotiates WPA3-SAE.
Alternatives considered:
  - Accept "sometimes connects" as v1 behavior — rejected; users cannot rely on a
    device that randomly shows HOLD RESET after valid provisioning.
  - Restrict v1 to WPA2-only routers — rejected for current bench/product reality;
    WPA3-SAE Personal must be supported.
  - Reopen provisioning AP automatically after saved boot failure — rejected; this
    conflicts with the selected product behavior and recovery remains factory reset.
  - Depend only on ESP-IDF auto-reconnect — rejected unless bounded and surfaced
    through application attempt logs.
Implementation contract:
  - docs/wifi_provisioning_architecture.md Saved Boot Reliability Contract is
    normative.
  - Saved boot uses max_attempts=5, per_attempt_timeout_ms=12000,
    cold_start_settle_ms=1000, and backoff_ms=[0,1000,3000,5000,8000].
  - STA config is compatible with WPA2-Personal, WPA2/WPA3 transition, and
    WPA3-SAE Personal; PMF capable true, PMF required false for v1 transition
    compatibility.
  - Final `WIFI_PROV_EVENT_SAVED_FAILURE_LOCKED` is emitted only after all saved
    attempts fail.
Expected behavior:
  - Known-good saved credentials connect reliably across cold boots.
  - Early auth failures can be recovered by retry before user-visible locked
    failure.
Non-goals:
  - Enterprise WiFi, captive upstream portals, automatic AP fallback, credential
    erase on saved failure, HTTPS, or AP password changes.
Consequences:
  - Test acceptance changes from one successful reboot to a 10-consecutive-cold-boot
    reliability sweep.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
  - agent-workspaces/implementer/backlog.md
Validation expectations:
  - Tester next run records 10 consecutive saved-credential cold boots with final
    PASS/FAIL and per-attempt logs.
Open questions:
  - None.
```

## 2026-06-23 — Product Boot Display Demo Retirement and Dynamic AP SSID

```text
Date: 2026-06-23
Decision: Normal product firmware must not run historical display demo/test
  screens at boot, and the WiFi provisioning AP SSID must be generated as
  `HIL-<board_number_2_digits>-<last_4_softap_mac_hex>`.
Context:
  - The OLED visual demo (`QR_SETUP`, `TEXT ONLY`, `DEMO_STEP`) has already served
    its validation purpose.
  - Product boot should show only real product states, not test demo content.
  - The old fixed AP SSID `b06_hil_setup` does not identify the physical device
    instance and does not generalize cleanly to sibling board projects.
Implementation contract:
  - docs/display_visual_demo_protocol.md is retired from normal product firmware.
  - `app_core_start()` must not call visual demo or smoke demo helpers.
  - `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` is removed from `sdkconfig.defaults`.
  - The board component owns the two-digit board number. For project `b06_hil`,
    the value is `06`; future `bNN_hil` projects use `NN`.
  - `wifi_provisioning` consumes the board-provided value and generates
    `HIL-06-ABCD` style SSIDs using the last two bytes of the SoftAP MAC.
  - The portal URL remains `http://192.168.4.1/`.
Expected behavior:
  - Fresh provisioning AP appears as `HIL-06-[0-9A-F]{4}`.
  - Normal product boot emits no `DEMO_STEP` logs and shows no demo-only QR/text.
  - Real WiFi provisioning display QR/status remains available.
Non-goals:
  - AP password, captive DNS, HTTPS, UI redesign, or removal of QR provisioning
    product behavior.
Consequences:
  - Test records should stop treating the visual demo as mandatory for ordinary
    display or WiFi work.
Affected files:
  - docs/display_visual_demo_protocol.md
  - docs/wifi_provisioning_architecture.md
  - docs/architecture.md
  - docs/test_strategy.md
  - agent-workspaces/architect/handoff.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/implementer/handoff.md
  - agent-workspaces/implementer/backlog.md
Validation expectations:
  - Tester confirms AP name pattern and absence of demo logs/screens on fresh boot.
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
Supersedence note:
  - This decision is superseded for normal product firmware by the 2026-06-23
    Product Boot Display Demo Retirement and Dynamic AP SSID decision. The demo
    remains historical validation evidence only.
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

## 2026-06-20 — Architect Immutable Mission Pillar

```text
Date: 2026-06-20
Decision: Architect task completes with documentation/architecture artifacts only;
  mission pillar is immutable while role is architect and cannot be overridden by
  plans, todos, harness, or conflicting prompts.
Context: Need explicit rule that "finish the task" means close docs/handoff, not
  ship firmware; prior sessions treated plan completion as permission to code.
Alternatives considered:
  - Rely on hard stop forbidden paths only without definition of done.
Implementation contract:
  - docs/architect_role_hard_stop.md § Immutable mission pillar at document top.
  - ROLE.md, AGENTS.md, methodology.md cross-reference mission and definition of done.
  - Priority rule: docs over code; partial firmware is role violation.
Expected behavior:
  - Architect stops when docs/handoff are complete even if code todos remain.
  - finish the task / complete all todos interpreted as architect slice only.
Non-goals:
  - Changing implementer or tester role definitions.
Consequences:
  - Architect may report task DONE without build or flash evidence.
Affected files:
  - docs/architect_role_hard_stop.md
  - agent-workspaces/architect/ROLE.md
  - AGENTS.md
  - docs/methodology.md
  - agent-workspaces/architect/decisions.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Architect cites mission pillar when asked to code to "finish the task".
Open questions:
  - None.
```

## 2026-06-23 — OLED Provisioning Setup UX (SSID on screen)

```text
Date: 2026-06-23
Decision: When no WiFi credentials are saved, the provisioning OLED screen uses
  QR-left + four-line right panel: numbered join/scan instructions and the SoftAP
  SSID split as prefix (7 chars) + suffix (remainder).
Context: Dynamic AP SSID HIL-06-MAC4 was generated but not shown on OLED; users
  saw only WIFI/SETUP beside the portal QR. The 64×64 right panel fits ~10
  SMALL glyphs per line; an 11-char SSID truncates if placed on one line.
Alternatives considered:
  - Single-line SSID: rejected (truncates unique MAC suffix).
  - WiFi join QR on right: rejected (two-QR ambiguity; user requested AP name text).
  - Unnumbered JOIN / SCAN QR: rejected (sequence join-then-scan less obvious).
  - OPEN password hint line: rejected (costs a line; AP is always open in v1).
Implementation contract:
  - wifi_provisioning passes s_ap_ssid on AP_STARTED and PORTAL_READY events.
  - app_core_wifi builds lines: "1 JOIN", ssid[0..6], ssid[7..], "2 SCAN QR".
  - PROV_SETUP_SSID_PREFIX_LEN = 7 fixed for HIL-NN-MAC4 format.
  - Fallback WIFI/SETUP when ssid NULL or strlen < 8.
  - QR payload unchanged: http://192.168.4.1.
Expected behavior:
  - User reads SSID from OLED, joins AP on phone, scans QR, opens portal.
Non-goals:
  - Spanish strings, dual QR, layout template changes.
Consequences:
  - PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID display strings superseded for
    provisioning setup path only.
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/display_delivery_contract.md
  - docs/oled_text_display_interface.md
  - agent-workspaces/architect/handoff.md (OLED_PROVISIONING_SETUP_UX)
  - components/wifi_provisioning/wifi_provisioning.c (implementer)
  - components/app_core/app_core_wifi.c (implementer)
Open questions:
  - None.
```

## 2026-06-23 — OLED WiFi Connected Status (IP + MAC)

```text
Date: 2026-06-23
Decision: On STA success (SUBMITTED_SUCCESS and SAVED_SUCCESS), show FULL_FOUR_LINES:
  WIFI OK / IPv4 / MAC bytes 0-2 / MAC bytes 3-5. Extend wifi_prov_event_info_t
  with sta_ipv4 and sta_mac supplied by wifi_provisioning; app_core splits MAC only.
Context: WIFI/OK gives no network identity. Full MAC (17 chars) fits one line at
  SMALL but split XX:XX:XX groups match human reading habits; IP alone on its line
  is immediately scannable.
Alternatives considered:
  - WIFI + OK on two lines: rejected (wastes a line; need IP + MAC).
  - Single-line MAC AA:BB:CC:DD:EE:FF: acceptable fit but dense; split preferred.
  - app_core reads esp_netif directly: rejected (couples app_core to WiFi stack).
  - Labels IP:/MAC: on separate lines: rejected (costs lines without clarity gain).
Implementation contract:
  - wifi_prov_event_info_t gains sta_ipv4, sta_mac (success events only).
  - wifi_provisioning caches IP on GOT_IP, formats MAC on success emit.
  - app_core show_wifi_connected_display: lines WIFI OK, sta_ipv4, mac[0..7], mac[9..].
  - WIFI_OK_MAC_PREFIX_LEN=8, WIFI_OK_MAC_SUFFIX_OFFSET=9.
  - Fallback FULL_TWO_LINES WIFI/OK if payload incomplete.
Expected behavior:
  - Screen persists until next display event; shown on provision success and saved boot.
Non-goals:
  - IPv6, AP MAC, QR, auto-clear, Spanish strings.
Consequences:
  - SUBMITTED_SUCCESS display changes from WIFI/OK to four-line detail.
  - SAVED_SUCCESS gains a display (was no-op).
Affected files:
  - docs/wifi_provisioning_architecture.md
  - docs/display_delivery_contract.md
  - docs/oled_text_display_interface.md
  - agent-workspaces/architect/handoff.md (OLED_WIFI_CONNECTED_STATUS)
  - components/wifi_provisioning/include/wifi_provisioning.h (implementer)
  - components/wifi_provisioning/wifi_provisioning.c (implementer)
  - components/app_core/app_core_wifi.c (implementer)
Validation expectations:
  - Tester: OLED IP/MAC match router DHCP and device STA MAC after provision and reboot.
Open questions:
  - None.
```

## 2026-06-23 — Error LED WiFi Link Status

```text
Date: 2026-06-23
Decision: Map WiFi product link state to GPIO8 error LED via wifi_link_status_t on
  every provisioning event, error_led component, and app_core_wifi adapter.
Context: Operators need WiFi status without reading OLED. Portal vs saved-connect
  vs connected vs locked-fail need distinct patterns. Solid ON only on saved-credentials
  connect path (not portal SUBMITTED_CONNECTING).
Alternatives considered:
  - Per-event LED mapping in app_core_wifi: rejected (fragile).
  - wifi_provisioning drives GPIO: rejected (coupling).
  - Polling portal_active from LED task: rejected (races).
Implementation contract:
  - wifi_link_status_t: UNPROVISIONED, CONNECTING, CONNECTED, DISCONNECTED.
  - error_led patterns: SLOW_BLINK 500/500ms, ON, OFF, FAST_BLINK 125/125ms.
  - app_core_wifi maps status to pattern; boot gap before first callback.
  - s_link_status single source of truth in wifi_provisioning.
Expected behavior:
  - No creds slow blink; saved connect solid ON; OK off; lock fast blink.
Non-goals:
  - STA_LOST_IP runtime, INA219 LED, portal connect solid ON.
Consequences:
  - New error_led component; wifi_prov_event_info_t gains link_status.
Affected files:
  - docs/error_led_wifi_link_architecture.md
  - docs/wifi_provisioning_architecture.md
  - agent-workspaces/architect/handoff.md (ERROR_LED_WIFI_LINK_STATUS)
  - components/error_led/, wifi_provisioning, app_core (implementer)
Validation expectations:
  - Tester visual LED per boot phase; serial log link_status on events.
Open questions:
  - None.
```

## 2026-06-23 — Error LED Runtime Link Status

```text
Date: 2026-06-23
Decision: Extend WiFi link_status updates to runtime with WIFI_PROV_EVENT_LINK_STATUS_CHANGED
  (LED-only event); keep existing wifi_link_status_t → error_led mapping unchanged.
Context: After CONNECTED, STA disconnect/LOST_IP left LED off because s_link_status was
  stale and no callback reached app_core_wifi. Operator requested LED-only fix aligned
  with prevailing link_status rules (same enum semantics as power-on), not WiFi retry design.
Alternatives considered:
  - Reemit SAVED_CONNECTING/SAVED_SUCCESS: rejected (changes OLED).
  - app_core polling: rejected (module boundary).
  - Direct GPIO from wifi_provisioning: rejected (coupling).
Implementation contract:
  - publish_link_status on enum change via LINK_STATUS_CHANGED.
  - Runtime hooks on STA_DISCONNECTED, STA_LOST_IP, STA_GOT_IP (pending source NONE).
  - app_core_wifi: empty case for LINK_STATUS_CHANGED; apply_wifi_link_status_led unchanged.
  - CONNECTING vs DISCONNECTED at runtime uses existing pending-attempt / locked semantics.
Expected behavior:
  - Link loss after connect: LED leaves OFF state per link_status.
  - Link restore: LED off; OLED unchanged on LINK_STATUS_CHANGED.
Non-goals:
  - WiFi reconnect policy, OLED on link loss, error_led timing changes.
Consequences:
  - wifi_prov_event_t gains LINK_STATUS_CHANGED; runtime rows in LED transition table.
Affected files:
  - docs/error_led_runtime_link_architecture.md (new)
  - docs/error_led_wifi_link_architecture.md
  - docs/wifi_provisioning_architecture.md
  - agent-workspaces/architect/handoff.md (ERROR_LED_RUNTIME_LINK_STATUS)
  - components/wifi_provisioning/, app_core_wifi.c (implementer)
Validation expectations:
  - Tester: AP outage with device connected; LED reflects loss; OLED static.
Open questions:
  - None.
```

## 2026-06-23 — WiFi Runtime Reconnect

```text
Date: 2026-06-23
Decision: Unbounded STA reconnect with capped exponential backoff after runtime link
  loss when NVS credentials are valid; never boot-lock from runtime loss alone.
Context: ERROR_LED_RUNTIME_LINK_STATUS (layer 1) detects loss and may show FAST blink
  but does not reconnect; AP recovery may require reboot. Operator chose infinite
  retry at runtime vs mirroring boot 5-attempt lock.
Alternatives considered:
  - Mirror boot (5 attempts then SAVED_FAILURE_LOCKED): rejected for runtime.
  - LED-only / no reconnect: rejected; product needs AP outage recovery.
  - Reuse SAVED_CONNECTING/SAVED_SUCCESS events: rejected (tester/log confusion).
  - esp_wifi_set_auto_connect only: rejected (opaque, hard to test, weak OLED/LED).
Implementation contract:
  - runtime_reconnect_task; run_sta_connect_attempt shared with connect_saved.
  - Events RUNTIME_RECONNECTING, RUNTIME_RESTORED; STA source=runtime in logs.
  - link_status CONNECTING for full episode (attempt + backoff); LED solid ON.
  - backoff_ms cap 30000; per_attempt_timeout_ms 12000.
  - Must not set s_locked_disconnected or s_saved_policy_exhausted from runtime.
Expected behavior:
  - Transient AP outage recovers without reboot; OLED CONNECTING then WIFI OK.
  - Indefinite outage: LED ON reconnecting, not HOLD RESET.
  - Boot bad creds: unchanged 5-attempt lock.
Non-goals:
  - Portal reopen, credential erase on runtime fail, change boot policy.
Consequences:
  - docs/wifi_runtime_reconnect_architecture.md; wifi_prov_event_t +2 events.
Affected files:
  - docs/wifi_runtime_reconnect_architecture.md (new)
  - docs/wifi_provisioning_architecture.md
  - docs/error_led_runtime_link_architecture.md
  - docs/error_led_wifi_link_architecture.md
  - agent-workspaces/architect/handoff.md (WIFI_RUNTIME_RECONNECT)
  - components/wifi_provisioning/, app_core_wifi.c (implementer pending)
Validation expectations:
  - Tester: AP off/on while connected; long outage; boot-lock regression.
Open questions:
  - 500 ms LOST_IP-only debounce if false starts observed.
```

## 2026-06-23 — WiFi Connect Cycle and Error LED v2

```text
Date: 2026-06-23
Decision: Replanned error LED and connect policy when NVS credentials exist: solid ON
  without creds; slow blink while connecting; fast 15 s after each 5-attempt round
  failure; repeat indefinitely; never SAVED_FAILURE_LOCKED or HOLD RESET OLED.
Context: Operator rejected terminal boot lock and HOLD RESET for AP-down scenarios.
  Prior v1 inverted UNPROVISIONED/CONNECTING LED mapping and locked after boot.
  Prior WIFI_RUNTIME_RECONNECT kept boot lock separate from runtime.
Alternatives considered:
  - Keep boot lock, runtime infinite: rejected (operator wants one contract).
  - v1 LED mapping: rejected.
Implementation contract:
  - wifi_link_status_t adds CONNECTING_ALERT; deprecate DISCONNECTED on creds path.
  - Events CONNECT_CYCLE_ACTIVE, CONNECT_ALERT_PHASE, CONNECT_RESTORED.
  - Round=5 attempts, alert=15s fast LED, unbounded cycle; boot=runtime same engine.
  - LED: UNPROVISIONED=ON, CONNECTING=SLOW, CONNECTING_ALERT=FAST, CONNECTED=OFF.
Expected behavior:
  - AP outage: slow → fast 15s → slow cycle; OLED stays CONNECTING; no HOLD RESET.
  - Success: LED off; WIFI OK screen.
  - No creds: LED solid ON.
Non-goals:
  - Portal reopen on failure; credential erase on network failure.
Consequences:
  - Supersedes ERROR_LED_WIFI_LINK_STATUS, ERROR_LED_RUNTIME_LINK_STATUS,
    WIFI_RUNTIME_RECONNECT for policy/LED.
  - docs/wifi_connect_cycle_architecture.md, error_led_connect_cycle_architecture.md
Affected files:
  - docs/wifi_connect_cycle_architecture.md (new)
  - docs/error_led_connect_cycle_architecture.md (new)
  - docs/wifi_provisioning_architecture.md, superseded LED/reconnect docs
  - agent-workspaces/architect/handoff.md (WIFI_CONNECT_CYCLE_AND_ERROR_LED_V2)
  - components/wifi_provisioning/, app_core_wifi.c (implemented; operator validated 2026-06-23)
Validation expectations:
  - Tester: no HOLD RESET with creds; LED phases match table; AP recovery without reboot.
  - **Operator validated 2026-06-23.** As-built doc:
    docs/wifi_connect_cycle_implementation_reference.md (boot blocks caller task;
    runtime uses wifi_rt_rc; logs source=boot not saved).
Open questions:
  - None.
```

---

## As-built documentation (2026-06-23)

```text
Date: 2026-06-23
Decision: Publish implementation reference for validated connect cycle v2.
Context: Operator confirmed firmware behavior matches v2 policy. Next implementer/LLM
  must reproduce without chat history.
Record: docs/wifi_connect_cycle_implementation_reference.md
Key as-built facts:
  - Motor: connect_cycle_run_forever → connect_cycle_run_round → run_sta_connect_attempt
  - Boot: connect_saved blocks same task as app_core_wifi_start (CONNECT_BOOT_SETTLE_MS=1000)
  - Runtime: handle_runtime_link_loss → xTaskCreate(wifi_rt_rc) → same motor
  - Events emitted: CONNECT_CYCLE_ACTIVE, CONNECT_ALERT_PHASE, CONNECT_RESTORED only (cycle)
  - Not emitted: SAVED_*, RUNTIME_RECONNECTING/RESTORED, SAVED_FAILURE_LOCKED
  - locked_disconnected() always false; LED default → ON
Affected files:
  - docs/wifi_connect_cycle_implementation_reference.md (new)
  - docs/wifi_connect_cycle_architecture.md, error_led_connect_cycle_architecture.md
  - docs/wifi_provisioning_architecture.md, agent handoffs
```

---

## 2026-06-23 — WiFi factory reset at runtime

```text
Date: 2026-06-23
Decision: Extend GPIO7 factory reset to runtime, not only cold boot.
Context: Connect cycle v2 runs indefinitely with credentials in NVS; boot-only
  erase forces power-cycle plus precise boot-window hold. Operator needs erase +
  portal while connected or cycling.
Alternatives considered:
  - esp_restart after erase: rejected as primary path (worse UX, slower).
  - Shorter hold (2000 ms): superseded — operator requires 10000 ms to prevent accidental erase.
  - Tap-to-erase: rejected (accidental erase risk).
Implementation contract:
  - docs/wifi_factory_reset_architecture.md
  - Monitor task wifi_fr_mon in app_core_wifi; shared 10000 ms hold sampling.
  - wifi_provisioning_factory_reset_to_portal(): abort cycle, stop STA, start portal.
  - run_sta_connect_attempt polls abort/credentials every <= 100 ms.
  - FACTORY_RESET_HOLD_MS=10000 (update legacy 2000 in app_core_wifi.c).
Expected behavior:
  - Hold GPIO7 10 s anywhere: NVS wifi_prov erased; portal; LED solid ON.
  - Short press: no change.
Consequences:
  - Handoff WIFI_FACTORY_RESET_RUNTIME for implementer.
  - wifi_provisioning_architecture.md Factory Reset section updated.
Affected files:
  - docs/wifi_factory_reset_architecture.md (new)
  - docs/wifi_provisioning_architecture.md, architecture.md
  - agent-workspaces/architect/handoff.md
Validation expectations:
  - Runtime reset from CONNECTED and from connect-cycle-outage without reboot.
Open questions:
  - None.
```

---

## 2026-06-23 — Factory reset hold duration 10 s

```text
Date: 2026-06-23
Decision: FACTORY_RESET_HOLD_MS = 10000 (10 s) for boot and runtime factory reset.
Context: Operator requested longer hold to avoid accidental credential erase.
  Supersedes 2000 ms hold in prior v1 boot implementation and initial runtime draft.
Consequences:
  - docs/wifi_factory_reset_architecture.md and related specs updated.
  - Implementer MUST change app_core_wifi.c FACTORY_RESET_HOLD_MS from 2000 to 10000.
Affected files:
  - docs/wifi_factory_reset_architecture.md, handoffs, test_strategy, procedures
Open questions:
  - None.
```

---

## 2026-06-23 — Factory reset as-built documentation (Run 021)

```text
Date: 2026-06-23
Decision: Publish implementation reference for validated factory reset v1.
Context: Run 021 PASS operator-critical path; tester Entry 024 reprovision OK.
Record: docs/wifi_factory_reset_implementation_reference.md
Key as-built facts:
  - Boot: factory_reset_requested_at_boot + shared hold helpers; no wait-for-release
  - Runtime: wifi_fr_mon incremental counter (not blocking hold_confirmed)
  - Portal API: vTaskDelete wifi_rt_rc; abort flag cleared before start_ap_portal
  - Abort: factory_reset_should_abort = flag OR !wifi_credentials_load
  - Poll: FACTORY_RESET_CONNECT_POLL_MS=100 in attempt wait, backoff, alert delay
  - Hold: FACTORY_RESET_HOLD_MS=10000 boot and runtime
Validated (Run 021): connect-cycle reset, portal idempotent, reprovision.
Deferred: boot hold, short-press, WIFI OK-only reset.
Affected files:
  - docs/wifi_factory_reset_implementation_reference.md (new)
  - docs/wifi_factory_reset_architecture.md, handoffs, test_strategy, procedures
Open questions:
  - None.
```
