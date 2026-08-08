# Bill of Materials

This BOM captures the main components documented for the SCARA arm. Verify quantities, dimensions, fasteners, and exact mechanical variants against the final CAD before purchasing or rebuilding.

| Category | Component | Qty | Notes |
|---|---|---:|---|
| Controller | ESP32 development board | 1 | Main robot controller |
| Motion | NEMA 17 stepper motor | 3 | A1, A2, Z |
| Drivers | DRV8825 stepper driver | 3 | One per stepper |
| End effector | MG90S micro servo | 1 | Gripper |
| Homing | Limit switch | 3 | A1, A2, Z reference |
| Power | 12 V switched-mode power supply | 1 | Stepper-driver power rail |
| Power | 5 V DC-DC buck converter | 1 | ESP32 / servo low-voltage rail |
| Protection | 100 µF electrolytic capacitor | 3 | Across each DRV8825 motor-power input |
| Transmission | GT2 timing belts | As required | Joint transmission |
| Transmission | GT2 pulleys / idlers | As required | Includes reduction stages |
| Z-axis | Lead screw and nut | 1 set | Vertical actuation |
| Z-axis | 10 mm steel guide shafts | As required | Structural guidance |
| Z-axis | LM10UU linear bearings | As required | Linear guidance |
| Joints | 35 × 52 × 12 mm thrust bearings | As required | Axial load support in rotary joints |
| Joints | 30 × 42 × 7 mm radial bearings | As required | Shaft centering |
| Structure | 3D-printed parts | Set | Base, links, mounts, covers, gripper parts |
| Hardware | Shafts, bolts, nuts, washers, spacers | As required | Match final CAD |
| Wiring | Wire, connectors, terminals | As required | Motor, logic, switch and power wiring |

## Before ordering

1. Confirm the final CAD revision.
2. Check shaft and bearing fits.
3. Confirm pulley tooth counts and belt lengths.
4. Set DRV8825 current limits for the actual NEMA 17 motors.
5. Size the power supply for the installed motors and expected simultaneous load.
