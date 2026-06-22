# NSEP — Robotics Projects

This repository contains firmware and example sketches for two embedded robotics projects built during the NSEP programme.

---

## Projects

### 1. RoboArm — 6-DOF Desktop Robot Arm

**Folder:** `RoboArm/`

PlatformIO firmware for a 6-DOF desktop robot arm driven by an **ESP32**. The arm is controlled via a physical analog joystick, hardware buttons, and a browser-based game-controller UI served directly from the ESP32 over WiFi.

#### Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32 (esp32dev) |
| PWM driver | Adafruit PCA9685, I2C addr `0x40`, SDA=GPIO21, SCL=GPIO22 |
| Servos | 6× hobby servo on PCA9685 channels 0–5 |
| Joystick | Analog VRX=GPIO34, VRY=GPIO35, SW=GPIO32 |
| Buttons | REC=GPIO33, PLAY=GPIO25, CLR=GPIO26, CYCLE=GPIO27 |

#### Joint Layout

| # | Joint | Range | Home |
|---|-------|-------|------|
| 0 | Base | 0–270° | 135° |
| 1 | Shoulder | 30–150° | 90° |
| 2 | Elbow | 0–180° | 90° |
| 3 | Wrist Pitch | 0–180° | 90° |
| 4 | Wrist Roll | 0–180° | 90° |
| 5 | Gripper | 0–90° | 45° |

#### Key Features

- **Web UI** — single-page app served from the ESP32 with dual XY thumbsticks, vertical triggers, preset buttons, and a pose recorder; supports keyboard and browser Gamepad API (Xbox/PS controllers)
- **Physical controls** — analog joystick jogs one joint pair at a time; push-button to cycle pairs; dedicated REC / PLAY / CLR / CYCLE buttons
- **Pose recording** — up to 50 poses with labels, one-shot and looping playback at 1200 ms/step, persisted to ESP32 flash via `Preferences`
- **WebSocket protocol** — compact 2-letter `TAG:arg` text commands (no JSON parsing on the hot path), zero-heap JSON responses via `snprintf` into static buffers
- **Serial debug interface** — `TEST`, `STATUS`, `S <joint> <angle>`, `INVERT`, `SWEEP`, `CALSHOW`, `HOME/READY/PICK/PLACE` (115200 baud)

#### Build & Flash

```powershell
pio run                  # compile
pio run -t upload        # flash over USB
pio run -t monitor       # open serial monitor (115200 baud)
```

Active source file: `src/main_new.cpp` (set via `build_src_filter` in `platformio.ini`).

#### Dependencies

- `adafruit/Adafruit PWM Servo Driver Library`
- `ESP32Async/ESPAsyncWebServer`
- `ESP32Async/AsyncTCP`

---

### 2. Mecanum Robot — 4WD Omnidirectional Robot

**Folder:** `Mecanum Robot/`

Arduino firmware for a 4-wheel mecanum robot built on the **Keyestudio KS0560** platform. Supports autonomous obstacle avoidance, IR remote control, line tracking, and a servo gripper, with 13 progressive lesson sketches covering every sensor and actuator on the platform.

#### Hardware

| Component | Detail |
|-----------|--------|
| MCU | Arduino Uno |
| Chassis | Keyestudio KS0560 4WD Mecanum Robot |
| Motors | 4× DC motor via I2C motor driver (SDA=D3, SCL=D2) |
| Ultrasonic | HC-SR04, TRIG=D12, ECHO=D13 |
| IR Receiver | A3 |
| Servo / Gripper | D9 |
| RGB LEDs | 4× WS2812B NeoPixel on D10 |
| Line Sensors | Left=A0, Middle=A1, Right=A2 |

#### Main Firmware Features (`src/main.cpp`)

- **Autonomous obstacle avoidance** — advances until an obstacle is detected within 40 cm, turns right 90°, then resumes
- **IR remote control** — key `64` toggles autonomous mode; key `22` opens/closes the servo gripper
- **Non-blocking state machine** — `STOPPED / MOVING_FORWARD / TURNING_RIGHT` states with no `delay()` in the main loop
- **Serial logging** — distance and motion state output at 9600 baud

#### Mecanum Movement API (`MecanumCar_v2`)

| Method | Movement |
|--------|----------|
| `Advance()` | Forward |
| `Back()` | Backward |
| `Turn_Left()` / `Turn_Right()` | Rotate in place |
| `L_Move()` / `R_Move()` | Strafe left / right |
| `LU_Move()` / `RU_Move()` | Diagonal forward |
| `LD_Move()` / `RD_Move()` | Diagonal backward |
| `drift_left()` / `drift_right()` | Drift |
| `Stop()` | Stop all motors |

#### Example Lessons (`examples/KS0560/`)

| Lesson | Topic |
|--------|-------|
| `lesson_0` | Servo angle initialisation |
| `lesson_1` | RGB color LED |
| `lesson_2.1 / 2.2` | WS2812B NeoPixel patterns |
| `lesson_3.1 / 3.2` | Servo control |
| `lesson_4` | Basic motor control |
| `lesson_5` | Line tracking sensor test |
| `lesson_6` | Line tracking robot |
| `lesson_7` | Ultrasonic distance sensor |
| `lesson_8` | Ultrasonic follow robot |
| `lesson_9` | Ultrasonic obstacle avoidance |
| `lesson_10.1 / 10.2` | IR remote receiver and LED control |
| `lesson_11` | IR remote controlled robot |
| `lesson_12.1 / 12.2` | Bluetooth app test and full app control |

#### Build & Upload

```bash
pio run                              # compile
pio run --target upload              # flash to Arduino Uno
pio device monitor --baud 9600       # serial monitor
```

#### Libraries

| Library | Purpose |
|---------|---------|
| `MecanumCar_v2` | I2C motor driver for the 4WD mecanum chassis |
| `Adafruit_NeoPixel` | WS2812B RGB LED control |
| `Servo` | Servo motor control |
| `ir` | IR receiver decoding |

---

## Tools

Both projects use [PlatformIO](https://platformio.org/). Install the PlatformIO IDE extension for VS Code or use the CLI (`pio`).
