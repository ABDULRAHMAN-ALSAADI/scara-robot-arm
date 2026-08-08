# Electronics and Wiring

## Power architecture

The robot uses separate voltage levels for motor power and low-voltage electronics:

- **12 V rail:** DRV8825 motor-power inputs.
- **5 V regulated rail:** ESP32/logic supply as appropriate for the selected board, and the servo supply.
- **Common ground:** all control and power grounds must share the required reference.
- **DRV8825 protection:** place a 100 µF electrolytic capacitor close to each driver motor-power input.

Do not power a loaded servo directly from a weak ESP32 regulator. Use the regulated 5 V supply and common ground.

## Legacy firmware pin mapping

The included legacy v10 firmware uses the following ESP32 mapping:

| Function | STEP / Signal | DIR | ENABLE | Limit |
|---|---:|---:|---:|---:|
| A1 shoulder | GPIO 17 | GPIO 16 | GPIO 5 | GPIO 14 |
| A2 elbow | GPIO 19 | GPIO 18 | GPIO 21 | GPIO 27 |
| Z-axis | GPIO 2 | GPIO 15 | GPIO 4 | GPIO 26 |

Later project revisions added the servo gripper on **GPIO 13**. Treat the legacy snapshot as reference code, not as a guaranteed final wiring specification.

## Commissioning checks

Before applying full motor power:

1. Verify 12 V and 5 V rails with a multimeter.
2. Verify common ground.
3. Set each DRV8825 current limit before sustained motion.
4. Test every limit switch manually.
5. Confirm the HOME direction of each axis at low speed.
6. Keep motor power immediately accessible during the first homing test.
7. Verify the gripper clears the base through the entire homing path.
8. Verify joint limits before enabling full-range FK/IK motion.
