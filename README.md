# Flight Software Mastodonte

**FlightSoftware-Mastodonte** is the embedded flight software running on the **YD-RP2040** controller of the experimental rocket **Mastodonte**.  
It acts as the onboard **flight sequencer**, managing time-critical events and hardware interfaces during the mission.

> © 2025 Paul Miailhe – Designed for safety-critical embedded rocket systems.

---

<p align="center">
  <img src="docs/Mastodonte-N6.png" alt="Carte Mastodonte" width="800"/>
</p>

---

## ✦ Features

- Arduino framework (Earle Philhower core) on RP2040
- Built-in WS2812B RGB LED control (no external wiring)
- Passive buzzer management with programmable tone and timing
- OLED display via I²C with auto-detection
- Debug interface with optional serial logging (timeout-protected)
- CMSIS-DAP support for flashing and debugging (via SWD)
- Modular C++ architecture (timers, state machines, IO abstraction)
- Compatible with 16MB flash layout (1MB sketch / 15MB filesystem)

---

## ✦ Platform Configuration

| Parameter           | Value                    |
|---------------------|--------------------------|
| Board               | `YD-RP2040`              |
| MCU                 | RP2040                   |
| Framework           | Arduino (earlephilhower) |
| Debug protocol      | CMSIS-DAP (SWD)          |
| Flash layout        | 1MB firmware / 15MB FS   |
| Clock frequency     | 133 MHz                  |
| USB stack           | TinyUSB                  |
| Toolchain           | `toolchain-rp2040-earlephilhower` |
| Build system        | PlatformIO               |

---

## ✦ External Libraries

Declared in `platformio.ini`:

| Library                      | Version   | Purpose                      |
|-----------------------------|-----------|------------------------------|
| Adafruit NeoPixel           | `1.12.5`  | WS2812B LED control          |
| Adafruit SSD1306            | `2.5.7`   | OLED I²C display             |
| Adafruit GFX                | `1.11.9`  | Core graphics (OLED backend) |

---
