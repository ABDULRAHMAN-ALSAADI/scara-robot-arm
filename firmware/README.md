# Firmware

## Included snapshot

`legacy-v10/SCARA_Robot_v10/` contains a preserved ESP32 development snapshot with:

- NEMA 17 / DRV8825 step-direction control
- Limit-switch homing
- Forward kinematics
- Inverse kinematics
- Calibration storage using ESP32 Preferences
- Local Wi-Fi access point
- Embedded browser interface
- Manual movement, STOP, OFF, and calibration endpoints

The Wi-Fi password in the public copy is intentionally replaced with `CHANGE_ME`.

## Important

This snapshot is **not the final mechanical configuration**. Its internal link lengths and calibration values differ from the final documented robot geometry.

Use it as an engineering/code reference. Before running any version on physical hardware, verify pin mapping, driver current, step direction, homing direction, mechanical joint limits, TCP definition, and safe collision boundaries.
