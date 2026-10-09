# 📡 ZeroNode IR Control

> **ESP32-C3 Super Mini + OLED - a compact universal infrared remote for your own compatible devices.**

<p align="center">
  <img alt="ESP32-C3" src="https://img.shields.io/badge/MCU-ESP32--C3-00979D?logo=espressif&logoColor=white" />
  <img alt="OLED" src="https://img.shields.io/badge/Display-SSD1306%20128x64-5B8CFF" />
  <img alt="IR" src="https://img.shields.io/badge/IR-Learn%20%7C%20Replay-FF5C5C" />
  <img alt="Storage" src="https://img.shields.io/badge/Memory-6%20slots-F39C12" />
  <img alt="Expansion" src="https://img.shields.io/badge/Expansion-microSD%20planned-7B61FF" />
</p>

ZeroNode IR Control is a DIY universal infrared remote built around an **ESP32-C3 Super Mini**, a **128x64 I2C OLED**, an IR receiver and an external IR transmitter module. It can learn compatible infrared commands, save them in ESP32 memory and replay the selected command.

The project is designed as a small standalone build: two buttons, an on-device display, local signal storage and no mobile app or external server required.

> [!WARNING]
> Use this project only with equipment that you own or are expressly authorized to control. Verify every module's voltage, current requirement and pin labels before applying power. A high-power IR transmitter must use a suitable power supply and must not be powered from the ESP32 3V3 pin.

## ✨ Features

- 📥 **RAW IR learning** from a demodulating infrared receiver;
- 📤 **RAW IR replay** through an external IR transmitter module;
- 💾 **6 command slots** stored in ESP32 non-volatile memory;
- ❄️ support for longer recorded frames used by many air conditioner remotes;
- 🖥️ OLED status screens for learning, sending, library slots and settings;
- 🎛️ two-button interface for navigation, selection and command sending;
- 🗑️ clearing of a saved command slot;
- 💳 planned microSD expansion for larger device libraries.

## ⚙️ Hardware

| Part | Quantity | Notes |
|---|---:|---|
| ESP32-C3 Super Mini | 1 | Main controller |
| SSD1306 OLED, 128x64, I2C | 1 | Firmware uses I2C address `0x3C` |
| Momentary push buttons | 2 | NEXT and OK |
| Demodulating IR receiver | 1 | Typical 38 kHz receiver module |
| IR transmitter module | 1 | Module with `IN`, `5V` and `GND` |
| TP4056 module | Optional | For a protected 1-cell Li-ion battery |
| 5 V boost converter | Optional | Required when the transmitter needs 5 V from a Li-ion cell |
| microSD SPI module | Planned | For expanded command libraries |

## 🔌 Wiring

This table is checked against the current firmware pin definitions. All modules share a common ground. The buttons use `INPUT_PULLUP`: connect one side of each button to its GPIO and the other side to **GND**. No external pull-up resistors are required.

| Device | Signal | ESP32-C3 Super Mini |
|---|---|---:|
| OLED SSD1306 | VCC | 3V3 |
| OLED SSD1306 | GND | GND |
| OLED SSD1306 | SDA | GPIO5 |
| OLED SSD1306 | SCL | GPIO6 |
| IR transmitter | IN / SIG | GPIO3 |
| IR transmitter | 5V | regulated 5 V supply |
| IR transmitter | GND | GND |
| IR receiver | OUT | GPIO4 |
| IR receiver | VCC | 3V3 |
| IR receiver | GND | GND |
| NEXT button | Signal | GPIO0 |
| OK button | Signal | GPIO1 |

> [!IMPORTANT]
> The current firmware uses **GPIO3** for the transmitter input and **GPIO4** for the receiver output. Do not swap these pins. The transmitter power must come from a supply that can provide the current required by the specific module.

## 🎛️ Controls

| Button | Short press | Long press |
|---|---|---|
| `NEXT` - GPIO0 | Move through menu items or slots | - |
| `OK` - GPIO1 | Select, learn or send | Return to the home screen |

To learn a command, select **Scan**, press `OK`, point the original remote at the receiver, then press the required button on the original remote. The captured signal is saved in the active slot.

## 🧠 How it works

The IR receiver outputs a demodulated timing signal. During learning, ZeroNode IR Control stores the mark and space durations of a compatible remote command. During sending, the saved durations are replayed through the transmitter with a 38 kHz carrier.

This is intentionally RAW replay. It does not claim to identify every remote protocol, recover protected data or guarantee compatibility with every device. Some devices use a different carrier frequency, an unusual timing format or an encrypted protocol.

### Compatibility limits

- **TV and media remotes:** commonly use IR protocols that can be learned and replayed when timing and carrier frequency match.
- **Air conditioner remotes:** often send a complete state packet containing temperature, mode, fan speed and other settings. A recorded RAW signal can replay that exact state, but it does not create a full editable AC interface by itself.
- **Other IR devices:** compatibility depends on the receiver frequency, transmitter strength and original protocol.

## 💳 microSD expansion

An extended device library is planned for a microSD card. The recommended first target is a **4 GB, 8 GB or 16 GB microSD or microSDHC card formatted as FAT32**.

The card will store command-library data, not automatically update the ESP32 firmware. Larger cards may use exFAT and require different filesystem support.

## 📁 Repository layout

```text
zeronode-ir-control/
├── README.md
├── docs/
│   └── pinout.md
└── firmware/
    ├── zeronode-ir-basic.bin
    ├── zeronode-ir-library.bin
    └── zeronode-ir-sd.bin
```

The repository is planned to publish compiled ESP32-C3 firmware images in the `firmware/` folder. The first release may contain only the Basic firmware. Library and microSD versions will be added as they are completed and tested.

## 📜 License

No license has been selected yet. Until one is added, this repository does not grant permission to reuse, modify or redistribute its contents.

---

Built by [@ZeroNodeDIY](https://github.com/ZeroNodeDIY) · ESP32-C3 · IR · OLED
