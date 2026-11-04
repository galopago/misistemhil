# Human feedback — `b03_hil`

## 2026-11-03 — Slow relay walk

Timing used: **ON 4 s / OFF 3 s / gap 2 s** between channels; then `mask=5` (K1+K3) 4 s; all OFF.

Human (2026-11-03): confirmed sequence matched expectation — activate/release audible/visible as described for each step.

| Check | Heard/seen activate? | Heard/seen release? | Notes |
| --- | --- | --- | --- |
| K1 | yes | yes | |
| K2 | yes | yes | |
| K3 | yes | yes | |
| K4 | yes | yes | |
| K5 | yes | yes | |
| K6 | yes | yes | |
| mask=5 (K1+K3 together) | yes | yes (cleanup) | |

Overall: **PASS** (API + human observation).

## 2026-11-03 — OE glitch patch power-cycle listen

Image: `B03_RELAY_OE_LEVEL_BEFORE_DIR` flashed ~20:38.

| Check | Result | Notes |
| --- | --- | --- |
| Short relay chatter on USB power cycle / reset at boot | **PASS — none** | Human 2026-11-03 ~20:39: no click sound after patch |

Overall: **PASS** (OE glitch fixed).

## 2026-11-03 — Slow relay walk after OE patch

Timing: ON 4 s / OFF 3 s / gap 2 s; then mask=5; all OFF.

Human (2026-11-03): confirmed intentional activate/release for all steps.

| Check | Heard/seen activate? | Heard/seen release? | Notes |
| --- | --- | --- | --- |
| K1 | yes | yes | |
| K2 | yes | yes | |
| K3 | yes | yes | |
| K4 | yes | yes | |
| K5 | yes | yes | |
| K6 | yes | yes | |
| mask=5 (K1+K3) | yes | yes (cleanup) | |

Overall: **PASS** (API + human, post OE patch).

## 2026-11-03 — Slow v1 + v2 after relay_sched

API: **106/106 PASS**. Human (2026-11-03): confirmed audible/visible behavior OK;
clarified that log `http=XXms` is response latency, not coil duration; mid-hold
cancel on 15000 ms arm felt shorter because cancel fired ~3 s in (expected).

| Check | OK? | Notes |
| --- | --- | --- |
| v1 K1…K6 on/off + mask=5 | yes | |
| pulse K1…K6 (~2.5 s ON then auto-OFF) | yes | |
| hold K1 ~6 s then auto-OFF | yes | |
| hold_many K2+K4 (OFF at ~3 s / ~5 s) | yes | |
| hold_cancel mid K6 | yes | cancelled early vs 15 s arm |
| encoder_step K3/K4 phases | yes | |
| pulse_train K5 | yes | |

Overall: **PASS** (API + human). No further action requested.

## 2026-11-03 — Main relay OLED live refresh (slow)

API walk PASS. Human confirm OLED tokens update live:

| Check | OK? |
| --- | --- |
| STA four-line relay screen (not WIFI OK) | yes |
| K1…K6 `[1]` then `[0]` in sequence | yes |
| mask=5 → `1:[1]` and `3:[1]` together | yes |

Overall: **PASS** (static OLED).

## 2026-11-03 — OLED pulse / one-shot / train / encoder viz

API PASS (~88 s). Architecture note: `P`/`T`/`E` still **reserved**;
`relay_ctrl_drive` does not update view tokens.

| Check | OLED token seen? | LED/click heard? | Notes |
| --- | --- | --- | --- |
| pulse K1…K6 @ 4 s | **only `0`** | yes (implied) | no `P` during blink |
| hold / hold_many / train / encoder | only `0` (same pattern) | electrical activity | no `T`/`E` |

Overall: **observed as designed today** — OLED stays `0` while coils/LEDs pulse.
Human wants token viz (`P`/`T`/`E`); that needs **architect** handoff (then
implementer). Not a v1 OLED regression.

## 2026-11-03 — OLED P/T/E after `B03_RELAY_OLED_VIEW_MODE`

API PASS. Human (2026-11-03): **confirmed** — tokens now correct.

| Check | OLED OK? | Notes |
| --- | --- | --- |
| pulse K1…K6 → `[P]` then `[0]` | yes | |
| hold / hold_many → `[P]` | yes | |
| train K5 → `[T]` then `[0]` | yes | |
| encoder K3/K4 → `[E]` then `[0]` | yes | |

Overall: **PASS** (`B03_RELAY_OLED_VIEW_MODE`).
