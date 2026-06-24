# Test Strategy

The initial strategy combines build checks, static review, and hardware
validation once the firmware starts using real peripherals.

All agentic development methodology documentation in this firmware tree is
maintained in English.

Shared-document changes for the OLED display interface are motivated by
`agent-workspaces/architect/handoff.md`, `OLED_TEXT_DISPLAY_INTERFACE`.

Shared-document changes for I2C bus validation are motivated by
`agent-workspaces/architect/handoff.md`, `I2C_BUS_CONCURRENCY`.

Shared-document changes for QR encoder validation are motivated by
`agent-workspaces/architect/handoff.md`, `QR_ENCODER_INTERFACE`.

Shared-document changes for display delivery validation are motivated by
`agent-workspaces/architect/handoff.md`, `DISPLAY_DELIVERY_CONTRACT`.

Shared-document changes for WiFi provisioning validation are motivated by
`agent-workspaces/architect/handoff.md`, `WIFI_PROVISIONING_ARCHITECTURE`.

## Levels

- Build: `idf.py build`.
- Formatting and warnings: follow the ESP-IDF toolchain output.
- Host tests: add them when there are modules with logic decoupled from drivers.
- Hardware tests: flashing, serial monitor, and validation with HIL instruments.
- Display renderer tests: prefer host-side tests for the OLED visual contract in
  `docs/oled_text_display_interface.md` before enabling physical I2C display
  communication.

## Test Record

Each run must be documented in `agent-workspaces/tester/test_runs.md` with:

- Commit or change description.
- ESP-IDF version.
- Commands executed.
- Connected hardware.
- Observed result.
- Relevant evidence or logs.

## Initial Criteria

- The project configures for target `esp32c3`.
- The firmware builds without errors.
- `app_main` starts and calls `app_core_start`.
- No peripherals are enabled without confirmed pins.

## OLED Display Criteria

The 0.96 inch I2C OLED display interface is architecture-ready but hardware
communication remains blocked until the pin map and driver choice are confirmed.
Renderer, state, layout, and canvas behavior should be tested without hardware
first.

Host or component tests for the OLED implementation must cover:

- Rendering `FULL_FOUR_LINES` as four `16 px` line bands across the full
  `128x64` display.
- Rendering `FULL_TWO_LINES` as two `32 px` line bands.
- Rendering `TOP_TWO_BOTTOM_ONE` with top compact lines and one larger bottom
  line.
- Rendering `QR_LEFT_TEXT_RIGHT` with a left `64x64` QR region and right text
  region.
- Runtime replacement of complete `DisplayLayout` or `DisplayState` values.
- Latest-state-wins behavior when consecutive content updates arrive quickly.
- `CLEAR` producing a blank framebuffer and `REFRESH` forcing a redraw without
  changing state.
- Region clipping for partially out-of-bounds rectangles.
- Array-order drawing for overlapping regions, where later regions overwrite
  earlier regions.
- Pixel-based text truncation when measurement is available, with no ellipsis.
- ASCII sanitization by replacing non-printable or non-ASCII characters with
  `?`.
- Product strings for v1 should be ASCII-only at the source; if `café` is passed,
  rendered output is `caf?` (see Character Set Policy in the OLED contract).
- Region-level and line-level `INVERTED` emphasis (informational alerts, not menu
  selection).
- Separator rendering with separators enabled and disabled.
- No v1 acceptance criteria for OLED sleep, dimming, or display power-off behavior.
- QR generation from short printable ASCII payloads using regular QR Code with
  `LOW` error correction.
- Graceful blank rendering when a QR payload cannot fit at scale `2` or fallback
  scale `1`.

Golden tests should verify deterministic geometry even if selected fonts make
pixel-perfect text output platform-specific. At minimum, they must verify
region coordinates, clipped rectangles, line band heights, draw order, inverted
band fill behavior, QR scale fallback, and QR placement.

## QR Encoder Criteria

QR encoding validation follows `docs/qr_encoder_interface.md`. Rendering-only
tests remain under OLED Display Criteria above.

### setup_url utility

- `setup_url_validate("http://192.168.4.1")` returns true.
- `setup_url_validate("http://255.255.255.255")` returns true.
- `setup_url_validate("https://192.168.4.1")` returns false.
- `setup_url_validate("http://192.168.1.1/setup")` returns false (root only; no path).
- `setup_url_format_ipv4(192, 168, 4, 1, buf, len)` writes `http://192.168.4.1`.
- `setup_url_self_test()` returns `ESP_OK` during boot and logs the self-test pass.
- Payload encodes site root; post-scan redirect to a subpath is out of firmware scope.

### Matrix generation

- `display_qr_generate("http://192.168.4.1")` succeeds with `width == 25`.
- `display_qr_generate("http://1.1.1.1")` succeeds with `width == 21`.
- Empty, null, or invalid product payloads return false without crash.
- Invalid product URLs return false even though qrcodegen could encode arbitrary bytes.
- Encoded product payloads fit in `64x64` at scale 2 with quiet zone 1.

### Integration with renderer

- `QR_LEFT_TEXT_RIGHT` with canonical payload renders a scannable code on hardware
  when the physical OLED driver is active.
- `DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT` through `display_controller_show_template`
  returns `ESP_ERR_INVALID_ARG`; QR setup uses `display_controller_show_qr_setup`
  / `app_core_display_show_qr_setup` only.
- Invalid payload leaves the QR region blank; fallback text comes from
  `DisplayController`, not the renderer.
- Switching from a QR layout to a text-only layout removes all QR pixels; the
  display must not keep a reserved QR zone between layouts.
- Replacing a QR payload via a new layout update follows the same refresh rules as
  any other display content change (no QR-specific policy).

### Dynamic layout behavior

- Text-only layouts (`FULL_FOUR_LINES`, `FULL_TWO_LINES`, single-line layouts) are
  valid default states with no QR region.
- QR layouts are occasional and instruction-driven; tests must include at least
  one layout transition away from QR to text-only.
- Absence of any display instruction is valid; no test shall require a waiting
  or placeholder screen.

### QR test record additions

When validating QR encoder work, extend the test run record with:

- Nayuki vendor version or commit pin.
- Payload strings exercised.
- Whether encode was host-only or verified by camera/scanner on device.

## Display Delivery Criteria

Validation follows `docs/display_delivery_contract.md`.

### Caller discipline

- Direct includes of `display_controller.h` are limited to `components/app_core/`
  and display component internals in v1 firmware.
- Instruction-source components include `app_core.h`, not `display_controller.h`
  or `display.h`; indirect exposure of `display_layout_template_t` through
  `app_core.h` does not authorize direct display-controller calls.
- Display component tree MUST NOT include WiFi/network headers.

### Notification path

- Text and QR instructions use the same **callback → `app_core` → direct API** path.
- Tests invoke `app_core` display entry points; no `esp_event` display transport in v1.
- QR: `app_core` calls `display_controller_show_qr_setup` after `setup_url_validate`.
- Invalid QR payload does not call display APIs.
- Text: `app_core_display_show_template(DISPLAY_TEMPLATE_QR_LEFT_TEXT_RIGHT, ...)`
  is not a valid QR path; use `app_core_display_show_qr_setup`.

### Non-requirements

- No test shall require an application-level queue between instruction source and `app_core`.
- No test shall poll for pending instructions.
- No test shall couple display behavior to WiFi/network state.

## WiFi Provisioning Criteria

Validation follows `docs/wifi_provisioning_architecture.md`.

### Static and component validation

- `components/wifi_credentials/` owns NVS namespace `wifi_prov` and keys `ssid`,
  `password`, and `provisioned`.
- `wifi_credentials_validate` accepts SSID length `1..32` bytes and password
  length `0..63` bytes, and rejects overlength inputs before copying.
- NVS save/load/erase tests cover:
  - missing credentials;
  - valid saved credentials;
  - malformed or missing `provisioned`;
  - erase by factory reset path.
- HTTP form parser rejects malformed percent-encoding and request bodies larger
  than `256` bytes.
- SoftAP startup implementation review confirms:
  - AP IP/DHCP is configured before `esp_wifi_start()`;
  - DHCP is not stopped/restarted after `WIFI_EVENT_AP_START`;
  - `WIFI_AP_STARTED_BIT` is cleared before `esp_wifi_start()`, not after;
  - provisioning startup errors return to `app_core` without `ESP_ERROR_CHECK`
    reboot loops.
- Page renderer tests or review confirm:
  - `GET /` includes one form with method `post`, action `/provision`, fields
    `ssid` and `password`, SSID `maxlength="32"`, password `maxlength="63"`, and
    submit label `Connect`;
  - setup, success, and failure pages set `Content-Type: text/html; charset=utf-8`
    and `Cache-Control: no-store`;
  - no JavaScript, external CSS, images, fonts, icons, CDN, filesystem assets, or
    frontend-side polling are required;
  - any echoed SSID is HTML-escaped;
  - password is never echoed in HTML responses, logs, or implementation notes.
- HTTP handler stack review confirms:
  - no local `char html[2048]` or equivalent large response buffer exists in any
    HTTP handler or page helper;
  - no local form body or pair buffer larger than `128` bytes exists on the HTTP
    worker stack;
  - setup/failure pages are sent by constant chunks, bounded static/heap buffers,
    or another explicitly bounded non-stack strategy;
  - `httpd_config_t.stack_size` is explicitly set to at least `8192` bytes unless
    every synchronous handler call chain is proven to use less than `512` bytes of
    local array storage.
- HTTP request header budget review confirms:
  - `sdkconfig.defaults` contains `CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048`;
  - generated ESP-IDF config contains `CONFIG_HTTPD_MAX_REQ_HDR_LEN 2048` after a
    clean configure/build;
  - `CONFIG_HTTPD_MAX_URI_LEN` may remain `512`;
  - no workaround changes the credential form to `GET` or puts credentials in the
    URL.
- SoftAP SSID generation review confirms:
  - the provisioning AP SSID format is `HIL-06-<MAC4>` for this board;
  - `06` is provided by board-owned configuration derived from the `b06_hil`
    project identifier, not hard-coded inside `wifi_provisioning`;
  - `<MAC4>` is the last two bytes of the WiFi SoftAP MAC formatted as four
    uppercase hexadecimal characters with no separators;
  - no product code still uses the old fixed SSID `b06_hil_setup`.
- Display demo retirement review confirms:
  - `app_core_start()` does not call demo/smoke display helpers;
  - `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` is absent from `sdkconfig.defaults`;
  - normal product boot has no `DEMO_STEP` logs;
  - demo-only payloads such as `SCAN` / `ME` and `TEXT` / `ONLY` are not used in
    product display calls.
- Static audit: display component tree has no WiFi, HTTP, or NVS credential
  includes.
- Static audit: `components/wifi_provisioning/` does not include
  `display_controller.h` and does not call `display_controller_*`.
- Static audit: `components/wifi_provisioning/` does not include `app_core.h` or
  any display header, and does not call `app_core_display_show_*`.
- Static audit: `components/wifi_provisioning/include/wifi_provisioning.h`
  exposes neutral provisioning lifecycle APIs and event types only; it does not
  expose `httpd_handle_t`, `wifi_config_t`, `nvs_handle_t`, display template
  types, or display controller types.
- Component review: `app_core` registers a `wifi_prov_event_cb_t` callback and is
  the only module that maps provisioning events to OLED text or QR display calls.

### Fresh or reset provisioning flow

- With missing credentials or after factory reset, the device starts open AP with
  SSID matching `HIL-06-[0-9A-F]{4}`.
- The AP uses `192.168.4.1` and serves `GET /`.
- Serial logs include AP IP/gateway/netmask, SoftAP start, HTTP server start,
  handler registration for `GET /`, `GET /health`, and `POST /provision`, and
  portal ready in that order.
- Serial logs do not show SoftAP timeout, panic, abort, or reboot loop.
- Phone client receives a `192.168.4.x` address or the tester records the client
  IP shown by the phone OS.
- `GET /health` returns plaintext `OK` and logs `GET /health from client`.
- `GET /health` also logs `GET /health response sent`.
- `GET /` returns an ASCII form with `ssid` and `password` fields.
- `GET /` logs `GET / from client` and `GET / response sent`.
- If the page is blank and `GET / from client` is absent, the failure is
  network/HTTP reachability.
- If `GET / from client` is present but `GET / response sent` is absent, the
  failure is handler/page rendering or stack safety.
- Serving `/health`, `/`, failure page, and success page must not trigger stack
  protection fault, Guru Meditation, abort, or reboot.
- `GET /` page is usable from a phone browser without internet access, JavaScript,
  or external assets.
- Joining the AP requires no AP password in v1.
- The OLED may show QR setup, but the QR payload must be `http://192.168.4.1` and
  the request must flow through the app_core provisioning event callback to
  `app_core_display_show_qr_setup`.

### Submitted credential outcomes

- `POST /provision` with invalid or overlength form data returns a failure page,
  keeps AP/HTTP active, and does not attempt STA.
- `POST /provision` from a phone browser reaches the application handler and logs
  `POST /provision from client`; HTTP 431 before this log is a header-budget
  failure.
- Valid form submissions log `POST /provision parsed form` before any STA attempt.
- `POST /provision` with wrong network credentials attempts STA for up to
  `30000 ms`, returns the failure page, keeps AP/HTTP active, and does not save
  credentials.
- Wrong-credential submissions log `POST /provision attempting STA` and
  `POST /provision failure response sent`.
- `POST /provision` with valid credentials attempts STA for up to `30000 ms`,
  returns success, saves credentials to NVS, stops AP/HTTP after the success page,
  and leaves the device connected as STA.
- Valid-credential submissions log fresh `STA got IP ip=<ipv4> source=submitted`,
  `credentials saved source=submitted`, and `POST /provision success response sent`.
- OLED `WIFI OK` or equivalent submitted-success display appears only after
  `POST /provision success response sent`.
- AP/HTTP teardown is deferred after success response and logs
  `success teardown scheduled`, `HTTP server stopped after provisioning success`,
  `SoftAP stopped after provisioning success`, and `STA-only mode active after
  provisioning success`.
- If the browser shows a blank page after valid credential submit, the run is FAIL
  even if OLED shows `WIFI OK` or credentials appear saved.
- The success page text includes `WiFi connected. You can close this page.`
- The failure page text includes `WiFi connection failed. Check SSID and password.`
  and includes a retry form.
- The failure page keeps the password field empty even after a failed submission.
- Malformed form submissions return `400 Bad Request`; wrong WiFi credentials
  return the normal failure page and keep AP/HTTP active.

### Reboot and recovery

- Reboot with saved valid credentials connects STA without opening a provisioning
  AP matching `HIL-06-[0-9A-F]{4}`.
- Saved valid credential boot logs `STA got IP ip=<ipv4> source=saved`.
- Saved valid credential boot logs the saved boot retry policy and per-attempt
  boundaries:
  - `saved boot connect policy attempts=5 per_attempt_timeout_ms=12000`;
  - `saved boot attempt <n>/5 start`;
  - disconnect reason logs when an attempt fails;
  - `saved boot connected attempt=<n>/5 ip=<ipv4>` on success.
- Run a saved-credential cold-boot reliability sweep of 10 consecutive boots after
  successful provisioning. Acceptance requires 10/10 final successes and 0 final
  locked failures. Attempts that initially fail with `reason=202` may pass only if
  a later attempt within the same boot gets IP before the retry policy is exhausted.
- The saved-credential reliability sweep must be run against at least one
  WPA2/WPA3 transition or WPA3-SAE Personal router before declaring v1 WiFi
  provisioning accepted on the current bench.
- Reboot with saved but unreachable or invalid credentials does not erase NVS and
  does not reopen AP; the device remains disconnected until factory reset.
- Holding `GPIO7` active-low for at least `2000 ms` at boot erases NVS namespace
  `wifi_prov` and starts AP provisioning again.
- A short or released `GPIO7` input at boot must not erase credentials.

### WiFi provisioning test record additions

When validating WiFi provisioning work, extend the test run record with:

- Handoff ID `WIFI_PROVISIONING_ARCHITECTURE`.
- Whether NVS was fresh, erased by factory reset, erased by host command
  (`agent-workspaces/tester/procedures.md`), or already provisioned.
- AP SSID observed and whether it was open or password-protected.
- Whether AP SSID matched `HIL-06-[0-9A-F]{4}` and the observed suffix matched the
  last four hexadecimal characters of the SoftAP MAC if available in logs.
- Client device used to open `http://192.168.4.1/`.
- Client IP address observed on the phone/laptop after AP association.
- Result of `http://192.168.4.1/health` before testing the HTML form.
- Serial evidence for `GET /health from client`, `GET /health response sent`,
  `GET / from client`, and `GET / response sent`.
- Serial evidence for `POST /provision from client`, `POST /provision parsed form`,
  `POST /provision attempting STA`, and the corresponding success/failure response
  sent log.
- For valid credentials, serial evidence for `STA got IP ip=<ipv4> source=submitted`,
  `credentials saved source=submitted`, `success teardown scheduled`,
  `SoftAP stopped after provisioning success`, and `STA-only mode active after
  provisioning success`.
- Whether HTTP 431 or `request URI/header too long` appeared in serial logs.
- SSID/password cases exercised, with sensitive password values redacted.
- Whether the browser showed success, failure, blank page, or transport error after
  submit.
- Whether OLED success appeared before or after web success response.
- Whether AP stopped after successful provisioning.
- Whether reboot behavior matched saved-success and saved-failure criteria.
- Saved-credential reboot sweep summary: number of boots, per-boot final
  PASS/FAIL, number of attempts per boot, last disconnect reason, and final IP
  address when connected.

## Display Visual Demo Criteria

Validation follows `docs/display_visual_demo_protocol.md`, which is retired from
normal product firmware.

### When required

- Normal product handoffs MUST NOT require the historical boot demo. Display
  behavior is validated through real product states and focused tests.
- A future demo is required only if a new architect handoff explicitly authorizes
  a temporary test-only display demo.

### Kconfig

- `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` must be absent from `sdkconfig.defaults` for
  normal product firmware.
- If the symbol still exists in generated config, it must not cause boot-time demo
  calls.

### Product boot checklist

- Confirm no `DEMO_STEP` logs appear during normal product boot.
- Confirm no demo-only QR setup, `SCAN` / `ME`, or `TEXT` / `ONLY` test screen
  appears unless a future test-only demo handoff explicitly requests it.
- Confirm product display states still appear from real events, such as WiFi
  provisioning QR/status and WiFi success/failure.

### Retired baseline

- The historical Run 006 demo acceptance remains archived in tester notes.
- Do not use prior demo PASS results as evidence that current product boot should
  display demo content.

### Visual demo test record additions

When validating display work, extend the test run record with:

- Handoff ID.
- Serial grep evidence that `DEMO_STEP` is absent for normal product boot.
- Whether any demo-only text/QR screen appeared unexpectedly.
- Which real product state caused each observed display update.

## I2C Bus Criteria

I2C validation follows the incremental phases in
`docs/i2c_bus_architecture.md`. Test only the active implementer phase unless
the architect explicitly authorizes a multi-phase re-test.

### Phase 1 — Direct sync

- Build succeeds for target `esp32c3`.
- Firmware boots after `i2c_bus_init()`.
- Serial logs show bus init on the configured SCL/SDA pins.
- OLED probe tries board-allowed addresses and logs detected or missing device.
- Missing OLED at boot does not panic; display stub may continue.
- No requirement yet for concurrent task I2C traffic.

### Phase 2 — Transaction executor

- `i2c_bus_transceive()` is the only device-driver I2C entry point.
- Two tasks issuing alternating transactions do not corrupt data or hang the bus.
- Transaction timeout returns an error and leaves the bus usable for the next
  transaction.
- Display driver may still be a stub, but any I2C path it uses must go through
  `i2c_bus_transceive()`.

### Phase 3 — Priority broker

- `i2c_broker_submit()` is used by application tasks that perform I2C.
- Display refresh and INA219 periodic reads can run concurrently for at least
  60 s without deadlock.
- Higher-priority INA219 traffic is not blocked for more than one sample interval
  by OLED traffic under normal load.
- Broker timeout returns an error without requiring reboot.

### Phase 4 — Observability (optional)

- Debug counters for submitted, completed, timeout, and dropped requests match
  observed traffic within the test window.

### I2C test record additions

When validating an I2C phase, extend the test run record with:

- Active I2C phase handoff ID (`I2C_BUS_PHASE1`, `PHASE2`, etc.).
- Connected I2C devices and addresses observed.
- Whether OLED physical traffic was blocked or active.
- Whether INA219 hardware was present or simulated/deferred.

## Pending Items

- Define peripheral tests after closing the pin map.
- Define HIL fixtures when concrete use cases exist.
- Automate builds in CI if the repository adopts a pipeline.
- Define physical OLED tests after confirming I2C pins, display address, and
  chosen driver.
- Execute I2C phase 3 contention tests after INA219 and physical OLED drivers are
  both active.
