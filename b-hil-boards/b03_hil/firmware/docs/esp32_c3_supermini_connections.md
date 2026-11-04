# ESP32-C3 SuperMini Electrical Connections

This document records the confirmed electrical connection map for the
`b00_basehil` firmware target using an `ESP32-C3 SuperMini`.

Source handoff: `agent-workspaces/architect/handoff.md`,
`OLED_TEXT_DISPLAY_INTERFACE`.

## Connector Summary

| ESP32-C3 SuperMini pin | Signal | Electrical behavior | Firmware meaning |
| --- | --- | --- | --- |
| `GPIO0` | `I2C_SCL` | Shared I2C clock line. | Configure as I2C SCL. |
| `GPIO1` | `I2C_SDA` | Shared I2C data line. | Configure as I2C SDA. |
| `GPIO8` | `ERROR_LED` | LED is connected to the positive rail and is active-low. | Drive `0` to turn the error LED on; drive `1` to turn it off. |
| `GPIO7` | `FACTORY_RESET_SWITCH` | Input has pull-up to the positive rail. The switch pulls the input low when pressed. | Read `0` as pressed/request factory reset; read `1` as released. |

## I2C Bus

`GPIO0` and `GPIO1` form one shared I2C bus.

### Minimum base test profile (normative for `b00_basehil` bench)

The human-supervised base bring-up bench uses **OLED only** on this bus. See
`docs/board_hardware_description.md`.

| Device | I2C address |
| --- | --- |
| `OLED_0_96_I2C` | `0x3C` or `0x3D` |

The OLED driver or board configuration must allow either common OLED address:
`0x3C` or `0x3D`. The firmware may select a configured address or probe only
these two expected OLED addresses.

**Controller IC (v1):** SSD1306, 128×64, I2C. Validated on bench hardware. SH1106
is out of scope unless the physical module changes.

### Optional / daughter-board I2C devices (not on the base test bench)

Product HIL boards (`bNN_hil`) MAY add further slaves on the same shared-bus
architecture (for example three `INA219` at `0x40`, `0x41`, `0x42`, or other
I2C devices). Those devices are **not** part of the `b00_basehil` minimum test
profile. Document them only in that board's
`docs/board_hardware_description.md` and optional device components.

Historical address set used in early INA219 experiments (daughter-board
example only):

| Device (example) | I2C address |
| --- | --- |
| `INA219_0` | `0x40` |
| `INA219_1` | `0x41` |
| `INA219_2` | `0x42` |

## Digestible Wiring Schematic

```text
                      ESP32-C3 SuperMini
                    +---------------------+
                    |                     |
    I2C SCL  <------| GPIO0               |------> Shared I2C SCL
    I2C SDA  <----->| GPIO1               |<-----> Shared I2C SDA
                    |                     |
    Error LED <-----| GPIO8               |
                    |                     |
    Reset SW  ----->| GPIO7               |
                    |                     |
                    +---------------------+

Shared I2C SCL/SDA bus (minimum base test profile):

    GPIO0 / SCL ----------------+
                                |
                              OLED
                           0x3C or 0x3D
                                |
    GPIO1 / SDA ----------------+

    (Daughter boards MAY attach more I2C slaves on the same bus architecture;
     they are not shown here because they are not part of the base test bench.)

Power and reference (minimum profile):

    Positive rail -------------+-------------+-------------+
                               |             |             |
                              OLED          LED          GPIO7 pull-up

    GND -----------------------+-------------+
                               |             |
                              OLED        ESP32-C3 GND

Active-low discrete signals:

    Positive rail ---- LED ---- GPIO8
                         LED on when GPIO8 = 0
                         LED off when GPIO8 = 1

    Positive rail ---- pull-up ---- GPIO7 ---- switch ---- GND
                         switch released: GPIO7 = 1
                         switch pressed:  GPIO7 = 0
```

## Implementation Rules

- `GPIO0` is the only confirmed SCL pin for this board target.
- `GPIO1` is the only confirmed SDA pin for this board target.
- On the **minimum base test profile**, only the OLED is required on the shared
  I2C bus.
- Extra I2C slaves (INA219 or others) belong to daughter-board hardware docs and
  optional components; they MUST NOT be treated as required for `b00_basehil`
  bring-up.
- The OLED display address is valid only when it is `0x3C` or `0x3D`.
- `GPIO8` must be modeled as an active-low output.
- Product WiFi link status drives the error LED via `wifi_link_status_t` and
  `docs/error_led_wifi_link_architecture.md` (slow blink = unprovisioned, solid ON
  = saved connect in progress, off = connected, fast blink = disconnected with
  creds).
- `GPIO7` must be modeled as an active-low input with pull-up behavior.
- A pressed factory reset switch is represented by logical `0`.
- A released factory reset switch is represented by logical `1`.

## Firmware Constants

Use names equivalent to these constants in board-specific configuration:

```text
BOARD_I2C_SCL_GPIO = GPIO0
BOARD_I2C_SDA_GPIO = GPIO1
BOARD_ERROR_LED_GPIO = GPIO8
BOARD_ERROR_LED_ACTIVE_LEVEL = 0
BOARD_FACTORY_RESET_GPIO = GPIO7
BOARD_FACTORY_RESET_ACTIVE_LEVEL = 0

BOARD_OLED_ALLOWED_ADDRESSES = [0x3C, 0x3D]
```

Daughter boards that use INA219 (or other I2C devices) define their own address
macros in that board's `components/board/` and hardware description. Those
macros are not required on the `b00_basehil` minimum test profile.

These constants are board details. Display rendering code must not depend on
GPIO numbers directly; it should receive hardware details from board or driver
configuration.
