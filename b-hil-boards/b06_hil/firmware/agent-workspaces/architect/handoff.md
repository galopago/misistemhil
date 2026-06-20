# Architect Handoff

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Current Status

Active handoff: `OLED_TEXT_DISPLAY_INTERFACE`.

The implementer must not enable physical I2C OLED communication until display
pins, address, and driver selection are confirmed against the schematic and
recorded in the relevant board documentation.

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
    enable pins without schematic confirmation.
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
  - The base display size is exactly 128x64.
  - Layouts are runtime data composed of explicit rectangular regions.
  - Text supports printable ASCII only, controlled truncation without ellipsis,
    LEFT/CENTER/RIGHT horizontal alignment, TOP/CENTER/BOTTOM vertical group
    alignment, and NORMAL/INVERTED emphasis.
  - QR regions generate regular QR Code matrices from sanitized payload text
    using LOW error correction and scale 2 with scale 1 fallback.
  - Null, malformed, out-of-bounds, overlapping, or oversized inputs must follow
    deterministic best-effort behavior.
Non-goals:
  - Do not implement icons, gauges, cursors, arbitrary bitmaps, widgets, Micro
    QR, Data Matrix, Aztec, scrolling text, animations, power management, or a
    platform-specific public visual contract.
  - Do not enable electrical I2C bus configuration or physical OLED updates
    until hardware details are confirmed.
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
Validation plan:
  - Add host or component tests for geometry, clipping, layout switching,
    truncation, sanitization, inverted rendering, draw order, refresh policy, and
    QR fit behavior as described in `docs/test_strategy.md`.
  - Run `idf.py build` after implementation.
  - Add hardware tests only after pins, address, and driver selection are
    confirmed.
Open questions:
  - Confirm ESP32-C3 SuperMini pins for I2C OLED.
  - Confirm display I2C address and physical controller/driver.
  - Confirm QR encoder dependency that fits firmware footprint and licensing.
```

## Template

```text
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
