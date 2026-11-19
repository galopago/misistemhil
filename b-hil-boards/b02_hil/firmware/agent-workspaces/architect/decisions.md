# Architect decisions — `b02_hil`

Append-only. Record architectural decisions for this board.

## 2026-11-17 — Daughter bootstrap without live base coupling

- Reuse model follows `docs/hil_base_reuse_contract.md`: contracts-first,
  **adapt-copy** of reference firmware, **no** git submodule / sub-repo /
  `EXTRA_COMPONENT_DIRS` into `b00_basehil`.
- Bootstrap snapshot: local working copy of family `docs/` + methodology
  guides; particular `docs/board_*.md` rewritten for `b02_hil`.
- After bootstrap, this tree **owns** its copies and may evolve independently.
  Re-absorbing family `BASE-CHANGE-…` IDs is optional and recorded in
  `docs/board_base_sync_log.md`.
- PCB copper / pad `pinfunction` in `../pcb/b02.kicad_pcb` is authoritative for
  GPIO assignment (same rule as `b03_hil`).
- Board identity: number **`02`**, hostname form `HIL-02-<MAC4>`.
- Product RS-485/relay semantics are **not** decided in bootstrap; only the
  hardware map and boot-safe `/KOE` rules are normative now.
- Firmware file adapt-copy is **implementer** work (`B02_FIRMWARE_ADAPT_COPY`).

## 2026-11-17 — Shared vs particular pins on this PCB

Pins that match the base bench for the same functions:

- I2C OLED: `GPIO0` SCL, `GPIO1` SDA
- Error LED: `GPIO8` active-low
- Factory reset: `GPIO7` active-low

Board-particular pins (must not be assumed from base docs):

- `/KOE` `GPIO2`; UART TX/RX `GPIO5`/`GPIO6`; `TTL_IN` `GPIO10`
- `485_CTR` `GPIO20`; relay CTR `GPIO21`/`GPIO9`/`GPIO3`/`GPIO4`

## 2026-11-17 — Isolated UART roles + boot-safe bring-up

Human-confirmed product roles (PCB-correlated):

| Element | Role |
| --- | --- |
| Isolation | Galvanic UART (ADuM1412 + PS1) |
| `K1` / `GPIO21` | TTL ↔ RS-485 mux |
| `K2` / `GPIO9` | RS-485 termination |
| `K3` / `GPIO3` | RS-485 pull-down |
| `K4` / `GPIO4` | RS-485 pull-up |
| `U2` ULN2003 | Relay coil driver |
| `U8` 74HCT541 + `/KOE` | Anti-glitch buffer during GPIO claim |
| `GPIO20` | MAX485 DE/`~RE` (via buffer + isolator) |

First implementer deliverable:

1. Adapt-copy base **onboarding** (no live base coupling).
2. `board_ctrl_init` with **level-before-direction** (same class as
   `b03_hil` `B03_RELAY_OE_LEVEL_BEFORE_DIR`), then enable OE with all
   relays off and `485_CTR=0`.

Runtime mux/term/DE APIs deferred.

## 2026-11-18 — Minimal single-channel relay host API (test)

- Family transport unchanged: `POST /v1/command` when STA host API is up.
- Commands: **`relay_set`** and **`relay_get`** only.
- Channels **`1`…`4`** = K1…K4; **one coil per request**.
- **No** `relay_set_all`, **no** `mask`, **no** `states[]` (human-confirmed).
- `485_CTR` not exposed as a relay channel.
- C API: `board_ctrl_relay_set` / `board_ctrl_relay_get`; host handlers call
  `board_ctrl` directly (`host_api` REQUIRES `board_ctrl`).
- `capabilities.summary` exact:
  `Isolated RS-485/TTL HIL board with single-channel relay test API`
- `/KOE` **must** already be enabled by `board_ctrl_init` before any
  `relay_set`; set/get must not disable or pulse OE (clarified 2026-11-18
  after human check). Without OE active, CTR changes cannot move coils.

## 2026-11-18 — UART path/line commands + `dut_uart` stream

- Path/line via **semantic POST commands** (`uart_path_*`, `uart_line_*`) so
  relays can change without touching baud (realistic DUT tests).
- Framing via `uart_config_*`; boot default **9600 8N1**; `uart_config_set`
  returns `busy` while stream open.
- Binary I/O: family stream plane, `stream_id` **`dut_uart`** only (no
  loopback).
- RS-485: each non-empty `write` asserts DE, TX, wait done, DE receive; TTL
  keeps DE inactive. Tentative CTR=1 drive (bench confirm).
- Path/line **MAY** change while stream open (no auto-close); DE not a user
  command.
- `capabilities.summary` exact when UART handoffs ship:
  `Isolated RS-485/TTL HIL board with dut_uart stream and path/line control`

## 2026-11-18 — STA main UART OLED (minimal HMI)

- While STA connected, replace base `WIFI OK` five-line with
  `FULL_FOUR_LINES` MEDIUM.
- Line 0: `HIL-02-<MAC4>` (no `.local`, no IP on this screen).
- Line 1: exact `PATH TTL` or `PATH RS485`.
- Line 2: `T:<0|1> U:<0|1> D:<0|1>` on RS-485; `T:- U:- D:-` on TTL
  (T=term, U=pull-up, D=pull-down).
- Line 3: originally `BAUD <n>`; **amended** after tester feedback to
  `BAUD <n> <d><p><s>` (e.g. `BAUD 9600 8N1`) — see
  `B02_MAIN_UART_OLED_FRAMING`.
- Event-driven refresh; paint via `app_core` only.

## 2026-11-18 — OLED line 3 framing (`B02_MAIN_UART_OLED_FRAMING`)

- Tester PASS on layout, then asked for data bits + parity on glass.
- Normative line 3: `BAUD <baud> <data><parity><stop>` with `N`/`E`/`O`.
- Fits MEDIUM ≤16 (`BAUD 115200 8N1` = 14). Format from live config fields.

## 2026-11-18 — Full HW UART framing matrix (`B02_UART_FRAMING_HW_FULL`)

- Tester: unlock `8E1` / `8O1` (baud list already OK).
- Human: expose **all** combinations the ESP32-C3 UART supports now.
- Authority: ESP-IDF UART on this MCU; ADuM/MAX485 do not filter framing.
- Allow: data **5–8**, parity **none|even|odd**, stop **1|2**, plus **1.5
  only with 5 data bits** → **27** profiles × 8 bauds.
- Reject: mark/space; 1.5 stop with data≠5; bauds outside list.
- Default remains **9600 8N1**. OLED cluster uses stop token `15` for 1.5.
