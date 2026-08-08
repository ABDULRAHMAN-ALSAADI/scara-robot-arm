# SCARA Robot Arm — Hardware Configuration Guide

**Firmware Version:** v10.22 — LittleFS Recipe Store  
**Target Microcontroller:** ESP32 (DevKit or equivalent)  
**Library:** AccelStepper + ESP32 Arduino Core

---

## Table of Contents

1. [Overview](#overview)
2. [Required Hardware](#required-hardware)
3. [Motor Driver Wiring (DRV8825)](#motor-driver-wiring-drv8825)
4. [Limit Switch Wiring](#limit-switch-wiring)
5. [Servo Gripper Wiring](#servo-gripper-wiring)
6. [Mechanical Constants](#mechanical-constants)
7. [Motor Speed & Acceleration Settings](#motor-speed--acceleration-settings)
8. [WiFi Configuration](#wifi-configuration)
9. [Arduino IDE Setup & Libraries](#arduino-ide-setup--libraries)
10. [Flash & Upload Instructions](#flash--upload-instructions)
11. [First Boot Checklist](#first-boot-checklist)
12. [NVS Storage Namespaces](#nvs-storage-namespaces)
13. [Serial Debug Interface](#serial-debug-interface)
14. [Web Interface Access](#web-interface-access)

---

## Overview

This firmware controls a 3-axis SCARA robot arm (Shoulder / Elbow / Z-axis) with a servo-driven gripper, hosted on an ESP32. It provides:

- Full Forward/Inverse Kinematics over a built-in web interface
- Homing sequences with limit switches
- Z-layered collision envelope safety
- Package recipe automation
- Dual WiFi mode (AP + STA)
- Non-volatile calibration stored in ESP32 Preferences (NVS) and LittleFS

---

## Required Hardware

| Component | Quantity | Notes |
|---|---|---|
| ESP32 DevKit (30-pin or 38-pin) | 1 | Any ESP32 with enough GPIO |
| DRV8825 Stepper Driver | 3 | One per axis (A1, A2, Z) |
| NEMA 17 Stepper Motor | 3 | One per axis |
| Micro Servo (e.g. SG90 / MG90S) | 1 | Gripper servo |
| Mechanical Endstop / Limit Switch | 3 | One per axis homing |
| 5 V regulated power supply | 1 | For ESP32 and servo |
| 12–24 V stepper power supply | 1 | For DRV8825 VM rail |
| 100 µF capacitor (per DRV8825) | 3 | Placed across VM/GND |
| Common ground wire | — | Link ESP32 GND with stepper PSU GND |

---

## Motor Driver Wiring (DRV8825)

Each stepper axis uses a DRV8825 driver in **STEP/DIR** mode. The ESP32 drives three signals per axis: **STEP**, **DIR**, and **EN** (active-LOW enable).

> **DRV8825 ENABLE is active LOW** — the firmware drives EN HIGH to disable (safe/idle) and LOW to enable (powered).

### GPIO Pin Assignment

| Signal | Arm 1 (Shoulder / A1) | Arm 2 (Elbow / A2) | Z Axis |
|---|---|---|---|
| STEP | GPIO **17** | GPIO **19** | GPIO **2** |
| DIR | GPIO **16** | GPIO **18** | GPIO **15** |
| EN | GPIO **5** | GPIO **21** | GPIO **4** |

### DRV8825 Wiring per Axis

```
ESP32 GPIO (STEP) ──────────── DRV8825 STEP
ESP32 GPIO (DIR)  ──────────── DRV8825 DIR
ESP32 GPIO (EN)   ──────────── DRV8825 EN   (HIGH = disabled, LOW = enabled)
ESP32 3.3V        ──────────── DRV8825 VDD  (logic supply)
ESP32 GND         ──────────── DRV8825 GND  (logic ground)
12–24V PSU (+)    ──────────── DRV8825 VMOT (motor supply, add 100µF cap across VMOT/GND)
12–24V PSU (−)    ──────────── DRV8825 GND
DRV8825 1A/1B     ──────────── Stepper coil A
DRV8825 2A/2B     ──────────── Stepper coil B
```

### Motor Direction Inversion

The firmware applies the following direction inversions in software (no rewiring needed):

| Axis | Direction Inverted |
|---|---|
| Arm 1 (A1) | **Yes** — `arm1.setPinsInverted(true, false, false)` |
| Arm 2 (A2) | No |
| Z Axis | No |

If your arm moves in the wrong direction after homing, swap the `setPinsInverted` first argument for that axis before rewiring.

### STEP Pulse Width

The DRV8825 requires a minimum STEP high/low pulse duration of **1.9 µs**. The firmware uses **3 µs** for reliable ESP32 output:

```cpp
arm1.setMinPulseWidth(3);  // DRV8825_MIN_STEP_PULSE_US
arm2.setMinPulseWidth(3);
zAxis.setMinPulseWidth(3);
```

> Do **not** use A4988 drivers without adjusting this value. A4988 does not require this margin.

---

## Limit Switch Wiring

Each axis has one **normally-open mechanical endstop** used as the HOME reference switch. All switches use the ESP32's internal pull-up resistors (`INPUT_PULLUP`). The switch triggers when the GPIO reads `LOW`.

| Axis | GPIO Pin | Switch Activation Direction |
|---|---|---|
| Arm 1 (A1 Shoulder) | GPIO **14** | Negative raw direction (toward home) |
| Arm 2 (A2 Elbow) | GPIO **27** | Positive raw direction (toward home) |
| Z Axis | GPIO **26** | Upward (negative raw direction) — top of travel |

### Wiring per Switch

```
Endstop COM  ──────── ESP32 GPIO (e.g. GPIO 14)
Endstop NO   ──────── ESP32 GND
(No external pull-up needed — INPUT_PULLUP is used in firmware)
```

> A2's homing direction is **reversed** relative to A1. A2 seeks its switch by moving in the **positive** raw step direction. See `homeA2()` in `07_HomingSequence.ino`.

### Home Release Distances

After a switch is triggered, each axis backs off by a fixed raw-step clearance before calibration coordinates are applied:

| Axis | Release Steps | Approx. Physical Distance |
|---|---|---|
| A1 | 1500 steps | ~10.5° raw clearance |
| A2 | 400 steps | ~10° raw clearance |
| Z | 1000 steps | ~2.5 mm below top switch |

---

## Servo Gripper Wiring

The gripper is driven by a standard hobby servo using ESP32 hardware PWM (LEDC).

| Signal | Connection |
|---|---|
| Servo SIGNAL (orange/yellow) | GPIO **13** |
| Servo V+ (red) | **External 5 V regulated supply** (do NOT use ESP32 3.3V or onboard 5V) |
| Servo GND (brown/black) | External supply GND, tied to ESP32 common GND |

> **Important:** The servo must be powered from an external 5 V supply. Drawing servo current through the ESP32's onboard regulator can cause brownouts and resets.

### PWM Parameters

| Parameter | Value |
|---|---|
| PWM Frequency | 50 Hz |
| PWM Resolution | 16-bit |
| Min Pulse Width | 500 µs |
| Max Pulse Width | 2500 µs |
| Hard Maximum Angle | 120° (180° is explicitly rejected) |

### Default Gripper Positions

| Position | Default Angle |
|---|---|
| OPEN | 0° |
| HOLD | 70° |
| MAX CLOSE | 90° |

These values are saved to NVS and can be recalibrated from the web interface without reflashing.

---

## Mechanical Constants

These values are defined in `SCARA_ROBOT_ARM_CODE.ino` and must match your physical build:

| Constant | Value | Description |
|---|---|---|
| `L1` | 136.5 mm | Length of Arm 1 (shoulder to elbow pivot) |
| `toolTcpLengthMm` | 120.0 mm | Tool Center Point: elbow pivot to grasp center (editable via web) |
| `A1_STEPS_PER_DEG` | 142.22 steps/° | A1 shoulder resolution |
| `A2_STEPS_PER_DEG` | 40.0 steps/° | A2 elbow resolution |
| `Z_STEPS_PER_MM` | 400.0 steps/mm | Z-axis linear resolution |

> If you change your drive ratio, belt pulley, or leadscrew, update these constants and reflash before running any homing or calibration.

---

## Motor Speed & Acceleration Settings

Speed ceilings are defined in `SCARA_ROBOT_ARM_CODE.ino`. Motion profiles scale these as a percentage for different operation modes.

### Hardware Speed Ceilings

| Axis | Max Speed | Acceleration |
|---|---|---|
| A1 (Shoulder) | 9500 steps/s (~66.8°/s) | 12000 steps/s² |
| A2 (Elbow) | 7500 steps/s (~187.5°/s) | 10000 steps/s² |
| Z Axis | 10000 steps/s | 5000 steps/s² |

### Homing Speeds (fixed, independent of user profiles)

| Axis | Homing Speed |
|---|---|
| A1 | 2600 steps/s |
| A2 | 2200 steps/s |
| Z | 5000 steps/s |

### Default Motion Profile Percentages

These scale the hardware ceilings above:

| Profile | Default % | When Used |
|---|---|---|
| Manual / FK | 60% | Point-to-point FK moves |
| Real-Time | 35% | Live slider dragging |
| IK | 60% | Inverse kinematics moves |
| Setup Jog | 20% | Calibration jogging (fixed) |

Profiles are adjustable from the web interface and saved to NVS.

### Motor Idle Policy

After each movement completes, all three DRV8825 drivers are **disabled after 1200 ms of inactivity** to allow cooling. If the arm is moved by hand while motors are off, you must press HOME again before using FK/IK.

---

## WiFi Configuration

The firmware runs in **AP+STA** (dual) WiFi mode. Update the credentials in `SCARA_ROBOT_ARM_CODE.ino` before flashing:

```cpp
// Backup access point (always available)
const char* AP_SSID = "SCARA-Robot";
const char* AP_PASS = "robot1234";

// Main robot-cell network (join this after boot)
const char* STA_SSID = "ALSAADI";
const char* STA_PASS = "Alsaadi123";
```

| Network | SSID | Default IP | Use |
|---|---|---|---|
| Backup AP | `SCARA-Robot` | `192.168.4.1` | Always available for setup/recovery |
| Station (STA) | `ALSAADI` | Assigned by router DHCP | Main operating network |

- STA connection timeout: **15 seconds**. If it fails, the backup AP remains active.
- The STA IP is printed to the Serial Monitor at boot — use this IP in any MQTT coordinator GUI.
- AP is on **channel 6**, visible SSID, max 4 connected stations.

> Change `AP_SSID`, `AP_PASS`, `STA_SSID`, and `STA_PASS` to match your own network before flashing.

---

## Arduino IDE Setup & Libraries

### Board Settings

| Setting | Value |
|---|---|
| Board | **ESP32 Dev Module** (or your specific ESP32 variant) |
| Upload Speed | 921600 |
| CPU Frequency | 240 MHz |
| Flash Frequency | 80 MHz |
| Flash Mode | DIO |
| Partition Scheme | **Default 4MB with spiffs** (or `Huge APP` if code is too large) |
| PSRAM | Disabled |

### Required Libraries

Install all of the following via Arduino Library Manager (`Sketch → Include Library → Manage Libraries`):

| Library | Purpose |
|---|---|
| **AccelStepper** by Mike McCauley | Stepper motor acceleration control |
| **ESP32 Arduino Core** (Espressif) | WiFi, WebServer, Preferences, LittleFS, LEDC PWM |

> The ESP32 Arduino Core (v2.x or v3.x) provides `WiFi.h`, `WebServer.h`, `Preferences.h`, `FS.h`, `LittleFS.h`, and `ledcAttach()` / `ledcWrite()`. Make sure it is installed via the Boards Manager (`Tools → Board → Boards Manager → search "esp32"`).

> **Note on LEDC API:** This firmware uses the newer `ledcAttach(pin, freq, resolution)` API available in ESP32 Arduino Core v3.x. If you are using Core v2.x, you may need to update to v3.x or adapt the gripper PWM code in `10_GripperControl.ino`.

---

## Flash & Upload Instructions

### File Structure

All `.ino` tabs must be in the **same folder** with the same base name as the main sketch:

```
SCARA_ROBOT_ARM_CODE/
├── SCARA_ROBOT_ARM_CODE.ino    ← Main tab (open this in Arduino IDE)
├── 01_DriverControl.ino
├── 02_CalibrationAndStorage.ino
├── 03_JointCoordinates.ino
├── 04_ForwardKinematics.ino
├── 05_InverseKinematics.ino
├── 06_MotionAndSafety.ino
├── 07_HomingSequence.ino
├── 08_WebApi.ino
├── 09_SerialDebug.ino
├── 10_GripperControl.ino
├── 11_CollisionSafety.ino
├── 12_PackageRecipes.ino
├── 13_ArmJobController.ino
└── webpage.h
```

Open `SCARA_ROBOT_ARM_CODE.ino` in the Arduino IDE. All other tabs will load automatically.

### Upload Steps

1. Connect the ESP32 via USB.
2. Select the correct COM port under `Tools → Port`.
3. Select `Tools → Board → ESP32 Dev Module`.
4. Set the Partition Scheme to **Default 4MB with spiffs** (required for LittleFS).
5. **For first flash:** Set `Tools → Erase All Flash Before Sketch Upload → Enabled`.  
   **For updates (keeping calibration):** Set this to **Disabled** so NVS calibration data is preserved.
6. Click **Upload**.

> After a successful flash, open the Serial Monitor at **115200 baud** to confirm boot output.

---

## First Boot Checklist

Work through these steps in order after flashing for the first time:

**1. Serial Monitor Confirmation**
- Open Serial Monitor at 115200 baud.
- Confirm you see: `SCARA v10.21 ready: compact persistent recipe store.`
- Note the Backup AP IP (`http://192.168.4.1`) and STA IP (if connected to your network).

**2. Connect to the Web Interface**
- Connect your phone or PC to the `SCARA-Robot` WiFi network (password: `robot1234`).
- Open a browser and navigate to `http://192.168.4.1`.

**3. Physically Position the Arm**
- Before pressing HOME, manually move the arm to the **Safe Startup Pose** (a known safe posture away from all hard limits and the robot body).
- Confirm this pose on the web interface using the **Startup Confirm** button.

**4. Run HOME**
- Press the HOME button on the web interface.
- The homing sequence runs in this order: **Z → A2 → A1**.
- Watch the Serial Monitor for progress messages.
- After homing, the arm moves to the ready pose: A1 = +130°, A2 = −136°.

**5. Run Calibration**
- Use the Setup/Calibration section of the web interface to jog each axis and save the geometric zero and physical endpoint limits.
- Calibration is stored in NVS and survives power cycles.

**6. Calibrate the Gripper**
- Move the gripper to OPEN, HOLD, and MAX CLOSE positions using the servo slider.
- Save the calibrated angles from the Gripper Calibration section.

---

## NVS Storage Namespaces

All calibration, settings, and recipes are stored in ESP32 Non-Volatile Storage (NVS) and LittleFS. These survive power cycles and reboots.

| Namespace | Content |
|---|---|
| `scara_raw_v2` | Raw joint calibration: geometric zero and physical endpoint raw steps for A1, A2, Z |
| `scara_motion` | Motion profile speed percentages (Manual FK, Real-Time, IK) |
| `scara_tool` | Tool Center Point length (L2 TCP) |
| `scara_grip` | Servo gripper OPEN / HOLD / MAX CLOSE angles |
| `scara_env3` | Z-layered collision envelopes and Safe Startup Park pose |
| LittleFS `/scara_recipes.dat` | Package recipes (taught box-type motion sequences) |

> **Important:** Uploading with "Erase All Flash Before Sketch Upload" **enabled** will wipe all of the above. Use this only on a fresh ESP32 or when intentionally resetting everything.

---

## Serial Debug Interface

Connect at **115200 baud**. The firmware prints status at boot and after every HOME/motion event. You can also send text commands via Serial for manual control and diagnostics. See `09_SerialDebug.ino` for the full command list.

---

## Web Interface Access

After boot, the web interface is available at:

| Network | URL |
|---|---|
| Backup AP | `http://192.168.4.1` |
| Shared WiFi (STA) | `http://<STA_IP>` (printed at boot in Serial Monitor) |

The interface provides: FK / IK jogging, real-time slider control, homing, calibration, gripper control, collision envelope editing, package recipe management, and the Arm Job Controller.

---

## Quick Reference: GPIO Summary

| GPIO | Function | Notes |
|---|---|---|
| 2 | Z STEP | |
| 4 | Z EN | Active LOW |
| 5 | A1 EN | Active LOW |
| 13 | Servo SIGNAL | Gripper PWM, 50 Hz, 16-bit |
| 14 | A1 LIMIT switch | INPUT_PULLUP, LOW = pressed |
| 15 | Z DIR | |
| 16 | A1 DIR | |
| 17 | A1 STEP | |
| 18 | A2 DIR | |
| 19 | A2 STEP | |
| 21 | A2 EN | Active LOW |
| 26 | Z LIMIT switch | INPUT_PULLUP, LOW = pressed |
| 27 | A2 LIMIT switch | INPUT_PULLUP, LOW = pressed |

---

*Generated from firmware source: SCARA v10.22*
