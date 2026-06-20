# Test Strategy

The initial strategy combines build checks, static review, and hardware
validation once the firmware starts using real peripherals.

All agentic development methodology documentation in this firmware tree is
maintained in English.

Shared-document changes for the OLED display interface are motivated by
`agent-workspaces/architect/handoff.md`, `OLED_TEXT_DISPLAY_INTERFACE`.

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

## Pending Items

- Define peripheral tests after closing the pin map.
- Define HIL fixtures when concrete use cases exist.
- Automate builds in CI if the repository adopts a pipeline.
- Define physical OLED tests after confirming I2C pins, display address, and
  chosen driver.
