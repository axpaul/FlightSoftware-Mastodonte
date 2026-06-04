# Flight Software Mastodonte

**FlightSoftware-Mastodonte** is the embedded flight software running on the **YD-RP2040** controller of the experimental rocket **Mastodonte**.  
It acts as the onboard **flight sequencer**, managing time-critical events and hardware interfaces during the mission.

> © 2025 Paul Miailhe – Designed for safety-critical embedded rocket systems.

---

<p align="center">
  <img src="docs/Mastodonte-N6.png" alt="Carte Mastodonte" width="800"/>
</p>

---

## Features

- Arduino framework (Earle Philhower core) on RP2040
- Non-blocking continuous battery voltage monitoring via hardware timer (1 Hz frequency)
- Smart LED battery warning (alternating state color/red blinking when VCC <= 6.0V)
- Built-in WS2812B RGB LED control (no external wiring)
- Passive buzzer management with programmable tone and timing
- OLED display via I²C with auto-detection
- Debug interface with optional serial logging (timeout-protected)
- CMSIS-DAP support for flashing and debugging (via SWD)
- Modular C++ architecture (timers, state machines, IO abstraction)
- Compatible with 16MB flash layout (1MB sketch / 15MB filesystem)

---

## Software Architecture

The flight software is built with a highly modular C++ structure to enforce separation of concerns, safety-critical execution, and predictability.

```mermaid
graph TD
    Main[main.cpp <br/> Orchestrator] -->|Initializes & Ticks| Sys[system.cpp <br/> Hardware Abstraction & Health]
    Main -->|Ticks| Seq[sequencer.cpp <br/> Flight Sequencer FSM]
    Seq -->|Logs events| Log[log.cpp <br/> LittleFS Flash Logging]
    Seq -->|Triggers| Buzzer[buzzer.cpp <br/> PWM audio feedback]
    Seq -->|Controls| Motors[drv8872.cpp <br/> Recovery H-Bridge]
```

### Module Breakdown

- **`main.cpp` (Orchestrator)**: The main entry point. It handles high-level hardware initialization in `setup()` and schedules tasks (sequencer execution, background battery checks) in `loop()`.
- **`sequencer.cpp` (Flight Logic)**: Manages the Finite State Machine (FSM) that drives the flight sequence (from `PRE_FLIGHT` to `TOUCHDOWN`). It handles external interrupts (RBF, Jack, Octocouplers) and schedules mission-critical events.
- **`system.cpp` (Hardware Abstraction)**: Initializes GPIOs and ADCs. It also runs the background battery check via a repeating hardware timer and controls the onboard WS2812B RGB LED.
- **`buzzer.cpp` (Audio Feedback)**: Drives the passive buzzer using a repeating timer to play non-blocking sound patterns corresponding to each flight state.
- **`log.cpp` (Logging System)**: Manages flash operations using LittleFS to write flight reports. Writes are deferred to state transitions to avoid blocking critical flight logic.
- **`drv8872.cpp` (Recovery Driver)**: Drives the H-bridges to control recovery motors during deployment.
- **`platform.h` (Hardware Mapping)**: Central board configuration mapping GPIO pins to flight functions.

---

## Platform Configuration

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
| Battery Monitor Pin | GP28 (ADC2)              |

---

## External Libraries

Declared in `platformio.ini`:

| Library                      | Version   | Purpose                      |
|-----------------------------|-----------|------------------------------|
| Adafruit NeoPixel           | `1.12.5`  | WS2812B LED control          |
| Adafruit SSD1306            | `2.5.7`   | OLED I²C display             |
| Adafruit GFX                | `1.11.9`  | Core graphics (OLED backend) |

---

## Synoptic

<p align="center">
  <img src="docs/Mastodonte_synoptique.png" alt="Synoptique Mastodonte" width="750"/>
</p>

---

## 🚦 Flight Sequencer States

The system is driven by a finite state machine (`sequencer.cpp`) that transitions through various mission phases.  
Each state configures RGB LED color and buzzer behavior to provide **visual and audible feedback**.

| State           | Color (LED)      | Buzzer Pattern                      | Description |
|------------------|------------------|--------------------------------------|-------------|
| `PRE_FLIGHT`     | 🟢 Green          | 🔈 Double soft beep (3s pause)       | System idle on ground, RBF and JACK expected. |
| `PYRO_RDY`       | 🟡 Yellow         | 🔈 1 low beep per second             | Ready for liftoff, RBF removed, JACK still in. |
| `ASCEND`         | 🔵 Blue           | 🔈 Very fast beeping                 | Liftoff confirmed, rocket in ascent. |
| `WINDOW`         | 🔵 Cyan           | 🔈 Rapid alert beeping               | Deployment window is open (timed or triggered). |
| `DEPLOY_ALGO`    | 🟠 Orange         | 🔈 Alternating mid beeps             | Deployment triggered via algorithm (sensor). |
| `DEPLOY_TIMER`   | 🟠 Orange         | 🔈 Alternating mid beeps             | Deployment triggered via timer timeout. |
| `DESCEND`        | 🟣 Magenta        | 🔈 Slow and regular beeping          | Descent under parachute. |
| `TOUCHDOWN`      | 🟢 Green (steady) | 🔈 Long beep every 30 seconds        | Touchdown detected, safe recovery state. |
| `ERROR_SEQ`      | 🔴 Red            | 🔈 Rapid high-pitched beeping        | System fault or invalid state transition. |

> [!TIP]
> **Battery Warning Indication:**
> If the battery voltage drops below **6.0V** during flight preparation (Pre-flight / Pyro Ready), the RGB LED will alternate between the **current state color** and **Red** every second. This alert is non-blocking to allow manual override but provides clear visual feedback of a low battery.

---

## 🔘 User Button USR (GP24) — Log Dump & Erase

A user-accessible button is connected to **GPIO 24** and is checked during system boot.

- **Press and release**: Dumps log content to Serial @ **115200 baud**.
- **Press and hold for 5 seconds**: Erases the entire log file from the onboard flash memory.

This provides a fast and safe way to extract and reset logs without reflashing the system.

---