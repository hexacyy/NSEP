# Mecanum Robot

An Arduino-based 4WD mecanum wheel robot with autonomous obstacle avoidance, IR remote control, and servo gripper support. Built on the Keyestudio KS0560 platform using PlatformIO.

## Hardware

- **Board:** Arduino Uno
- **Chassis:** Keyestudio KS0560 4WD Mecanum Robot
- **Motors:** 4x DC motors controlled via I2C motor driver (SDA=D3, SCL=D2)
- **Ultrasonic Sensor:** HC-SR04 (TRIG=D12, ECHO=D13) for obstacle detection
- **IR Receiver:** Connected to A3
- **Servo:** Connected to D9 (used as a gripper/claw)
- **RGB LEDs:** 4x WS2812B NeoPixels on D10
- **Line Tracking Sensors:** Left=A0, Middle=A1, Right=A2

## Features

### Main Firmware (`src/main.cpp`)
- **Autonomous obstacle avoidance** — moves forward, detects obstacles within 40 cm, turns right 90°, then resumes
- **IR remote control** — start/stop autonomous mode and toggle servo gripper via IR remote
- **Non-blocking state machine** — uses timed states (`STOPPED`, `MOVING_FORWARD`, `TURNING_RIGHT`) with no blocking `delay()` in the main loop
- **Serial logging** — distance and motion state output at 9600 baud

### IR Remote Key Bindings

| Key Code | Action |
|----------|--------|
| `64` | Toggle autonomous mode (start/stop) |
| `22` | Toggle servo gripper (open/close) |

### Tunable Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `DEFAULT_SPEED` | 50 | Motor speed (0–255) |
| `OBSTACLE_DISTANCE_CM` | 40 | Distance threshold to trigger a turn |
| `TURN_90_MS` | 350 ms | Duration of a right turn (tune for exact 90°) |
| `POST_TURN_SETTLE_MS` | 300 ms | Settle time after a turn before moving |

## Example Sketches (`examples/KS0560/`)

| Lesson | Description |
|--------|-------------|
| `lesson_0` | Servo angle initialization |
| `lesson_1` | RGB color LED |
| `lesson_2.1` / `2.2` | WS2812B NeoPixel patterns |
| `lesson_3.1` / `3.2` | Servo control |
| `lesson_4` | Basic motor control |
| `lesson_5` | Line tracking sensor test |
| `lesson_6` | Line tracking robot |
| `lesson_7` | Ultrasonic distance sensor |
| `lesson_8` | Ultrasonic follow robot |
| `lesson_9` | Ultrasonic obstacle avoidance robot |
| `lesson_10.1` / `10.2` | IR remote receiver and LED control |
| `lesson_11` | IR remote controlled robot |
| `lesson_12.1` / `12.2` | Bluetooth app test and full app control |

## Libraries

| Library | Purpose |
|---------|---------|
| `MecanumCar_v2` | I2C motor driver abstraction for the 4WD mecanum chassis |
| `Adafruit_NeoPixel` | WS2812B RGB LED control |
| `Servo` | Servo motor control |
| `ir` | IR receiver decoding |

## Build & Upload

This project uses [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload to Arduino Uno
pio run --target upload

# Open serial monitor (9600 baud)
pio device monitor --baud 9600
```

Or use the PlatformIO IDE extension for VS Code.

## Mecanum Wheel Movement

Mecanum wheels allow omnidirectional movement. The `MecanumCar_v2` library exposes:

| Method | Movement |
|--------|----------|
| `Advance()` | Forward |
| `Back()` | Backward |
| `Turn_Left()` | Rotate left |
| `Turn_Right()` | Rotate right |
| `L_Move()` | Strafe left |
| `R_Move()` | Strafe right |
| `LU_Move()` / `RU_Move()` | Diagonal forward-left / forward-right |
| `LD_Move()` / `RD_Move()` | Diagonal back-left / back-right |
| `drift_left()` / `drift_right()` | Drift |
| `Stop()` | Stop all motors |

Per-wheel speed is set via the global variables `speed_Upper_L`, `speed_Lower_L`, `speed_Upper_R`, `speed_Lower_R` (0–255).
