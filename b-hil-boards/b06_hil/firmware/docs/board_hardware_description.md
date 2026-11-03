# Board Hardware Description — `b06_hil`

Particular hardware profile for **this** repository only.

Sources (KiCad 7, authoritative for pins/nets):

- `../pcb/b06.kicad_sch`
- `../pcb/b06.kicad_pcb` (title `HIL-06`)
- Datasheet asset: `../pcb/assets/pdf/ina219.pdf`

Do **not** assume `b00_basehil` pin maps without confirming against this file.
For the MCU GPIOs that this board does use, they match the common SuperMini
I2C / error-LED / factory-reset pattern, with **three INA219** slaves added.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b06_hil
Schematic / PCB references:
  - ../pcb/b06.kicad_sch
  - ../pcb/b06.kicad_pcb
```

## MCU / module

| Item | Value |
| --- | --- |
| Module | ESP32-C3 SuperMini (`U4`) |
| SoC class | ESP32-C3 (RISC-V) |
| ESP-IDF target | `esp32c3` |
| Flash (this module) | **4 MB** (same SuperMini class as base reference) |
| Role | Runs base pillars + reads three INA219 channels |

Flash size and partition table for this board firmware **adopt the base
reference** in `docs/flash_partition_architecture.md` (`partitions.csv`: large
single `factory` app at `0x3F0000`, NVS at `0x9000`, `CONFIG_ESPTOOLPY_FLASHSIZE`
= `4MB`). Do **not** keep older 1 MB factory / 2 MB flash assumptions. Implementer
must ensure `firmware/partitions.csv` and `sdkconfig.defaults` match that
contract (architect docs only; no build in architect role).

## Power topology (passive splitter)

```text
USB-C IN (J1) VBUS
    --net /PSWI-->
  [JP1 solder jumper to +5V]  and/or  J5 pin16 (+5V) / pin15 (PSWI)
    --net +5V-->
  ESP32-C3 SuperMini 5V pad
  + polyfuse F1 --> channel path U1 (silk OUT 3)
  + polyfuse F2 --> channel path U2 (silk OUT 2)
  + polyfuse F3 --> channel path U3 (silk OUT 1)

Per channel (example U1):
  +5V --F--> INA219 IN+ --shunt--> INA219 IN- --VBUS--> USB-C OUT
```

Rules:

- This is a **passive** 5 V split: no onboard regulator between IN and OUTs.
- `JP1` is a **normally open** solder jumper between `/PSWI` (J1 VBUS) and
  `+5V`. For standalone use, close `JP1` (or otherwise bond PSWI to +5V, e.g.
  via `J5`) so the input feeds the board rail.
- `+3.3V` for INA219 `VS`, OLED `VDD`, pull-ups, and error LED anode comes from
  the SuperMini `3V3` pad.

## On-board peripherals

| Ref | Peripheral | Interface | Notes |
| --- | --- | --- | --- |
| `J1` | USB-C receptacle (power-only) | Power | **Input** — silk context / schematic note `USB ONLY-POWER IN` |
| `J2` | USB-C receptacle (power-only) | Power | **Output silk OUT 1** |
| `J3` | USB-C receptacle (power-only) | Power | **Output silk OUT 2** |
| `J4` | USB-C receptacle (power-only) | Power | **Output silk OUT 3** |
| `U1` | INA219AxD | I2C `0x40` | Measures silk **OUT 3** (`J4`) |
| `U2` | INA219AxD | I2C `0x41` | Measures silk **OUT 2** (`J3`) |
| `U3` | INA219AxD | I2C `0x42` | Measures silk **OUT 1** (`J2`) |
| `U5` | OLED 128×64 I2C | I2C | SSD1306-class panel; series R9/R10 to bus |
| `D3` | Error LED | GPIO | **Firmware-owned** — active-low from MCU |
| `SW1` | Push button | GPIO | Factory-reset / `DEFAULTS` |
| `F1`/`F2`/`F3` | Polyfuse | Power | One per output channel |
| `J5` | 1×16 socket | Remote | Remote display / controls (firmware-relevant pins only) |
| `JP1` | 2-pad solder jumper | Power | Bridges `/PSWI` to `+5V` when closed |

### Firmware-irrelevant LEDs (PCB only)

These LEDs are **not connected to the MCU** and have **no firmware
significance**. Do not model, drive, poll, expose via host API, or mention them
in implementer acceptance criteria:

- `D1`, `D2`, `D4` — passive channel power-present indicators
- Nets `/OUT_PW_LED1`, `/OUT_PW_LED2`, `/OUT_PW_LED3` and `J5` pins 10/12/14 —
  remote copies of those passive indicators

They may remain on the PCB for human visual cues only.

## Channel map (canonical for firmware / host UX)

**User-facing channel numbers follow PCB silkscreen** (`OUT 1` / `OUT 2` /
`OUT 3`).

| Silk channel | USB-C connector | INA219 | I2C addr | Address pins | Polyfuse |
| --- | --- | --- | --- | --- | --- |
| **OUT 1** | `J2` | `U3` | `0x42` | `A1=GND`, `A0=SDA` | `F3` |
| **OUT 2** | `J3` | `U2` | `0x41` | `A1=GND`, `A0=+3.3V` | `F2` |
| **OUT 3** | `J4` | `U1` | `0x40` | `A1=GND`, `A0=GND` | `F1` |
| **IN** | `J1` | — | — | — | — |

INA219 addresses match schematic annotations `0x40` / `0x41` / `0x42` and the
datasheet A0/A1 encoding (GND / VS / SDA / SCL).

Shunt resistors (schematic `R6`/`R5`/`R8` between each IN+/IN−) exist but
**component values are not filled** in the KiCad properties (`Value` remains
`R`). BOM ohms remain an **open question**. For firmware bring-up of handoff
`B06_MAIN_METERING_OLED`, use provisional calibration constant
`BOARD_INA219_SHUNT_OHMS = 0.1f` (same value for all three channels) until BOM
confirms `R5`/`R6`/`R8`. Wrong shunt skews current accuracy; it does not block
OLED layout bring-up. See `docs/board_main_metering_oled_architecture.md`.

## MCU pin map (`U4` ESP32-C3 SuperMini)

Confirmed from PCB pad nets:

| Signal name | MCU pin | Direction | Net | Notes |
| --- | --- | --- | --- | --- |
| `I2C_SCL` / `DISP_SCL` | `GPIO0` | Output (clock) | `/DISP_SCL` | Shared I2C SCL |
| `I2C_SDA` / `DISP_SDA` | `GPIO1` | Bidirectional | `/DISP_SDA` | Shared I2C SDA |
| `FACTORY_RESET` / `DEFAULTS` | `GPIO7` | Input | `/DEFAULTS` | `SW1` to GND; pull-up `R1` to `+3.3V`; active-low |
| `ERROR_LED` / `ERR_LED` | `GPIO8` | Output | `/ERR_LED` | Active-low; `D3` anode on `+3.3V`, cathode via `R15` |

Unused / unconnected on this PCB (do not claim product functions):

`GPIO2`, `GPIO3`, `GPIO4`, `GPIO5`, `GPIO6`, `GPIO9`, `GPIO10`, `GPIO20`,
`GPIO21`.

Power pads on module: `5V` ← `+5V`, `GND`, `3V3` → board `+3.3V`.

### Firmware constant names (planned)

```text
BOARD_HIL_NUMBER_STRING = "06"
BOARD_I2C_SCL_GPIO = GPIO0
BOARD_I2C_SDA_GPIO = GPIO1
BOARD_ERROR_LED_GPIO = GPIO8
BOARD_ERROR_LED_ACTIVE_LEVEL = 0
BOARD_FACTORY_RESET_GPIO = GPIO7
BOARD_FACTORY_RESET_ACTIVE_LEVEL = 0
BOARD_INA219_OUT1_ADDR = 0x42   # silk OUT 1 / U3
BOARD_INA219_OUT2_ADDR = 0x41   # silk OUT 2 / U2
BOARD_INA219_OUT3_ADDR = 0x40   # silk OUT 3 / U1
BOARD_INA219_SHUNT_OHMS = 0.1f  # provisional; confirm BOM R5/R6/R8
BOARD_OLED_ADDRESS = TBD        # typically 0x3C or 0x3D; not labeled on sch
```

Main metering OLED (STA connected): hostname + per-channel V/I — normative
contract in `docs/board_main_metering_oled_architecture.md`.

## Shared I2C bus

| Device | Address | Role |
| --- | --- | --- |
| `U1` INA219 | `0x40` | Silk OUT 3 |
| `U2` INA219 | `0x41` | Silk OUT 2 |
| `U3` INA219 | `0x42` | Silk OUT 1 |
| `U5` OLED | TBD (`0x3C`/`0x3D`) | Local display |

Bus rules follow base `docs/i2c_bus_architecture.md`. On-board OLED attaches
through series resistors:

- `R9`: `/DISP_SCL` ↔ `Net-(U5-SCL)`
- `R10`: `/DISP_SDA` ↔ `Net-(U5-SDA)`

Schematic note: *DISCONNECT RESISTORS TO USE REMOTE DISPLAY* — removing R9/R10
isolates `U5` so a remote panel on `J5` can own the bus.

## Error LED and factory reset

| Function | Detail |
| --- | --- |
| Error LED `D3` | Anode `+3.3V`, cathode ← `R15` ← `/ERR_LED` ← `GPIO8` (active-low) |
| Remote error | `/ERR_LED` → `R16` → `/ERR_LED_EXT` → `J5` pin 8 |
| Factory reset `SW1` | `/DEFAULTS` (`GPIO7`) pulled to GND when pressed; `R1` pull-up to `+3.3V` |
| Hold time | Follow base `docs/wifi_factory_reset_architecture.md` (10 s continuous) |

## Remote connector `J5` (1×16)

| Pin | Net | Purpose |
| --- | --- | --- |
| 1 | `+3.3V` | Logic rail |
| 2 | `/DISP_SCL` | I2C SCL |
| 3 | `GND` | Ground |
| 4 | `/DISP_SDA` | I2C SDA |
| 5 | `GND` | Ground |
| 6 | `/DEFAULTS` | Factory-reset sense (shared with `SW1`) |
| 7 | `+3.3V` | Logic rail |
| 8 | `/ERR_LED_EXT` | Remote error LED drive (via `R16`) — MCU-driven path |
| 9 | `GND` | Ground |
| 10 | `/OUT_PW_LED1` | **Firmware-irrelevant** passive LED net |
| 11 | `GND` | Ground |
| 12 | `/OUT_PW_LED2` | **Firmware-irrelevant** passive LED net |
| 13 | `GND` | Ground |
| 14 | `/OUT_PW_LED3` | **Firmware-irrelevant** passive LED net |
| 15 | `/PSWI` | Input USB VBUS (pre-jumper) |
| 16 | `+5V` | Board 5 V rail |

Silk on PCB also labels `PWR_SWI_OUT` for the PSWI-related remote power path.

## Digestible wiring (MCU side)

```text
                      ESP32-C3 SuperMini (U4)
                    +---------------------------+
    I2C SCL  <------| GPIO0  /DISP_SCL          |---- INA219 x3 + OLED (+ J5)
    I2C SDA  <----->| GPIO1  /DISP_SDA          |
    Error LED <-----| GPIO8  /ERR_LED           |---- D3 (active-low)
    Reset SW  ----->| GPIO7  /DEFAULTS          |---- SW1 to GND
                    | 5V, 3V3, GND              |
                    +---------------------------+

USB-C:  J1=IN , J2=OUT1 , J3=OUT2 , J4=OUT3
I2C:    U3=0x42 (OUT1), U2=0x41 (OUT2), U1=0x40 (OUT3), U5=OLED
```

## Differences from `b00_basehil` minimum bench

| Topic | `b00_basehil` | `b06_hil` |
| --- | --- | --- |
| Product role | Base bring-up bench | Passive USB 5 V 1→3 splitter + metering |
| Extra I2C | OLED only | OLED + three INA219 |
| Power path | Bench supply | USB-C IN/OUT split with polyfuses |
| MCU GPIOs used | GPIO0/1/7/8 | Same set |
| Flash / partitions | 4 MB, large factory (`flash_partition_architecture.md`) | Same contract adopted |
| Board number | `00` | `06` |

## Firmware board ownership

- `components/board/` (when created) owns pins, HIL number `"06"`, INA219
  address table keyed by **silk channel**, and **error** LED / reset polarity
  only (`D3` / `GPIO8`). Ignore `D1`/`D2`/`D4` and `OUT_PW_LED*` in firmware.
- INA219 device drivers are **particular** optional components; the shared
  `i2c_bus` contract stays base-aligned.
- Do not hard-code GPIO literals outside `components/board/`.

## Open questions

1. Exact BOM values for shunt resistors (`R5`/`R6`/`R8`), LED resistors,
   polyfuses, and decoupling caps (schematic `Value` fields still generic).
   Firmware provisional: `BOARD_INA219_SHUNT_OHMS = 0.1f` until confirmed.
2. Confirmed OLED I2C address (`0x3C` vs `0x3D`) on built hardware.
3. Production expectation for `JP1` (closed by default vs left open for remote
   PSWI control via `J5`).
4. Host API metering commands frozen in
   `docs/board_host_api_metering_architecture.md` (`meter_read`,
   `meter_read_all`). Shunt accuracy still tracks open question (1).
