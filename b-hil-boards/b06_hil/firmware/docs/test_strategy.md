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
- Region-level and line-level `INVERTED` emphasis.
- Separator rendering with separators enabled and disabled.
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
- `setup_url_validate("http://192.168.1.1/setup")` returns false.
- `setup_url_format_ipv4(192, 168, 4, 1, buf, len)` writes `http://192.168.4.1`.

### Matrix generation

- `display_qr_generate("http://192.168.4.1")` succeeds with `width == 25`.
- `display_qr_generate("http://1.1.1.1")` succeeds with `width == 21`.
- Empty, null, or invalid product payloads return false without crash.
- Encoded product payloads fit in `64x64` at scale 2 with quiet zone 1.

### Integration with renderer

- `QR_LEFT_TEXT_RIGHT` with canonical payload renders a scannable code on hardware
  when the physical OLED driver is active.
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
- Absence of any draw-QR instruction is valid; no test shall require a waiting
  or placeholder screen for URL availability.

### QR test record additions

When validating QR encoder work, extend the test run record with:

- Nayuki vendor version or commit pin.
- Payload strings exercised.
- Whether encode was host-only or verified by camera/scanner on device.

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
