# LD1115H Presence & Motion Detection with ESP32

## Overview
This project integrates the **LD1115H Mini 24G Human Presence Sensor** with an **ESP32**, using UART communication to read motion and presence detection data. The code filters noisy data, improves real-time detection, and introduces configurable thresholds for movement and presence sensitivity.

## Features
- **Detects motion (`mov`) and presence (`occ`)** using the LD1115H radar sensor.
- **Filters out noise** and incorrect sensor data.
- **Adjustable thresholds** for fine-tuning detection.
- **Built-in debounce logic** to reduce false triggers.
- **Dynamic mode interpretation** to classify motion/presence more accurately.

## Hardware Requirements
- ESP32 Development Board
- LD1115H Mini 24G Human Presence Sensor
- Dupont wires for UART connection

## Wiring (ESP32 to LD1115H Sensor)
| LD1115H Pin | ESP32 Pin |
|-----------|----------|
| VCC (3.6-5V) | 3.3V or 5V |
| GND | GND |
| URX | GPIO5 |
| UTX | GPIO4 |

## Code Breakdown
### 1. **Threshold Variables**
```cpp
int TH1 = 1200;  // Motion sensitivity (higher = less sensitive)
int TH2 = 1800;  // Presence sensitivity (higher = less sensitive)
unsigned long clear_time = 750;  // Delay before resetting movement detection (ms)
```
- `TH1` adjusts motion sensitivity (higher = detects less movement).
- `TH2` adjusts presence sensitivity (higher = needs stronger reflection to detect presence).
- `clear_time` defines how long the system waits before clearing movement status.

### 2. **Mode Interpretation** (Guessing)
The LD1115H sensor provides a "mode" value which we interpret as:
| Mode | Meaning |
|------|---------|
| 1 | Very light movement |
| 3 | Small movement |
| 5 | Stronger movement |
| 6 | Calm presence |
| 7 | Presence remains stable |
| 8 | Clear presence |
| 9 | Very strong presence |

### 3. **Filtering False Positives**
- Unknown modes are ignored.
- Presence (`occ`) is only detected **after a delay** to prevent false readings immediately following movement.
- A **debounce time of 500ms** prevents unwanted status changes.

## Output Example
```
🔵 Movement detected! (Mode: Small movement | Signal: 13887)
🔵 Movement detected! (Mode: Stronger movement | Signal: 16500)
🔻 No movement detected.
🟢 Presence detected! (Mode: Clear presence | Signal: 6466)
🟢 Presence detected! (Mode: Very strong presence | Signal: 6412)
```

## Open Questions / To-Do
### 🟡 **Distance Calculation**
The sensor provides a signal strength, but there's no official formula to convert this into a **distance measurement**.
- We need a calibration test with known distances to create an estimation formula.
- If you have **measured signal strengths at known distances**, please share!

### 🔍 **Datasheet / Specifications**
- There is **no official spec sheet** for the **LD1115H Mini 24G Human Presence Sensor**.
- If you find **detailed technical documentation** (e.g., from **AliExpress or other suppliers**), please contribute.

## Contributions
- If you improve the detection algorithm or find **better hardware specs**, feel free to open a **pull request** or share your findings!

## License
This project is open-source under the **MIT License**.

