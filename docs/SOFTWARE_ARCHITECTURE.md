# Software Architecture

The SCARA software evolved from basic direct joint control into a modular embedded robot-control system.

## Core functions

### Motion control

- Step/direction control for DRV8825 drivers
- Speed and acceleration limits
- Joint-position tracking
- Coordinated A1/A2/Z motion
- Motor enable/disable handling

### Homing and calibration

- Limit-switch reference acquisition
- Joint-zero calibration
- Safe motion limits
- Calibration persistence
- Collision-aware gripper-clearance concepts in later revisions

### Kinematics

**Forward kinematics** converts A1/A2 joint angles into Cartesian X/Y position.

**Inverse kinematics** converts a requested X/Y target into valid A1/A2 joint solutions while applying configured joint limits.

### Web interface

The ESP32 hosts a local web interface used for:

- Manual joint commands
- Cartesian target commands
- FK/IK visualization
- Homing
- Calibration
- Status monitoring
- STOP / motor-off control

### Automation and integration

Later project revisions expanded the architecture toward:

- Stored poses and recipe-style motion sequences
- Persistent robot configuration
- HTTP API control
- Robot-cell integration
- MQTT-ready communication concepts

## Included source snapshot

`firmware/legacy-v10/SCARA_Robot_v10/` is intentionally marked **legacy**. It is useful for inspecting the actual embedded implementation of AccelStepper-based motion, ESP32 WebServer endpoints, Preferences-based calibration storage, homing, FK/IK, and browser control.

Its geometry values belong to an earlier calibration state and must not be confused with the final mechanical dimensions in the technical report.
