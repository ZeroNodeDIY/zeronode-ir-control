# zeronode-ir-control
ZeroNode IR Control - a powerful ESP32-C3 universal infrared remote with learning, replay and expandable device libraries.
# ZeroNode IR Control

ZeroNode IR Control is a universal infrared remote controller based on the ESP32-C3 Super Mini.

The project is designed for TVs, air conditioners and other devices that use infrared remote controls. It includes an OLED interface, two-button navigation, IR signal learning, signal replay and support for a high-power external IR transmitter module.

## Current project status

The current firmware is an early working version with these features:

- ZeroNode startup animation on a 0.96 inch SSD1306 OLED
- English on-device menu
- Two-button navigation
- IR signal learning through a demodulating receiver
- Raw signal storage in ESP32 flash
- Signal replay through the external IR transmitter
- Six internal command slots
- Support for long raw frames used by many air conditioner remotes

The current source file is kept locally in `private-source/` for development. It should not be uploaded to the public repository. Public releases are planned as compiled `.bin` files.

## Planned firmware versions

The project may be released in several firmware variants:

1. Basic firmware with no preloaded device library
2. Firmware with a small built-in library for common TV brands
3. Firmware with an extended library loaded from a microSD card

The long-term goal is one universal firmware that uses the built-in library when no card is present and automatically loads an extended library when a compatible microSD card is detected.

## Hardware

- ESP32-C3 Super Mini
- 0.96 inch 128x64 SSD1306 I2C OLED
- Two momentary push buttons
- Demodulating IR receiver module
- Ready-made high-power IR transmitter module with `IN`, `5V` and `GND`
- Optional TP4056 charging and protection module
- Optional single-cell Li-ion battery
- Optional 5 V boost converter for the transmitter and ESP32 5 V input
- Optional microSD SPI module

## Pinout

| Component | Pin | ESP32-C3 pin |
| --- | --- | --- |
| OLED | VCC | 3V3 |
| OLED | GND | GND |
| OLED | SDA | GPIO5 |
| OLED | SCL | GPIO6 |
| Next button | one side | GPIO0 |
| Next button | other side | GND |
| OK button | one side | GPIO1 |
| OK button | other side | GND |
| IR transmitter | IN / SIG | GPIO3 |
| IR transmitter | GND | GND |
| IR transmitter | 5V | regulated 5 V supply |
| IR receiver | OUT | GPIO4 |
| IR receiver | VCC | 3V3 |
| IR receiver | GND | GND |

The transmitter input is GPIO3 in the current firmware. GPIO4 is reserved for the future IR receiver output.

## Power and safety

The high-power IR transmitter must not be powered from the ESP32 3V3 output. Use a stable supply that matches the markings on the transmitter module. If the module requires 5 V, use a 5 V boost converter when running from a single Li-ion cell.

For a TP4056 charging circuit:

```text
Battery positive  -> TP4056 B+
Battery negative  -> TP4056 B-
TP4056 OUT+       -> power switch -> boost converter IN+
TP4056 OUT-       -> common GND   -> boost converter IN-
Boost OUT+ 5 V    -> ESP32 5V and transmitter 5V
Boost OUT-        -> common GND
```

The transmitter module must be checked before use. A 3 W rating can require a substantial current, and the ESP32 5 V pin or a USB source may not be suitable as its power supply.

## microSD plan

The recommended first target is a 4 GB, 8 GB or 16 GB microSD card formatted as FAT32. Very large cards may use exFAT and may require a different filesystem library. Cheap cards should be tested because their advertised capacity may be inaccurate.

The microSD card is planned for command library data. It does not automatically update the ESP32 firmware unless a separate firmware update feature is implemented.

## Releases

Firmware binaries will be published through GitHub Releases, for example:

```text
zeronode-ir-basic.bin
zeronode-ir-library.bin
zeronode-ir-sd.bin
```

The `.bin` files are compiled firmware images. The source code is currently kept private during development.

## License

This repository currently has no open-source license. Unless a separate written permission is provided, the firmware binaries, branding, documentation and project assets may not be copied, modified, redistributed or used in derivative products.

Copyright (c) 2026 ZeroNodeDIY.

## Disclaimer

This is a DIY electronics project. Verify the voltage, current and pin labels of every module before connecting power. The author is not responsible for damage caused by incorrect wiring, unsuitable power supplies or modified firmware.
