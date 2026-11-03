# Board Hardware Description — `b00_basehil`

Particular hardware profile for **this** repository only. Other family boards
replace this file with their MCU, peripherals, and pin map while keeping the
same documentation philosophy.

Pin and polarity detail for the SuperMini bench also appears in
`docs/esp32_c3_supermini_connections.md`. Firmware constants live in
`components/board/include/board_pins.h`. **This particular test profile does
not include INA219 (or other extra I2C sensors).** Sibling boards may document
INA219 or other additional I2C devices in **their** hardware description; do
not infer them from this file.

Source handoff: `agent-workspaces/architect/handoff.md`, `HIL_BASE_REUSE_CONTRACT`
(particular filled example for the base tree).

Template: `docs/hil_board_particular_spec_template.md`.

All agentic development methodology documentation in this firmware tree is
maintained in English.

```text
Board ID: b00_basehil
Schematic / PCB references:
  - ../pcb/ (board PCB tree)
  - docs/esp32_c3_supermini_connections.md (SuperMini connection map; for this
    test profile, ignore any INA219 entries — not fitted on the minimum bench)
```

## MCU / module

| Item | Value |
| --- | --- |
| Module | ESP32-C3 SuperMini |
| SoC class | ESP32-C3 (RISC-V) |
| ESP-IDF target | `esp32c3` |
| Role | Minimum MCU for human-supervised base bring-up and pillar reference |

Other `bNN_hil` boards MUST NOT assume this MCU. They document their own module
in their own `board_hardware_description.md`.

## On-board / bench peripherals (this profile)

| Peripheral | Interface | Notes |
| --- | --- | --- |
| 0.96 inch OLED | I2C | SSD1306, 128×64; address `0x3C` or `0x3D` (v1 validated SSD1306) |
| Error LED | GPIO | Active-low to positive rail |
| Factory reset switch | GPIO | Active-low with pull-up; 10 s hold per factory-reset architecture |

**Not part of this test bench:** INA219 current monitors or other extra I2C
devices. Family boards that need them (or different I2C slaves) list them only
in that board's `board_hardware_description.md`.

## Power and reference

Shared positive rail and GND for OLED, error LED anode path, and GPIO7 pull-up
as described for those nets in `docs/esp32_c3_supermini_connections.md`.

## Pin map (this minimum test profile)

| Signal name | MCU pin / pad | Direction | Notes |
| --- | --- | --- | --- |
| `I2C_SCL` | `GPIO0` | Output (clock) | Shared I2C SCL |
| `I2C_SDA` | `GPIO1` | Bidirectional | Shared I2C SDA |
| `ERROR_LED` | `GPIO8` | Output | Active-low (`0` = LED on) |
| `FACTORY_RESET_SWITCH` | `GPIO7` | Input | Active-low pressed (`0`); pull-up when released |

### I2C addresses on the shared bus (this profile)

| Device | Address |
| --- | --- |
| `OLED_0_96_I2C` | `0x3C` or `0x3D` |

### Firmware constant names used by this profile

```text
BOARD_I2C_SCL_GPIO = GPIO0
BOARD_I2C_SDA_GPIO = GPIO1
BOARD_ERROR_LED_GPIO = GPIO8
BOARD_ERROR_LED_ACTIVE_LEVEL = 0
BOARD_FACTORY_RESET_GPIO = GPIO7
BOARD_FACTORY_RESET_ACTIVE_LEVEL = 0
BOARD_OLED_ADDRESS_{0,1} = 0x3C, 0x3D
BOARD_HIL_NUMBER_STRING = "00"
```

`board_pins.h` may still define additional symbols for other experiments or
future boards; they are **out of scope** for this particular test-hardware
description until listed here.

## Digestible wiring (this board)

```text
                      ESP32-C3 SuperMini
                    +---------------------+
    I2C SCL  <------| GPIO0               |
    I2C SDA  <----->| GPIO1               |
    Error LED <-----| GPIO8               |  active-low
    Reset SW  ----->| GPIO7               |  active-low + pull-up
                    +---------------------+

Shared I2C (this test profile): OLED only (0x3C/0x3D)
```

## Differences from other family boards

This file **is** the `b00_basehil` minimum test profile. Sibling boards list how
they differ in **their** hardware description (different MCU, extra I2C devices
such as INA219, missing OLED, extra transceiver, different reset GPIO, etc.).

## Firmware board ownership

- `components/board/` owns pin/address helpers for this tree.
- Display, WiFi, and other modules MUST obtain pins/addresses from board
  configuration, not hard-coded GPIO literals scattered in drivers.
- No driver or `board_pins.h` cleanup is required solely because INA219 is
  absent from this particular description; other boards may still use I2C
  extras.
