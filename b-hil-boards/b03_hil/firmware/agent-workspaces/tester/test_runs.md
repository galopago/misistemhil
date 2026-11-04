# Test runs — `b03_hil`

## 2026-11-03T19:45 — Build + flash smoke (bringup package)

| Field | Value |
| --- | --- |
| Change | Implementer handoff: code complete packages A–E (scaffold, pins, `relay_ctrl`, `app_core`, host API relay) |
| Tree / commit | `b03_hil` at `ab3af3d` (`main`, local tree may include uncommitted work elsewhere in parent repo) |
| ESP-IDF | v5.3.4 |
| Target | `esp32c3` |
| Port | `/dev/ttyACM0` |
| Hardware | ESP32-C3 (QFN32) rev v0.4, 4MB flash (XMC), MAC `8c:d0:b2:a7:f0:6e`, USB-Serial/JTAG |

### Commands

```bash
source "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyACM0 flash
```

### Result

| Step | Outcome |
| --- | --- |
| `set-target` + `build` | **PASS** — `b03_hil_firmware.bin` size `0xf4750` (~76% free in app partition) |
| `flash` | **PASS** — bootloader, partition table, and app written; hash verified; hard reset |

### Evidence notes

- Flash map: `0x0` bootloader, `0x8000` partition table, `0x10000` app; flash size 4MB; baud 460800.
- Post-flash serial capture (pyserial 115200, ~8 s after reset; `idf.py monitor` needs a TTY):
  - OLED I2C `0x3C` on SCL GPIO0 / SDA GPIO1; display task started.
  - WiFi STA connected `openwrt-iot`, IP `192.168.1.11`.
  - `device_id: identity HIL-03-F06F`; `device_discovery: mdns start hostname=HIL-03-F06F`.
  - `host_api: host API started port=80`.
  - `main_task: Returned from app_main()`.
- Host capabilities / relay command exercise and LED/relay visual check **not** done in this run.

### Verdict

Build, flash, and boot-to-host-API **PASS**. Relay/host command smoke still open.

## 2026-11-03T19:46 — Host API relay command smoke

| Field | Value |
| --- | --- |
| Target | `http://192.168.1.11/v1/command` (`HIL-03-F06F.local`) |
| Contract | `docs/board_host_api_relay_architecture.md` (`B03_HOST_API_RELAY`) |
| ESP-IDF / HW | Same flashed image as prior run |

### Coverage

| Area | Result |
| --- | --- |
| `capabilities` summary + `relay_*` + mandatory cmds; `streams=[]` | PASS |
| `relay_get_all` shape; baseline `mask=0` | PASS |
| `relay_set` / `relay_get` ch1 on/off | PASS |
| Walk `relay_set` ch1…6 on → `relay_get_all` `mask=63` | PASS |
| `relay_set_all` `mask=5` and `states` (mask 42) + readback | PASS |
| Negatives: ch7, both/neither mask|states, mask=64, get_all extra key → `bad_request` | PASS |
| Cleanup `relay_set_all mask=0` | PASS |

### Result

**32 / 32 PASS**. API shadow/readback matches contract. Physical coil/LED click observation still human-side (not instrumented here).

### Verdict

Host relay command contract **PASS** on live board.

## 2026-11-03T19:47 — Slow human-observable relay walk

| Field | Value |
| --- | --- |
| Target | `http://192.168.1.11/v1/command` |
| Timing | ON hold **4 s**, OFF hold **3 s**, between channels **2 s** |
| Duration | ~73 s wall clock |
| Purpose | Human hear/see activation & release per channel |

### Sequence

1. All OFF baseline (`mask=0`), hold 3 s  
2. For K1…K6: ON 4 s → OFF 3 s → pause 2 s  
3. Pattern `mask=5` (K1+K3 ON) hold 4 s  
4. Cleanup all OFF hold 3 s  

### API result

All `relay_set` / `relay_get` / `relay_set_all` / `relay_get_all` responses **ok** during the walk.

### Human observation

**PASS** (human 2026-11-03): K1…K6 activate/release and mask=5 (K1+K3) matched expectation. See `feedback.md`.

## 2026-11-03T20:38 — OE glitch patch flash (`B03_RELAY_OE_LEVEL_BEFORE_DIR`)

| Field | Value |
| --- | --- |
| Change | `relay_ctrl_init` level-before-direction (OE + CTR) |
| ESP-IDF | v5.3.4 |
| Port | `/dev/ttyACM0` |
| Commands | `idf.py build` + `idf.py -p /dev/ttyACM0 flash` |

### Result

| Step | Outcome |
| --- | --- |
| Build | **PASS** (`relay_ctrl.c` rebuilt; bin `0xf4770`) |
| Flash | **PASS** (hash verified; hard reset) |
| Boot log | **PASS** — `relay_ctrl: OE enabled, mask=0x00`; return from `app_main` |

### Human observation (power-cycle / reset chatter)

**PASS** (human 2026-11-03): no short click sound after patch. See `feedback.md`.

## 2026-11-03T20:39 — Host relay commands retest (post OE patch)

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` (`192.168.1.11`) |
| Image | Same flashed `B03_RELAY_OE_LEVEL_BEFORE_DIR` build |
| Contract | `B03_HOST_API_RELAY` |

### Result

**32 / 32 PASS** — capabilities, set/get walk 1…6, `set_all` mask/states, negatives `bad_request`, cleanup `mask=0`. Regression vs pre-patch smoke: none.

## 2026-11-03T20:40 — Slow human relay walk (post OE patch)

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` |
| Timing | ON **4 s** / OFF **3 s** / gap **2 s** (~67 s) |
| Sequence | all OFF → K1…K6 on/off → mask=5 → all OFF |

### API result

All steps **PASS**.

### Human observation

**PASS** (human 2026-11-03): K1…K6 and mask=5 confirmed. See `feedback.md`.

## 2026-11-03T22:02 — Slow v1 + slow v2 host relay (`B03_HOST_API_RELAY_V2_LLM`)

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` |
| Duration | ~141 s wall clock |
| API checks | **106 / 106 PASS** |

### Part A — v1 slow

ON 4 s / OFF 3 s / gap 2 s for K1…K6; `mask=5`; cleanup `relay_release_all`.

### Part B — v2 slow / acceptance

| Area | Notes |
| --- | --- |
| capabilities + `timing_limits` + v2 cmds | PASS |
| `relay_pulse` K1…K6 @ 2500 ms; HTTP ≪ duration; auto-OFF | PASS |
| `relay_hold` 6000 ms; remaining_ms; natural OFF | PASS |
| `relay_hold_many` K2=5s K4=3s independent deadlines | PASS |
| `relay_hold_cancel` mid-hold | PASS |
| preempt `relay_set` during hold | PASS |
| `encoder_step` + second → `busy` | PASS |
| `encoder_pulse_train` | PASS |
| negatives (pulse 10 / period 5 / train+direction) | PASS |

### Human observation

**PASS** (human 2026-11-03): audible/visible OK; timing confusion clarified
(`http=ms` = latency). See `feedback.md`.

## 2026-11-03T22:01 — Build + flash

| Field | Value |
| --- | --- |
| ESP-IDF | v5.3.4 |
| Port | `/dev/ttyACM0` |
| App size | `0xf7ef0` (~75% free in partition) |
| Commands | `idf.py build` + `idf.py -p /dev/ttyACM0 flash` |

### Result

Build **PASS** (incremental, already current). Flash **PASS** (hash verified; hard reset).

## 2026-11-03T22:20 — Build + flash `B03_MAIN_RELAY_OLED`

| Field | Value |
| --- | --- |
| Change | Main STA OLED: four-line relay view (`app_core_relay_oled`, relay view tokens) |
| ESP-IDF | v5.3.4 |
| Port | `/dev/ttyACM0` |
| App size | `0xf8210` (~75% free) |
| Commands | `idf.py build` + `idf.py -p /dev/ttyACM0 flash` |

### Result

Build **PASS** (rebuilt `app_core_relay_oled`, `relay_ctrl`, `relay_sched`, host_api, wifi). Flash **PASS**.

### Human / next

Confirm STA screen shows `HIL-03-<MAC4>` + `N:[0|1]` pairs (not WIFI OK); `relay_set` updates live.

## 2026-11-03T22:21 — Slow OLED + relay live refresh

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` |
| Timing | ON 4 s / OFF 3 s / gap 2 s; then mask=5; all OFF |
| API | **PASS** |

### Expect on OLED

Line0 `HIL-03-F06F`; lines with `1:[X]  2:[X]` / `3:[X]  4:[X]` / `5:[X]  6:[X]`;
each channel `[1]` then `[0]` in turn; mask=5 → `1:[1]` and `3:[1]`.

### Human observation

**PASS** (human): static OLED live refresh confirmed.

## 2026-11-03T22:23 — Slow pulse/hold/train/encoder OLED observation

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` |
| API | **PASS** (~88 s) |
| Expect electrical | pulse 4 s/ch; hold 6 s; hold_many; train; encoder |
| Expect OLED tokens | `P`/`T`/`E` **not** yet product-enabled (reserved); likely stay `0`/`1` |

### Human observation

**Confirmed** (human 2026-11-03): during pulse blink OLED stayed **`0`** only
(no `P`/`T`/`E`). Matches reserved-token + `drive` policy in
`docs/board_main_relay_oled_architecture.md`. Gap for product token viz →
architect.

## 2026-11-03T22:33 — Build + flash `B03_RELAY_OLED_VIEW_MODE`

| Field | Value |
| --- | --- |
| Change | OLED view modes: pulse→`P`, train→`T`, encoder→`E`, clear→`0` |
| ESP-IDF | v5.3.4 |
| Port | `/dev/ttyACM0` |
| App size | `0xf8360` |
| Commands | `idf.py build` + `idf.py -p /dev/ttyACM0 flash` |

### Result

Build **PASS**. Flash **PASS**. Boot check next / pending slow P/T/E human walk.

## 2026-11-03T22:34 — Slow OLED P/T/E view-mode walk

| Field | Value |
| --- | --- |
| Target | `http://HIL-03-F06F.local/v1/command` |
| API | **PASS** (~88 s) |
| Expect | pulse/hold→`P`; train→`T`; encoder→`E`; end→`0` (stable letter, not edge flicker) |

### Human observation

**PASS** (human 2026-11-03): `P`/`T`/`E` then clear to `0` confirmed.
