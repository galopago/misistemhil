# Test Runs — `b06_hil`

## 2026-11-03 — Build / flash / host API remote commands

| Field | Value |
| --- | --- |
| Board | ESP32-C3 USB-JTAG MAC `8c:d0:b2:a9:24:cd` |
| Identity / mDNS | `HIL-06-24CE` → `192.168.1.10` |
| App | `b06_hil_firmware` (`e33b7b1-dirty`), ESP-IDF v5.3.4 |
| Port | `/dev/ttyACM0` |
| STA | `openwrt-iot`, host API `port=80` |
| Endpoint | `POST http://192.168.1.10/v1/command` (+ mDNS verified) |

### Environment

```bash
source ~/esp/esp-idf/export.sh
cd firmware
idf.py build
idf.py -p /dev/ttyACM0 flash
# serial: STA IP + mdns hostname captured via pyserial
```

Flash note: first flash attempt failed (`/dev/ttyACM0` missing); USB reappeared
(Espressif `303a:1001`); second flash OK. App partition ~4% free (size warning).

### Command results

| Command | Args | Outcome |
| --- | --- | --- |
| `ping` | `{}` | PASS — `pong: true` |
| `info` | `{}` | PASS — `board_id=b06_hil`, `board_number=06`, `identity=HIL-06-24CE`, `sta_ipv4=192.168.1.10`, `streams_supported=false` |
| `capabilities` | `{}` | PASS — lists `ping`/`info`/`capabilities`/`meter_read`/`meter_read_all`; `streams: []`; no `echo`/`loopback` |
| `meter_read` | `channel:1` | PASS — `ok:true`, V≈5.13 V, I≈0.046 A, `power_w≈V*I` |
| `meter_read` | `channel:2` | PASS — `ok:true`, V≈5.14 V, I≈0 (noise) |
| `meter_read` | `channel:3` | PASS — `ok:true`, V≈5.13 V, I≈0 (noise) |
| `meter_read_all` | `{}` | PASS — `channels` length 3, order 1..3, all `ok:true` |
| `meter_read` | `channel:9` | PASS (negative) — outer `bad_request` |
| unknown name | — | PASS (negative) — `unknown_command` |
| `ping` via mDNS | — | PASS — `http://HIL-06-24CE.local/v1/command` |

Sample `meter_read_all` voltages ~5.12–5.14 V (USB 5 V bus). OUT1 drew ~47 mA;
OUT2/OUT3 near zero / signed noise.

### Acceptance checklist (`B06_HOST_API_METERING` items exercised)

1. capabilities catalog — PASS  
2. meter_read 1/2/3 — PASS  
3. bad channel → `bad_request` — PASS (used 9; same code path as 0/4)  
4. meter_read_all length/order — PASS  
5. I2C single-channel failure isolation — NOT EXERCISED (all channels OK)  
6. `power_w = voltage_v * current_a` — PASS (spot-check OUT1)  
7. SoftAP unavailable — NOT EXERCISED  
8. Module boundary (includes) — NOT CODE-REVIEWED this run  
9. OLED + meter concurrency — NOT EXPLICITLY STRESSED (OLED metering loop was running; commands succeeded)

### Verdict

**PASS** for exercised remote command surface on STA. Remaining acceptance items
5, 7–9 need dedicated follow-up if formal closeout of `B06_HOST_API_METERING`
is required.

## 2026-11-03 — Retest after credential erase + re-provision

Human erased WiFi credentials and re-entered them. Same board
`HIL-06-24CE` / `192.168.1.10`. No rebuild/flash this run.

| Command | Outcome |
| --- | --- |
| `ping` / `info` / `capabilities` | PASS |
| `meter_read` 1/2/3 | PASS (~5.13 V; OUT1 ~46 mA) |
| `meter_read_all` | PASS (3 channels) |
| `meter_read` channel 0 | PASS — `bad_request` |
| unknown command | PASS — `unknown_command` |
| mDNS `ping` | PASS |

**Verdict:** PASS after re-provision.

## 2026-11-03 — Retest after power cycle

Board reachable immediately as `HIL-06-24CE` / `192.168.1.10`.

| Command | Outcome |
| --- | --- |
| `ping` / `info` / `capabilities` | PASS |
| `meter_read` channel 1 (first) | **ANOMALY** — outer `ok:true`, result `ok:false` (no V/I/P) |
| `meter_read` channel 2/3 | PASS |
| `meter_read_all` | PASS (all three `ok:true`, OUT1 ~47 mA) |
| bad channel / unknown | PASS |
| mDNS `ping` | PASS |

Follow-up: five immediate `meter_read` channel 1 retries — all PASS. Second
`meter_read_all` — all PASS.

**Verdict:** Host API up after power cycle. One transient OUT1 `ok:false` on the
first post-boot `meter_read`; subsequent reads OK. Worth implementer attention if
reproducible at cold start.

## 2026-11-03 — Post power-cycle retest (network only, no USB reset)

Human confirmed board online; tester used only HTTP/mDNS (no serial open/reset).

| Command | Outcome |
| --- | --- |
| burst `meter_read` ch1 ×5 | PASS (all `ok:true`) |
| `ping` / `info` / `capabilities` | PASS |
| `meter_read` 1/2/3 + `meter_read_all` | PASS |
| bad channel / unknown / mDNS ping | PASS |

**Verdict:** PASS this run; prior OUT1 transient not reproduced.

## 2026-11-03 — After 4MB partition flash + power cycle

Network-only. `HIL-06-24CE` / `192.168.1.10`. Burst OUT1 ×5 + full suite — all PASS
(no OUT1 `ok:false`). mDNS ping PASS.
