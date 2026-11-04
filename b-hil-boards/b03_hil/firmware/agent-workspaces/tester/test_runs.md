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
