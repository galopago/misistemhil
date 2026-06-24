# Display Visual Demo Protocol

Human-in-the-loop visual validation for the HIL bench OLED.

**Status: retired from normal firmware builds.** This document remains as a
historical record of the demo-test that validated the display stack. It is not a
product UX feature and must not be wired into boot/runtime display calls for the
WiFi provisioning firmware.

Source handoff: `agent-workspaces/architect/handoff.md`,
`DISPLAY_VISUAL_DEMO_PROTOCOL`.

## Purpose

- Preserve the historical display demo acceptance record from Run 006.
- Make explicit that the QR demo, text-only demo, and cycling visual steps must
  not be called by product boot code.
- Provide guardrails for future temporary display demos if a human explicitly
  authorizes a new isolated test-only handoff.

## Current Product Rule

Normal firmware MUST NOT call the historical demo helpers or cycle through test
screens such as QR setup demo, `TEXT ONLY`, or generic layout exercises.

Required product behavior:

- Display calls must be caused by real product state only, such as boot status,
  WiFi provisioning QR/status, WiFi success/failure, or future product features.
- `app_core_start()` must not call `app_core_run_visual_demo()` or
  `app_core_run_display_smoke()`.
- `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` must not be used to enable boot-time demo
  display behavior in product firmware.
- Demo-only payloads such as `http://192.168.4.1` QR demo, `SCAN` / `ME`,
  `TEXT` / `ONLY`, and `DEMO_STEP` serial markers must not appear in normal boot
  logs.
- Historical demo source files may be deleted or left unreachable only if no
  product code, CMake target, Kconfig default, or handoff requires them.

If a future display investigation needs a demo again, it requires a new explicit
architect handoff with test-only scope, authorized files, activation mechanism,
and removal criteria.

## Activation (Kconfig)

Retired. `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` must be removed from
`sdkconfig.defaults`, and product firmware must build without depending on this
symbol.

## Code location

| Item | Location |
| --- | --- |
| Demo sequence | `components/app_core/app_core_display_demo.c` |
| Public entry | `void app_core_run_visual_demo(void)` in `app_core_display_demo.h` |
| Smoke entry | `void app_core_run_display_smoke(void)` in `app_core_display_demo.h` |
| Boot wiring | `app_core_start()` in `app_core.c` |
| Kconfig | `components/app_core/Kconfig` |

These locations are historical. Implementers removing the demo should delete or
disconnect these files and references so product builds no longer contain demo
entry points.

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

The tester now greps serial output for absence of `DEMO_STEP` in normal product
boots.

## Step rules

- Maximum **4 steps** per demo sequence.
- Maximum **30 seconds** total hold time (sum of all `hold_ms`).
- Each step MUST use `app_core_display_show_template()` or
  `app_core_display_show_qr_setup()` — not `display_controller_*` directly.
- Steps run on the calling task (typically `app_core_start` context); use
  `vTaskDelay` for holds.

## Historical baseline sequence

Historical Run 006 sequence when `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO=y`:

| Step | `name=` | API | Payload / lines | Hold (s) | Human expects on OLED |
| --- | --- | --- | --- | --- | --- |
| 1 | `FULL_FOUR_LINES` | `app_core_display_show_template` | `b06_hil`, `READY`, `v0.1`, `""` | 5 | Four-line layout; three visible text lines top-down, fourth empty |
| 2 | `QR_SETUP` | `app_core_display_show_qr_setup` | `http://192.168.4.1`, `SCAN`, `ME` | 8 | QR on left, text on right; phone camera scans URL |
| 3 | `FULL_TWO_LINES` | `app_core_display_show_template` | `TEXT`, `ONLY` | 5 | Two text lines only; **no QR pixels** remain |

Total hold: 18 s (within 30 s limit).

This is historical evidence only. Normal product firmware must not enable this
sequence or the smoke-only fallback.

## Implementer obligations

For this retirement:

1. Remove boot/runtime calls to `app_core_run_visual_demo()` and
   `app_core_run_display_smoke()`.
2. Remove demo source/header/Kconfig/CMake references if they are no longer used.
3. Remove `CONFIG_B06_HIL_DISPLAY_VISUAL_DEMO` from `sdkconfig.defaults`.
4. Preserve real display APIs used by product state:
   `app_core_display_show_template()` and `app_core_display_show_qr_setup()`.
5. Do not remove QR generation or setup QR product behavior used by WiFi
   provisioning.

## Tester procedure

1. Build and flash firmware.
2. Capture serial during normal product boot.
3. Confirm no `DEMO_STEP` logs appear.
4. Confirm no demo-only QR/text screen appears.
5. Confirm real product display states still appear when product events request
   them.

For normal product boot after retirement:

- No `DEMO_STEP` logs.
- No QR demo appears unless WiFi provisioning is actually active and requests the
  product setup QR.
- No `TEXT ONLY` test screen appears.
- Product WiFi/display status screens still work.

## Non-goals (v1)

- Menus, buttons, or user input on the device.
- Serial CLI to trigger or change demo steps on demand.
- Automated camera OCR or image snapshot comparison.
- Per-handoff demo variants without a new explicit architect handoff and removal
  criteria.

## Related documents

- `docs/display_delivery_contract.md`
- `docs/test_strategy.md` — Display Visual Demo Criteria
- `docs/oled_text_display_interface.md`
- `agent-workspaces/tester/feedback.md` — human observation template
