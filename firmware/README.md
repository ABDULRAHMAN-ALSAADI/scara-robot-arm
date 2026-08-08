# Firmware

The current robot firmware is **v10.22** and is located in [`v10.22/`](v10.22/).

The sketch is intentionally split into subsystem tabs so the control logic is easier to inspect and maintain:

- `SCARA_ROBOT_ARM_CODE.ino` — hardware configuration, shared state, setup and loop
- `01_DriverControl.ino` — driver enable/disable and emergency stop
- `02_CalibrationAndStorage.ino` — raw calibration and persistent settings
- `03_JointCoordinates.ino` — joint/raw-step conversions
- `04_ForwardKinematics.ino` — FK
- `05_InverseKinematics.ino` — IK
- `06_MotionAndSafety.ino` — coordinated motion and safety checks
- `07_HomingSequence.ino` — homing workflow
- `08_WebApi.ino` — browser/API control layer
- `09_SerialDebug.ino` — serial diagnostics
- `10_GripperControl.ino` — servo gripper
- `11_CollisionSafety.ino` — collision envelopes and safe routing
- `12_PackageRecipes.ino` — stored automation recipes
- `13_ArmJobController.ino` — higher-level job execution
- `webpage.h` — embedded browser interface

## Before flashing

1. Open `SCARA_ROBOT_ARM_CODE.ino`.
2. Replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` with your local network details if STA mode is required.
3. Verify motor direction, steps-per-unit, joint limits, homing direction, and saved calibration against your physical robot.
4. Read [`../docs/software/CALIBRATION_GUIDE.md`](../docs/software/CALIBRATION_GUIDE.md) before commissioning.

Wi-Fi passwords from the earlier development repository are intentionally not copied into this canonical repository.
