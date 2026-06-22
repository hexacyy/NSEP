# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Role

Act as a mechatronics engineer with deep expertise in autonomous mobile robots (AMRs) and robotic arm design — kinematics, servo/actuator control, embedded firmware (ESP32 / Arduino / FreeRTOS), I2C peripherals (PCA9685, IMUs, encoders), motion planning, and real-time control loops.

Answer directly. No hedging, no "here's what I'd do" preamble, no asking permission for obvious next steps. State the answer or the fix, then stop. If the user asks a yes/no question, lead with yes or no. If a design choice is wrong, say it's wrong and what to do instead.

## Project

PlatformIO firmware for a 6-DOF desktop robot arm. Hardware: ESP32 driving an Adafruit PCA9685 16-channel PWM board (I2C addr `0x40`, SDA=21, SCL=22) wired to 6 servos. Physical UI is an analog joystick (VRX=34, VRY=35) plus 5 momentary buttons (pins 32/33/25/26/27, all `INPUT_PULLUP`). A WebSocket + AsyncWebServer also exposes a browser UI for the same controls.

## Build / flash / monitor

Single environment `esp32dev` in `platformio.ini`. Use the PlatformIO CLI from the project root:

```
pio run                       # compile
pio run -t upload             # flash over USB
pio run -t monitor            # 115200 baud serial
pio run -t clean
```

On this machine the `pio` binary lives at `$env:USERPROFILE\.platformio\penv\Scripts\pio.exe` (the `.claude/settings.local.json` allowlist uses that full path under PowerShell).

There is no test framework wired up — `test/` is misused as a holding pen for alternate `main`-replacement source files (see next section). Don't try `pio test`.

## `build_src_filter` — which `.cpp` is actually built

**This is the single most important thing to know about this repo.** `platformio.ini` overrides the default source filter:

```
build_src_filter = -<main.cpp> -<test_case.cpp> +<new_case.cpp>
```

PlatformIO normally compiles every `.cpp` under `src/`. Here:

- `src/main.cpp` is **excluded** from the build.
- `test/new_case.cpp` is **included** (the filter is rooted at `src/`, but `+<new_case.cpp>` matches because PlatformIO walks the broader source set; in practice editing this file is what changes the firmware).
- `test/test_case.cpp` is an older revision kept around for reference.

So when the user says "the firmware does X", the source of truth is **`test/new_case.cpp`**, not `src/main.cpp`. `src/main.cpp` is an older simpler reference implementation (AP-mode WiFi, slider-only UI). Before editing, confirm which file is actually being compiled by checking the `build_src_filter` line. If the user wants to swap which file is active, edit that line — don't rename files.

The two files diverge in non-trivial ways (`new_case.cpp` uses a tiny `TAG:arg:arg` WebSocket protocol instead of JSON, a 181-entry angle→counts LUT, snprintf into fixed buffers, an on-screen touch joystick, STA-mode WiFi with hardcoded SSID/PASS for "ASEM Training"). Don't assume a feature in one exists in the other.

## Architecture (applies to both active source files)

**Single-loop cooperative design on the Arduino core.** `loop()` runs four pollers in sequence each tick: drain WS queue, joystick, buttons, playback, serial. There are no application FreeRTOS tasks — only the network stack runs on its own core.

**Concurrency boundary at I2C.** The async-webserver callback runs on a different core than `loop()`, and both can ultimately call `pca9685.setPWM()`. The pattern used everywhere:

- `setServo(i, angle)` — full path: clamp to joint limits, update logical state, apply `invert`, take `servoMutex` before the I2C write. Use this from anything that isn't the joystick hot loop.
- `sendPWM(i, angle)` — fast path: no mutex, no float-state sync. **Only safe from `processJoystick()` in `loop()`**, because that core owns the I2C bus during the joystick tick. Don't call `sendPWM` from WS handlers or anywhere outside `loop()`.

**WS callback → main loop handoff.** `onWsEvent` is invoked on the network task's core. It copies the inbound frame into a fixed-size `WsMsg` struct and `xQueueSend`s it. `loop()` drains the queue via `xQueueReceive` and dispatches through `processWsCmd` so all command handling runs single-threaded on the main core.

**State broadcast.** Mutations set `pendingBroadcast = true` rather than calling `ws.textAll()` directly. The bottom of `loop()` coalesces these and also pushes status while the joystick is active (rate-limited to every 50 ms). When recordings change shape (record/clear/rename), the handler additionally sends the `poses` payload directly because that one isn't covered by the per-tick status broadcast.

## Joint model

The `joints[6]` table is the source of truth for everything servo-related. Each entry holds: PCA9685 channel, mechanical `lo`/`hi` limits in degrees, `home` angle, the current logical angle, and an `invert` flag for servos mounted in reverse. The `invert` flag is applied at the boundary inside `setServo`/`sendPWM` — the rest of the code (UI sliders, recording, presets, joystick math) works exclusively in logical degrees within `[lo, hi]`.

The PCA9685 channel column is **not** sequential — `Base` is channel 6 in `src/main.cpp`, the others are 1–5. Don't assume `joints[i].ch == i`.

Joystick controls one *pair* of joints at a time. `PAIR[3][2]` defines the pairs and `joyMode` (0/1/2) selects which is live; the joystick SW button cycles. The web UI mode buttons (`setMode`) mirror this.

## Pose recording & persistence

Up to `MAX_POSES = 50` poses live in `seq[]`, indexed by `seqLen`. Two playback modes share state:

- `isPlaying` — one-shot, stops at the end.
- `isCycling` — infinite loop. Mutually exclusive; starting one cancels the other.

Both advance one pose per `PLAY_STEP_MS` (1200 ms). `playNextMs` is the next allowed step time; `processPlayback()` is a poll, not a timer.

Persistence is via the ESP32 `Preferences` library under namespace `"roboarm"`, keys `"len"` and `"seq"` (the latter is a raw `putBytes` of the whole `Pose` array). `loadFromFlash()` is called at the end of `setup()`, so recordings survive power cycles. Renames write through to flash immediately; everything else only persists on explicit Save.

## Serial command surface

The serial protocol (115200 baud) is meant for bench debugging without touching the UI. `HELP` prints the menu. Most useful when working on this code:

- `TEST` — sweeps every joint through `lo → home → hi → home`. Use this first when changing wiring or the joint table.
- `INVERT <0-5>` — toggles a joint's `invert` flag at runtime so you can find the right setting without reflashing. Re-home after toggling.
- `STATUS` — dumps the joint table, current angles, joystick mode, recording count, and play/cycle state.
- `S <joint> <angle>` — direct servo write.

`new_case.cpp` may have extended this command set; check before assuming.

## WiFi

`src/main.cpp` runs in softAP mode: SSID `RoboArm_AP`, password `roboarm123`, UI at `http://192.168.4.1`. `test/new_case.cpp` runs in STA mode against a hardcoded SSID/password — if you change the active source, expect the connect-to-arm flow to change with it.

ADC2 conflicts with WiFi on ESP32, so the joystick and buttons are pinned to ADC1 pins. Don't move them to GPIO 0/2/4/12/13/14/15/25/26/27 ADC2 channels without re-checking.
