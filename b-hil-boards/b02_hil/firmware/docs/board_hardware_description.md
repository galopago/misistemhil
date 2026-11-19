# Board Hardware Description — `b02_hil`

Particular hardware profile for **this** repository only.

Sources (KiCad — **PCB pad↔net / pinfunction is authoritative**):

- `../pcb/b02.kicad_pcb` (title `HIL-02`)
- `../pcb/b02.kicad_sch` (supporting; do **not** override PCB copper when
  schematic label geometry disagrees)

Do **not** assume `b00_basehil` pin maps. Local working copy
`docs/esp32_c3_supermini_connections.md` describes the **base** bench only.

Shared SuperMini patterns that **do** match this PCB: I2C on `GPIO0`/`GPIO1`,
error LED on `GPIO8`, factory-reset on `GPIO7`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b02_hil
Board number: 02
Schematic / PCB references:
  - ../pcb/b02.kicad_sch
  - ../pcb/b02.kicad_pcb
Identity: SoftAP / mDNS HIL-02-<MAC4>
```

## MCU / module

| Item | Value |
| --- | --- |
| Module | ESP32-C3 SuperMini (`U4`) |
| SoC class | ESP32-C3 (RISC-V) |
| ESP-IDF target | `esp32c3` |
| Flash (this module class) | **4 MB** expected (same SuperMini class as base) |
| Role | Runs base pillars + isolated serial / relay control |

Flash/partitions: adopt local working copy
`docs/flash_partition_architecture.md` unless a measured module differs.
Confirm physical flash on built units.

## Power and isolation topology

```text
USB 5V into SuperMini 5V pad (U4.16 = +5V)
  -> board net +5V
  -> U7 AMS1117-3.3 -> board net +3V3 (MCU / logic / OLED / buffer VCC)
  -> PS1 Mornsun B03XT1WR3 isolator DC-DC:
       Vin side: +3V3 / GND1
       Vout side: +3.3V / GND2  (MAX485, isolated ADuM secondary, J2 GND)

U4.14 (module 3V3 pad) is unconnected on this PCB (module regulates locally).
```

Rules:

- Relay coils (`K1`…`K4` A2) and `74HCT541` VCC are on **`+3V3` / GND1**.
- Isolated bus domain uses **`+3.3V` / GND2** from `PS1`.
- Never treat GND1 and GND2 as the same net in firmware assumptions.

## On-board peripherals

| Ref | Peripheral | Interface | Notes |
| --- | --- | --- | --- |
| `U4` | ESP32-C3 SuperMini | — | MCU |
| `U8` | 74HCT541 octal buffer | GPIO | Control path enable/buffer (`/KOE`) |
| `U2` | ULN2003 sink driver | from `U8` Y outs | Four relay coil drivers |
| `K1`…`K4` | Relay DPDT Omron G6K-2F-Y | coils via `U2` | Mode / termination switching |
| `U3` | ADuM1412 digital isolator | UART + DE path | MCU domain ↔ bus domain |
| `U1` | MAX485E | RS-485 | Isolated A/B on `R_PU`/`R_PD` |
| `U6` | 74LVC2G34 | TX/RX sense | Activity LED buffers |
| `U5` | OLED 128×64 I2C | I2C | SSD1306-class; series `R9`/`R10` |
| `U7` | AMS1117-3.3 | power | 5 V → 3.3 V (GND1 domain) |
| `PS1` | B03XT1WR3 | isolated DC-DC | Creates GND2 / +3.3V |
| `D3` | Error LED | GPIO | Firmware-owned, active-low via `ERR_LED` |
| `D1` / `D2` | TX / RX activity LEDs | from `U6` | Hardware activity; not firmware PWM |
| `SW1` | Push button | GPIO | Factory-reset / `DEFAULTS` |
| `J1` | 1×12 socket | remote | Display / reset / LED sense |
| `J2` | 1×6 socket | DUT | TTL TX/RX, TTL aux, RS-485 A/B |
| `D4` | Dual TVS | A/B | ESD on RS-485 lines |

## Control enable path (boot-safe)

MCU lines that command relays or RS-485 DE do **not** drive actuators directly.
Path:

```text
MCU CTR/DE  -->  U8 74HCT541 An/Yn  -->  ULN2003 / 485_DRV
                     ^
                     G1 & G2 = /KOE (active-low Output Enable)
```

`/KOE` (`/{slash}KOE` in KiCad):

- Tied to **`U8.G1` and `U8.G2`**
- Pulled **HIGH** to `+3V3` by **`R18`** → buffer outputs **Hi-Z** while MCU
  boots / pins float
- Driven by **`U4.GPIO2`**

### Normative firmware enable sequence

Authoritative step list, polarity, forbidden orders, and `board_ctrl` API:
**`docs/board_gpio_bringup_architecture.md`** (`B02_GPIO_BRINGUP_SAFE`).

Summary (do not implement from this summary alone):

1. Level-before-direction on `/KOE` → drive inactive high as output.
2. Level-before-direction on `K1`…`K4` CTR and `485_CTR` → all inactive low.
3. Enable `/KOE` (active low) with actuators cold.
4. Cold state: TTL path, PU/PD/term off, DE not driving.

Runtime mux/term/DE product APIs are **not** authorized until a later handoff.

## MCU pin map (from PCB pinfunction)

| Signal | MCU GPIO | PCB pad | Dir | Notes |
| --- | --- | --- | --- |
| `DISP_SCL` / I2C SCL | `GPIO0` | 9 | Out | → `R9` → OLED SCL; also `J1.2` |
| `DISP_SDA` / I2C SDA | `GPIO1` | 10 | BiDi | → `R10` → OLED SDA; also `J1.4` |
| `/KOE` | `GPIO2` | 11 | Out | Active-low OE for `U8` |
| `K3CTR` (pull-down) | `GPIO3` | 12 | Out | → `U8.A1` → `U2.I2` → `K3` coil |
| `K4CTR` (pull-up) | `GPIO4` | 13 | Out | → `U8.A0` → `U2.I1` → `K4` coil |
| `UART_TX` | `GPIO5` | 1 | Out | → isolator `VIB` / activity path |
| `UART_RX` | `GPIO6` | 2 | In | ← isolator `VOC` / activity path |
| `DEFAULTS` / factory reset | `GPIO7` | 3 | In | Active-low + pull-up `R1`; `SW1`; `J1.6` |
| `ERR_LED` | `GPIO8` | 4 | Out | Active-low → `D3` |
| `K2CTR` (termination) | `GPIO9` | 5 | Out | → `U8.A2` → `U2.I3` → `K2` coil (**strapping**) |
| `TTL_IN` | `GPIO10` | 6 | In | ← isolator `VOD` (aux sense; product use pending) |
| `485_CTR` | `GPIO20` | 7 | Out | → `U8.A4` → `485_DRV` → isolator → MAX485 DE/`~RE` |
| `K1CTR` (TTL/RS-485 mux) | `GPIO21` | 8 | Out | → `U8.A3` → `U2.I4` → `K1` coil |

Unused / tied on buffer inputs: `U8.A5`…`A7` tied to `GND1`.

### I2C addresses (this board)

| Device | Address |
| --- | --- |
| `OLED_0_96_I2C` (`U5`) | `0x3C` or `0x3D` (same class as base; confirm on unit) |

No INA219 or other extra I2C product slaves on this PCB.

### Firmware constant names (target after adapt-copy)

Normative names/values: `docs/board_gpio_bringup_architecture.md`.

```text
BOARD_HIL_NUMBER_STRING = "02"
BOARD_I2C_SCL_GPIO = GPIO0
BOARD_I2C_SDA_GPIO = GPIO1
BOARD_ERROR_LED_GPIO = GPIO8
BOARD_ERROR_LED_ACTIVE_LEVEL = 0
BOARD_FACTORY_RESET_GPIO = GPIO7
BOARD_FACTORY_RESET_ACTIVE_LEVEL = 0
BOARD_OLED_ADDRESS_{0,1} = 0x3C, 0x3D
BOARD_CTRL_OE_GPIO = GPIO2          # /KOE active-low
BOARD_UART_TX_GPIO = GPIO5
BOARD_UART_RX_GPIO = GPIO6
BOARD_TTL_IN_GPIO = GPIO10
BOARD_K1_CTR_GPIO = GPIO21          # TTL/RS-485 mux
BOARD_K2_CTR_GPIO = GPIO9           # termination
BOARD_K3_CTR_GPIO = GPIO3           # pull-down
BOARD_K4_CTR_GPIO = GPIO4           # pull-up
BOARD_485_CTR_GPIO = GPIO20         # MAX485 DE/~RE path
```

`components/board/` owns these constants after implementer adapt-copy. Do not
scatter GPIO literals in drivers.

## Relay / resistor map (PCB-derived; human-confirmed roles)

| Relay | Coil | Function when **energized** | Cold (de-energized) |
| --- | --- | --- | --- |
| `K1` | `U2.O4` ← `GPIO21` | UART mux → **RS-485** (DI/RO) | UART mux → **TTL** on `J2` |
| `K2` | `U2.O3` ← `GPIO9` | **Termination** on (`R_PU`—`R6`—`R_PD`) | Term off |
| `K3` | `U2.O2` ← `GPIO3` | **Pull-down** on B (`R_PD` via `R7` to `GND2`) | PD off |
| `K4` | `U2.O1` ← `GPIO4` | **Pull-up** on A (`R_PU` via `R8` from `+3.3V`) | PU off |

`485_CTR` (`GPIO20`) is **not** a relay: after `U8` it becomes `485_DRV` into
the isolator and then MAX485 DE/`~RE` (tied). Bootstrap holds it inactive
(`0`) after OE enable — see GPIO bring-up doc.

## Connector summary

### `J1` (remote / accessory, GND1 domain)

| Pin | Net |
| --- | --- |
| 1 | `+3V3` |
| 2 | `DISP_SCL` |
| 3 | `GND1` |
| 4 | `DISP_SDA` |
| 5 | `GND1` |
| 6 | `DEFAULTS` |
| 7 | `+3V3` |
| 8 | `TX_LED_EXT` |
| 9 | `+3V3` |
| 10 | `RX_LED_EXT` |
| 11 | `+3V3` |
| 12 | `ERR_LED_EXT` |

### `J2` (DUT, isolated GND2 domain)

| Pin | Net |
| --- | --- |
| 1 | `UART1_TTL_TX` |
| 2 | `UART1_TTL_RX` |
| 3 | `GND2` |
| 4 | `TTL_AUX_IN` |
| 5 | `R_PU` (RS-485 A) |
| 6 | `R_PD` (RS-485 B) |

## Differences from `b00_basehil` hardware profile

- Adds isolated RS-485/TTL path (`ADuM1412`, `MAX485E`, `PS1`)
- Adds four mode/termination relays + `74HCT541` `/KOE` + `ULN2003`
- Adds UART on `GPIO5`/`GPIO6` and DE control on `GPIO20`
- Keeps I2C OLED + error LED + factory-reset GPIO numbers aligned with base
  bench for those three functions only

## Firmware board ownership

- `components/board/` owns pin/address helpers for this tree after adapt-copy
- Display, WiFi, and other modules MUST obtain pins/addresses from board
  configuration
- Pending product semantics use `TODO(b02-hil):` — do not invent DE polarity or
  relay truth tables in code without architect product handoff
