# Board Hardware Description — `b03_hil`

Particular hardware profile for **this** repository only.

Sources (KiCad, authoritative for pins/nets — **PCB pad↔net map**):

- `../pcb/b03.kicad_pcb` (title `HIL-03`)
- `../pcb/b03.kicad_sch` (supporting; some schematic labels are misplaced
  relative to symbol geometry — do **not** override PCB copper/pinfunction)

Do **not** assume `b00_basehil` pin maps without confirming against this file.
Shared SuperMini patterns that **do** match this PCB: I2C on `GPIO0`/`GPIO1`,
error LED on `GPIO8`, factory-reset on `GPIO7`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b03_hil
Board number: 03
Schematic / PCB references:
  - ../pcb/b03.kicad_sch
  - ../pcb/b03.kicad_pcb
Identity: SoftAP / mDNS HIL-03-<MAC4>
```

## MCU / module

| Item | Value |
| --- | --- |
| Module | ESP32-C3 SuperMini (`U4`) |
| SoC class | ESP32-C3 (RISC-V) |
| ESP-IDF target | `esp32c3` |
| Flash (this module class) | **4 MB** expected (same SuperMini class as base) |
| Role | Runs base pillars + drives six relays through enable buffer |

Flash/partitions: adopt base reference in `docs/flash_partition_architecture.md`
unless a measured module differs. Confirm physical flash on built units.

## Power topology

```text
USB 5V into SuperMini 5V pad (U4.16 = +5V)
  -> board net +5V
  -> U7 AMS1117-3.3 -> board net +3V3
  -> relay coil commons (Kn.A2), ULN2003 COM clamp, OLED VDD,
     /KOE pull-up R18, /DEFAULTS pull-up R1, error LED anode path

U4.14 (module 3V3 pad) is unconnected on this PCB (module regulates locally).
```

Rules:

- Relay coils are referenced to **`+3V3`** (Omron G6K-2F-Y footprint; treat as
  3 V coil class unless BOM says otherwise).
- Board `+3V3` for logic peripherals comes from `U7`, not from the SuperMini
  3V3 pad.

## On-board peripherals

| Ref | Peripheral | Interface | Notes |
| --- | --- | --- | --- |
| `U4` | ESP32-C3 SuperMini | — | MCU |
| `U8` | 74HCT541 octal buffer | GPIO | Relay **control** path enable/buffer |
| `U2` | ULN2003 sink driver | from `U8` Y outs | Coil drivers |
| `K1`…`K6` | Relay DPDT Omron G6K-2F-Y | coils via `U2` | Six independent channels |
| `U5` | OLED 128×64 I2C | I2C | SSD1306-class; series `R9`/`R10` |
| `U7` | AMS1117-3.3 | power | 5 V → 3.3 V |
| `D7` | Error LED | GPIO | Firmware-owned, active-low |
| `D1`…`D6` | Channel LEDs | from `KnDRV` | PCB status; on when drive asserted |
| `SW1` | Push button | GPIO | Factory-reset / `DEFAULTS` |
| `J1` | 1×20 socket | remote | Display / controls / LED sense |
| `J2` | 1×14 socket | relay contacts | NO/C/(NC) to external loads |

## Relay enable path (boot-safe)

GPIOs that command relays do **not** drive coils directly. Path:

```text
MCU KnCTR  -->  U8 74HCT541 An/Yn  -->  KnDRV  -->  U2 ULN2003  -->  Kn coil
                     ^
                     G1 & G2 = /KOE (active-low Output Enable)
```

`/KOE` (`/{slash}KOE` in KiCad netlist):

- Tied to **`U8.G1` and `U8.G2`**
- Pulled **HIGH** to `+3V3` by **`R18`** → buffer outputs **Hi-Z** while MCU
  boots / pins float
- Driven by **`U4.GPIO2`**

### Normative firmware enable sequence

Required before any relay may be energized. Full GPIO order (including
ESP-IDF **level-before-direction**) lives in
`docs/board_relay_control_architecture.md` (`B03_RELAY_OE_LEVEL_BEFORE_DIR`).

Summary:

1. Latch `/KOE` inactive (**high**) in the OUT register, **then** make OE an
   output (never `gpio_config(OUTPUT)` first — OUT defaults to 0 = OE active).
2. Latch all six `KnCTR` **low**, **then** make them outputs.
3. Only then drive `/KOE` **low** to enable `U8`.
4. After that, individual `KnCTR` high energizes the matching relay.

On shutdown / fault paths that must freeze contacts: drive all `KnCTR` low,
then optionally release `/KOE` high. Hardware constraint: **never enable
`/KOE` while any `KnCTR` is high or still an input/floating**, and **never
let the MCU drive `/KOE` low as a side effect of enabling the pad as output**.

## MCU pin map (from PCB pinfunction)

| Signal | MCU GPIO | PCB pad | Dir | Notes |
| --- | --- | --- | --- |
| `K1CTR` | `GPIO21` | 8 | Out | → `U8.A0` |
| `K2CTR` | `GPIO9` | 5 | Out | → `U8.A1` (**strapping pin**) |
| `K3CTR` | `GPIO3` | 12 | Out | → `U8.A2` |
| `K4CTR` | `GPIO4` | 13 | Out | → `U8.A3` |
| `K5CTR` | `GPIO10` | 6 | Out | → `U8.A4` |
| `K6CTR` | `GPIO20` | 7 | Out | → `U8.A5` |
| `/KOE` | `GPIO2` | 11 | Out | Active-low OE; pull-up `R18` (**strapping pin**) |
| `DISP_SCL` | `GPIO0` | 9 | Out | I2C SCL (+ series `R9` to OLED) |
| `DISP_SDA` | `GPIO1` | 10 | I/O | I2C SDA (+ series `R10` to OLED) |
| `ERR_LED` | `GPIO8` | 4 | Out | Active-low (**strapping pin**) |
| `DEFAULTS` | `GPIO7` | 3 | In | Active-low; pull-up `R1` to `+3V3` |
| — | `GPIO5` | 1 | — | Unconnected on PCB |
| — | `GPIO6` | 2 | — | Unconnected on PCB |

### Suggested firmware constants

```text
BOARD_HIL_NUMBER_STRING = "03"
BOARD_I2C_SCL_GPIO = GPIO0
BOARD_I2C_SDA_GPIO = GPIO1
BOARD_ERROR_LED_GPIO = GPIO8
BOARD_ERROR_LED_ACTIVE_LEVEL = 0
BOARD_FACTORY_RESET_GPIO = GPIO7
BOARD_FACTORY_RESET_ACTIVE_LEVEL = 0
BOARD_RELAY_OE_GPIO = GPIO2
BOARD_RELAY_OE_ACTIVE_LEVEL = 0          /* /KOE low = enabled */
BOARD_RELAY_CTR_ACTIVE_LEVEL = 1         /* high after OE enables relay */
BOARD_RELAY1_CTR_GPIO = GPIO21
BOARD_RELAY2_CTR_GPIO = GPIO9
BOARD_RELAY3_CTR_GPIO = GPIO3
BOARD_RELAY4_CTR_GPIO = GPIO4
BOARD_RELAY5_CTR_GPIO = GPIO10
BOARD_RELAY6_CTR_GPIO = GPIO20
```

## `U8` 74HCT541 map

| `U8` pin | Function | Net |
| --- | --- | --- |
| G1, G2 | Output enable (active low) | `/KOE` |
| A0…A5 | Inputs | `K1CTR`…`K6CTR` |
| A6, A7 | Tied | `GND` |
| Y0…Y5 | Outputs | `K1DRV`…`K6DRV` |
| Y6, Y7 | — | Unconnected |
| VCC | — | `+3V3` |

## `U2` ULN2003 → relay coils

| ULN input net | ULN in pad | ULN out pad | Relay coil |
| --- | --- | --- | --- |
| `K1DRV` | I4 (4) | O4 (13) | `K1.A1` (`K1.A2` = `+3V3`) |
| `K2DRV` | I3 (3) | O3 (14) | `K2.A1` |
| `K3DRV` | I2 (2) | O2 (15) | `K3.A1` |
| `K4DRV` | I1 (1) | O1 (16) | `K4.A1` |
| `K5DRV` | I5 (5) | O5 (12) | `K5.A1` |
| `K6DRV` | I6 (6) | O6 (11) | `K6.A1` |

`U2` COM (pad 9) = `+3V3` (kickback clamp). `U2` GND (pad 8) = `GND`.

## Relay contact connector `J2` (1×14)

| `J2` pin | Net | Channel |
| --- | --- | --- |
| 1 | `K1NO` | K1 normally open |
| 2 | `K1C` | K1 common |
| 3 | `K2NO` | K2 NO |
| 4 | `K2C` | K2 C |
| 5 | `K3NO` | K3 NO |
| 6 | `K3C` | K3 C |
| 7 | `K4NO` | K4 NO |
| 8 | `K4C` | K4 C |
| 9 | `K5NC` | K5 normally closed |
| 10 | `K5C` | K5 C |
| 11 | `K5NO` | K5 NO |
| 12 | `K6NC` | K6 NC |
| 13 | `K6NO` | K6 NO |
| 14 | `K6C` | K6 C |

Note: K1–K4 expose **C+NO** only on `J2`; K5–K6 expose **NC+NO+C**. Second
DPDT poles on several relays are unconnected on this PCB.

## Remote connector `J1` (1×20)

| `J1` pin | Net |
| --- | --- |
| 1, 19 | `+3V3` |
| 2 | `DISP_SCL` |
| 3, 5, 7, 9, 11, 13, 15, 17 | `GND` |
| 4 | `DISP_SDA` |
| 6 | `DEFAULTS` |
| 8, 10, 12, 14, 16, 18 | `K1_LED_EXT`…`K6_LED_EXT` |
| 20 | `ERR_LED_EXT` |

Channel LED EXT nets are resistor-tied to the corresponding `KnDRV` (same
assertion as on-board `D1`…`D6`).

## I2C

| Device | Net path | Address |
| --- | --- | --- |
| OLED `U5` | `GPIO0/1` → `R9`/`R10` → `U5.SCL`/`SDA` | TBD (`0x3C` or `0x3D` typical) |

No extra I2C product devices (e.g. INA219) on this board.

## Open questions

- Confirm physical SuperMini flash size on built `b03` units (expect 4 MB).
- Confirm OLED I2C address on hardware.
- Resistor/capacitor **values** are unset in the PCB value fields (`R`/`C`
  placeholders) — freeze BOM values for drive current / LED brightness.
- Confirm G6K coil voltage rating matches `+3V3` rail.
- Strapping: `GPIO2` (`/KOE`, pulled high — good default), `GPIO8` (error LED),
  `GPIO9` (`K2CTR`) are ESP32-C3 strapping pins; validate reliable boot with
  this wiring.
- None remaining for v1 command names (frozen in
  `docs/board_host_api_relay_architecture.md`).
