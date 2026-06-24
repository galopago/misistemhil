# ESP32-C3 SuperMini Electrical Connections

This document records the confirmed electrical connection map for the
`b06_hil` firmware target using an `ESP32-C3 SuperMini`.

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

`GPIO0` and `GPIO1` form one shared I2C bus. The 0.96 inch I2C OLED display and
the three `INA219` current monitor devices are connected to this same bus.

| Device | I2C address |
| --- | --- |
| `INA219_0` | `0x40` |
| `INA219_1` | `0x41` |
| `INA219_2` | `0x42` |
| `OLED_0_96_I2C` | `0x3C` or `0x3D` |

The OLED driver or board configuration must allow either common OLED address:
`0x3C` or `0x3D`. The firmware may select a configured address or probe only
these two expected OLED addresses.

**Controller IC (v1):** SSD1306, 128×64, I2C. Validated on bench hardware. SH1106
is out of scope unless the physical module changes.

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

Shared I2C SCL/SDA bus:

    GPIO0 / SCL ----------------+----------------+----------------+----------------+
                                |                |                |                |
                              OLED            INA219           INA219           INA219
                           0x3C or 0x3D        0x40             0x41             0x42
                                |                |                |                |
    GPIO1 / SDA ----------------+----------------+----------------+----------------+

Power and reference:

    Positive rail -------------+-------------+-------------+-------------+
                               |             |             |             |
                              OLED          INA219        LED          GPIO7 pull-up

    GND -----------------------+-------------+-------------+-------------+
                               |             |             |
                              OLED          INA219        ESP32-C3 GND

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
- The OLED display and all three `INA219` devices share the same I2C bus.
- The `INA219` devices use fixed expected addresses `0x40`, `0x41`, and `0x42`.
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

BOARD_INA219_ADDRESSES = [0x40, 0x41, 0x42]
BOARD_OLED_ALLOWED_ADDRESSES = [0x3C, 0x3D]
```

These constants are board details. Display rendering code must not depend on
GPIO numbers directly; it should receive hardware details from board or driver
configuration.
