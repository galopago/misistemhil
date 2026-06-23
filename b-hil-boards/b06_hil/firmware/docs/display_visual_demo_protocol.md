# Display Visual Demo Protocol

Human-in-the-loop visual validation for the HIL bench OLED. This is **not** a
product UX feature; it is a repeatable demo-test so the tester and a human
operator can confirm what appears on the physical panel after display-related
work.

Source handoff: `agent-workspaces/architect/handoff.md`,
`DISPLAY_VISUAL_DEMO_PROTOCOL`.

## Purpose

- Give the tester a **short, documented boot sequence** to observe on the OLED.
- Tie serial log markers to each step so evidence is grep-friendly.
- Separate **human observation** (what the operator sees) from **technical
  conclusion** (PASS/FAIL/BLOCKED) in tester records.

## Scope

Mandatory when a handoff touches any of:

- Display rendering, layouts, or templates.
- QR encoding or `display_qr`.
- `app_core` display delivery (`app_core_display_show_*`).

If the handoff does not touch the display, no demo manifest is required.

## Activation (Kconfig)

| Symbol | Default (HIL) | Behavior |
| --- | --- | --- |
| `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` | `y` in `sdkconfig.defaults` | After display init, run the full demo sequence in `app_core_run_visual_demo()`. |
| `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` | `n` | After display init, run **smoke only**: one `FULL_FOUR_LINES` screen (`b06_hil`, `READY`, `v0.1`, `""`). No step cycling. |

Disable the Kconfig for builds that must not cycle layouts at boot.

## Code location

| Item | Location |
| --- | --- |
| Demo sequence | `components/app_core/app_core_display_demo.c` |
| Public entry | `void app_core_run_visual_demo(void)` in `app_core_display_demo.h` |
| Smoke entry | `void app_core_run_display_smoke(void)` in `app_core_display_demo.h` |
| Boot wiring | `app_core_start()` in `app_core.c` |
| Kconfig | `components/app_core/Kconfig` |

Only `app_core` calls `app_core_display_show_*` during the demo (same path as
production delivery per `docs/display_delivery_contract.md`).

## Serial log contract

For each step, `app_core_display_demo.c` calls the step display API first. Then
it logs the hold marker and delays:

```text
DEMO_STEP i/N name=<step_id> hold_ms=<milliseconds>
```

After the hold delay:

```text
DEMO_STEP i/N done
```

- `i` and `N` are 1-based step index and total step count.
- `<step_id>` is a short stable identifier (e.g. `FULL_FOUR_LINES`,
  `QR_SETUP`, `FULL_TWO_LINES`).
- Log level: `ESP_LOGI` with tag `app_core_demo`.
- If a step API fails, the implementation logs `demo step <name> failed: <err>`
  and still emits the hold marker for the step. Tester records that step as
  `FAIL` unless the handoff explicitly allows it.

The tester MAY grep serial output for `DEMO_STEP` to confirm order and timing.

## Step rules

- Maximum **4 steps** per demo sequence.
- Maximum **30 seconds** total hold time (sum of all `hold_ms`).
- Each step MUST use `app_core_display_show_template()` or
  `app_core_display_show_qr_setup()` — not `display_controller_*` directly.
- Steps run on the calling task (typically `app_core_start` context); use
  `vTaskDelay` for holds.

## Baseline v1 sequence (current firmware)

When `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y`:

| Step | `name=` | API | Payload / lines | Hold (s) | Human expects on OLED |
| --- | --- | --- | --- | --- | --- |
| 1 | `FULL_FOUR_LINES` | `app_core_display_show_template` | `b06_hil`, `READY`, `v0.1`, `""` | 5 | Four-line layout; three visible text lines top-down, fourth empty |
| 2 | `QR_SETUP` | `app_core_display_show_qr_setup` | `http://192.168.4.1`, `SCAN`, `ME` | 8 | QR on left, text on right; phone camera scans URL |
| 3 | `FULL_TWO_LINES` | `app_core_display_show_template` | `TEXT`, `ONLY` | 5 | Two text lines only; **no QR pixels** remain |

Total hold: 18 s (within 30 s limit).

When Kconfig is `n`, `app_core_run_display_smoke()` shows only the same
`FULL_FOUR_LINES` smoke screen (indefinite until next boot; no `DEMO_STEP`
markers for the multi-step sequence).

## Implementer obligations

For every display-related handoff completion:

1. Keep `app_core_display_demo.c` aligned with the **Demo Manifest** below.
2. Add a `## Display Visual Demo` section to
   `agent-workspaces/implementer/handoff.md` containing:
   - Required Kconfig value (`CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y` unless
     documented otherwise).
   - Demo Manifest table (copy of current steps).
   - Commands: `idf.py build`, `idf.py flash`, `idf.py monitor` (or equivalent).
   - Estimated boot observation window (sum of holds + ~5 s margin).
3. Do not set `Ready for tester: Yes` without a complete manifest.

### Demo Manifest template (copy into implementer handoff)

```text
Handoff ID:
Kconfig: CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y
Total hold (s):
Commands: idf.py build && idf.py flash && idf.py monitor

Step | name= | API | Payload / lines | Hold (s) | Human expects on OLED
1    |       |     |                 |          |
2    |       |     |                 |          |
...
```

## Tester procedure

1. Confirm implementer handoff includes **Display Visual Demo** manifest.
2. Verify `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` in `sdkconfig` or
   `sdkconfig.defaults` (note value in test run).
3. Build and flash firmware.
4. Capture serial for **30–45 s** after reset.
5. Human operator watches OLED during the boot window.
6. For each manifest step, record in `agent-workspaces/tester/feedback.md`:
   - Expected (from manifest)
   - Observed (human words)
   - Result: PASS | FAIL | BLOCKED
7. Summarize in `agent-workspaces/tester/test_runs.md` with handoff ID and
   Kconfig value.

If `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=n`, mark the multi-step visual demo as
**BLOCKED**; do not infer PASS from prior runs. Smoke-only (single four-line
screen) may still be noted separately.

## Non-goals (v1)

- Menus, buttons, or user input on the device.
- Serial CLI to trigger or change demo steps on demand.
- Automated camera OCR or image snapshot comparison.
- Per-handoff demo variants without a firmware commit updating
  `app_core_display_demo.c`.

## Related documents

- `docs/display_delivery_contract.md`
- `docs/test_strategy.md` — Display Visual Demo Criteria
- `docs/oled_text_display_interface.md`
- `agent-workspaces/tester/feedback.md` — human observation template
