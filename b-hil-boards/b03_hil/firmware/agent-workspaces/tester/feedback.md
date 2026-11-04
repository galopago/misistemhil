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
