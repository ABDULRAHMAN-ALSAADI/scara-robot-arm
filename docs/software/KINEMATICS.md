# SCARA Robot Arm — Kinematics Reference

**Firmware Version:** v10.22  
**Relevant Source Files:** `03_JointCoordinates.ino`, `04_ForwardKinematics.ino`, `05_InverseKinematics.ino`, `06_MotionAndSafety.ino`, `11_CollisionSafety.ino`

---

## Table of Contents

1. [Robot Geometry](#robot-geometry)
2. [Coordinate System](#coordinate-system)
3. [Joint-to-Step Conversion](#joint-to-step-conversion)
4. [Forward Kinematics (FK)](#forward-kinematics-fk)
5. [Inverse Kinematics (IK)](#inverse-kinematics-ik)
6. [IK Configuration Selection](#ik-configuration-selection)
7. [Joint Limits & Safety Clamping](#joint-limits--safety-clamping)
8. [Motion Planning & Queuing](#motion-planning--queuing)
9. [Collision Envelope System](#collision-envelope-system)
10. [Auto Safe Route](#auto-safe-route)
11. [Velocity Monitoring](#velocity-monitoring)
12. [Tuning Parameters](#tuning-parameters)

---

## Robot Geometry

This is a **2-link planar SCARA arm** with a vertical Z axis. The end-effector position is determined by three values:

- **A1** — Shoulder joint angle (degrees)
- **A2** — Elbow joint angle (degrees)
- **Z** — Vertical axis position (millimeters)

The two arm segments are defined as:

| Parameter | Symbol | Value | Description |
|---|---|---|---|
| Upper arm length | `L1` | **136.5 mm** | Shoulder pivot to elbow pivot |
| Tool Center Point | `toolTcpLengthMm` | **120.0 mm** *(editable)* | Elbow pivot to grasp center |

`toolTcpLengthMm` can be updated at runtime via the web interface and is saved to NVS. It is effectively **L2** in the kinematic equations.

<img width="5100" height="2022" alt="B2 Z-AXIS ARM BASE ASSEMBLY98" src="https://github.com/user-attachments/assets/977fb36b-cf99-4fac-908b-73b847c02684" />


**Maximum reach** (arm fully extended):
```
maxReach = L1 + toolTcpLengthMm = 136.5 + 120.0 = 256.5 mm
```

**Minimum reach** (arm fully folded, singular point):
```
minReach = |L1 - toolTcpLengthMm| = |136.5 - 120.0| = 16.5 mm
```

Any XY target with a radial distance outside `[minReach, maxReach]` is rejected by the IK solver.

---

## Coordinate System

### XY Plane

The origin is the shoulder pivot (A1 rotation axis). Positive X points forward from the base when A1 = 0°. The XY plane is the robot's working plane at any given Z height.

```
         +Y
          |
          |
 -X ──────┼────── +X    (shoulder pivot at origin)
          |
          |
         -Y
```

### Z Axis

Positive Z moves **downward** (increasing raw steps from the top HOME switch). The web interface and IK coordinates express Z in millimeters relative to the calibrated zero point.

### A1 (Shoulder)

- Away from the HOME switch = **positive** raw steps → **positive degrees**
- Default range: **−160° to +170°**
- HOME switch is on the negative-angle side

### A2 (Elbow)

- Away from the HOME switch = **negative** raw steps → **positive degrees**
- The raw step direction is inverted relative to A1 (see conversion formulas below)
- Default range: **−150° to +160°**
- HOME switch is on the negative raw step / positive-angle side

---

## Joint-to-Step Conversion

Source: `03_JointCoordinates.ino`

Each axis converts between physical units (degrees or mm) and stepper motor raw steps using the calibrated zero position (`a1ZeroSteps`, `a2ZeroSteps`, `zZeroSteps`) stored in NVS.

### Angle to Steps

```cpp
// A1: positive angle = more steps away from home switch
long a1AngleToSteps(float angleDeg) {
    return a1ZeroSteps + lround(angleDeg * A1_STEPS_PER_DEG);
}

// A2: inverted — positive angle = fewer steps (away from switch in negative direction)
long a2AngleToSteps(float angleDeg) {
    return a2ZeroSteps - lround(angleDeg * A2_STEPS_PER_DEG);
}

// Z: positive mm = more steps downward from top HOME
long zMmToSteps(float zMM) {
    return zZeroSteps + lround(zMM * Z_STEPS_PER_MM);
}
```

> A2's formula uses **subtraction** because its motor runs in the opposite direction from A1. If you reverse the A2 motor wiring, this formula must be updated to match.

### Steps to Angle

```cpp
float a1StepsToAngle(long steps) {
    return (float)(steps - a1ZeroSteps) / A1_STEPS_PER_DEG;
}

float a2StepsToAngle(long steps) {
    return (float)(a2ZeroSteps - steps) / A2_STEPS_PER_DEG;
}

float zStepsToMm(long steps) {
    return (float)(steps - zZeroSteps) / Z_STEPS_PER_MM;
}
```

### Resolution Constants

| Axis | Steps per Unit | Unit |
|---|---|---|
| A1 (Shoulder) | 142.22 | steps / degree |
| A2 (Elbow) | 40.0 | steps / degree |
| Z Axis | 400.0 | steps / mm |

These values are set at compile time in the main `.ino` file. If you change your drive ratio, belt pulley count, or lead screw pitch, update these constants and reflash.

### Reading Current Position

```cpp
float actualA1() { return a1StepsToAngle(arm1.currentPosition()); }
float actualA2() { return a2StepsToAngle(arm2.currentPosition()); }
float actualZ()  { return zStepsToMm(zAxis.currentPosition()); }
```

These read directly from the AccelStepper position counter, which is authoritative as long as no steps have been lost.

---

## Forward Kinematics (FK)

Source: `04_ForwardKinematics.ino`

FK calculates the XY position of the Tool Center Point from the current joint angles. Z passes through unchanged.

### Formula

```cpp
Position forwardKinematics(float a1Deg, float a2Deg, float zMM) {
    float a1 = radians(a1Deg);
    float a2 = radians(a2Deg);

    Position p;
    p.x = L1 * cos(a1) + toolTcpLengthMm * cos(a1 + a2);
    p.y = L1 * sin(a1) + toolTcpLengthMm * sin(a1 + a2);
    p.z = zMM;
    return p;
}
```

**In mathematical notation:**
```
x = L1·cos(θ₁) + L2·cos(θ₁ + θ₂)
y = L1·sin(θ₁) + L2·sin(θ₁ + θ₂)
z = z
```

where θ₁ = A1 in radians, θ₂ = A2 in radians, L1 = shoulder link length, L2 = TCP length.

### Angle Normalization

Used to keep angle results within [−180°, +180°] after arithmetic:

```cpp
float normalizeAngle(float angleDeg) {
    while (angleDeg > 180.0f)  angleDeg -= 360.0f;
    while (angleDeg < -180.0f) angleDeg += 360.0f;
    return angleDeg;
}
```

---

## Inverse Kinematics (IK)

Source: `05_InverseKinematics.ino`

IK calculates the joint angles (A1, A2) required to reach a given XY position. Z is independent and passed directly.

### Algorithm

**Step 1 — Check reachability:**
```
distance = sqrt(x² + y²)
maxReach = L1 + L2
minReach = |L1 - L2|

if distance > maxReach OR distance < minReach → target rejected
```

**Step 2 — Solve A2 (elbow angle):**
```
cos(θ₂) = (x² + y² − L1² − L2²) / (2 · L1 · L2)

θ₂ = atan2(±sin(θ₂), cos(θ₂))
```

The sign of sin(θ₂) determines the elbow configuration:
- **Elbow-Down** (`elbowDownConfiguration = true`): sin(θ₂) is **positive** → A2 > 0
- **Elbow-Up** (`elbowDownConfiguration = false`): sin(θ₂) is **negative** → A2 < 0

**Step 3 — Solve A1 (shoulder angle):**
```
θ₁ = atan2(y, x) − atan2(L2·sin(θ₂), L1 + L2·cos(θ₂))
```

**Step 4 — Resolve ±180° wrap ambiguity:**

At the ±180° wrap boundary, the same XY orientation can be represented by −180° or +180°. The solver evaluates three candidates (base − 360°, base, base + 360°) and selects the one that:

1. Falls within calibrated A1 limits (with `IK_ROUNDING_JOINT_TOL_DEG = 0.5°` tolerance)
2. Is nearest to the **current A1 position** (minimum joint travel)
3. In a tie, prefers the positive-side equivalent (the negative side is the home switch side and is deliberately more restricted)

**Step 5 — Clamp A2 and verify:**

A2 is constrained to the calibrated safe range. The result is then verified by running FK on the solution and checking that the position error is within:

```
IK_MAX_CLAMP_POSITION_ERROR_MM = 1.0 mm
```

If the error exceeds this, the IK solution is rejected.

### IK Result Structure

```cpp
struct IKSolution {
    bool   ok;     // true if a valid solution was found
    float  a1;     // computed shoulder angle (degrees)
    float  a2;     // computed elbow angle (degrees)
    String error;  // "OK" or a descriptive failure message
};
```

---

## IK Configuration Selection

Source: `05_InverseKinematics.ino`

When no specific configuration is requested, `selectNearestIKConfiguration()` automatically picks the best one:

```
1. Try both Elbow-Down and Elbow-Up solutions
2. If only one is valid → use it
3. If both are valid → pick the one with least total joint travel:
      cost = |ΔA1| + |ΔA2| from current pose
4. If neither is valid → return failure
```

This minimises unnecessary arm movement and avoids sudden large-angle swings when crossing the workspace.

---

## Joint Limits & Safety Clamping

Joint limits are stored as **raw step counts** from the HOME switches and converted to degree values at runtime via `updateDerivedJointCoordinates()`. This means physical endpoints stay correct even if the geometric zero is later recalibrated.

### Default Operating Ranges

| Joint | Min | Max |
|---|---|---|
| A1 (Shoulder) | −160.0° | +170.0° |
| A2 (Elbow) | −150.0° | +160.0° |
| Z | −135.0 mm | +135.0 mm |

These are derived defaults; actual limits are what you save during commissioning calibration.

### IK Rounding Tolerance

The IK solver accepts solutions that are slightly outside the calibrated limits (within 0.5°) and then clamps them back before executing. This handles floating-point rounding at the exact boundary without rejecting valid workspace points:

```cpp
constexpr float IK_ROUNDING_JOINT_TOL_DEG = 0.50f;    // accepted overshoot before clamping
constexpr float IK_MAX_CLAMP_POSITION_ERROR_MM = 1.00f; // max error after clamping
```

---

## Motion Planning & Queuing

Source: `06_MotionAndSafety.ino`

All joint moves — whether from FK jogging, IK, or recipe playback — pass through `queueJointMove()`, which applies safety checks before commanding the motors.

### Move Acceptance Flow

```
queueJointMove(a1, a2, z, profile)
  │
  ├─ [blocked if] recipe is playing (unless called internally by recipe engine)
  ├─ [blocked if] autonomous job is active
  ├─ [blocked if] not homed
  ├─ [blocked if] homing in progress
  ├─ [blocked if] fault active
  ├─ [blocked if] safe route is already running
  ├─ [blocked if] target is outside calibrated joint limits
  ├─ [blocked if] target pose violates a collision envelope
  │
  ├─ collision path check (start → target, direct line):
  │     OK → queueDirectJointMove()
  │     BLOCKED → startCollisionSafeRoute()
  │
  └─ motion executes via AccelStepper
```

### Direct Move

When the direct path is clear, all three axes start simultaneously and run to their targets using AccelStepper acceleration profiles:

```cpp
arm1.moveTo(a1AngleToSteps(a1Deg));
arm2.moveTo(a2AngleToSteps(a2Deg));
zAxis.moveTo(zMmToSteps(zMM));
```

### Motion Service Slicing

The main `loop()` calls `runMotionServiceSlice()` which services AccelStepper in time-bounded slices, allowing the web server to remain responsive between motor steps:

| Profile | Slice Duration |
|---|---|
| FK / IK point-to-point | 5000 µs |
| Real-Time (slider dragging) | 1500 µs |

During homing, slicing is disabled and motor service runs blocking.

### Move Timeout

Any move that does not complete within 60 seconds triggers an emergency stop:

```cpp
constexpr unsigned long MOVE_TIMEOUT_MS = 60000UL;
```

---

## Collision Envelope System

Source: `11_CollisionSafety.ino`

The collision system prevents the arm from entering forbidden zones by enforcing **Z-layered directional A2 limits** as a function of A1 angle.

### Concept

A **collision envelope layer** defines a boundary curve in the A1/A2 plane, active only within a specified Z height range. Layers are independent so different collision boundaries can be set for different Z heights (e.g. the arm can swing further at high Z than at low Z where objects are present).

Each layer has four properties:

| Property | Options | Meaning |
|---|---|---|
| A1 Region | `POSITIVE` / `NEGATIVE` | Which side of the A1 range this layer applies to |
| A2 Boundary Direction | `MAXIMUM` / `MINIMUM` | A2 must stay **below** or **above** the limit |
| Z Range | `zFrom` → `zTo` (mm) | The vertical band where this layer is active |
| Boundary Points | Up to 16 `(A1, A2_limit)` pairs | The piecewise-linear limit curve |

**Limits table:**

| Constant | Value |
|---|---|
| Max layers | 12 |
| Max boundary points per layer | 16 |

### Boundary Interpolation

The limit curve between boundary points is **linearly interpolated**. When the robot's A1 angle falls between two saved points, the A2 limit is computed as:

```
f = (a1 − a1_A) / (a1_B − a1_A)
limit = a2_A + f * (a2_B − a2_A)
```

Points outside the defined range use the nearest endpoint's limit value (flat extension).

An optional **margin** (0–20°) is subtracted from MAXIMUM limits and added to MINIMUM limits as a safety buffer:

```cpp
if (maximum) limitDeg -= collisionEnvelopeMarginDeg;
else         limitDeg += collisionEnvelopeMarginDeg;
```

### Pose Check

Before any move is accepted, the **target pose** is checked:

```
for each active layer at current Z:
    interpolate A2 limit at target A1
    if A2 violates limit → reject move
```

### Path Check

The **entire path** from current pose to target is also checked (not just the endpoint). The path check computes the tightest (most restrictive) interpolated limit across the full A1 sweep of the move, and checks whether the A2 range of the move would violate it:

```
a1Low  = min(startA1, targetA1)
a1High = max(startA1, targetA1)

for each layer overlapping the Z move:
    find tightest A2 limit across [a1Low, a1High]
    if A2 range of move violates that limit → path blocked → use safe route
```

### Enabling / Disabling

Collision protection is toggled at runtime from the web interface and saved to NVS. Protection cannot be enabled if no boundary points exist, or if the current pose already violates an active layer.

---

## Auto Safe Route

Source: `06_MotionAndSafety.ino`

When `queueJointMove()` finds that the **direct path is blocked** by a collision envelope, it automatically plans a three-phase detour through a saved **A2 safe corridor** angle:

### Three Phases

```
Phase 1: CLEAR_A2_TO_ZERO
    Move A2 to the saved corridor angle (at current A1 and Z)

Phase 2: MOVE_A1_AT_ZERO
    Move A1 to the target angle (A2 held at corridor angle)

Phase 3: FINAL_TARGET
    Move A2 and Z to their final target values
```

Each phase is checked against the collision envelope before executing. If any phase cannot be planned safely, the entire move is rejected with an error message.

### Safe Corridor Setting

The A2 corridor angle is configured from the web interface and saved to NVS:

```cpp
float collisionSafeCorridorA2Deg;  // must be within calibrated A2 limits
```

This angle should be set to an A2 position where A1 can rotate freely without risk of collision — typically near A2 = 0° (arm straight out).

### Route Status

The current routing phase is visible in the web interface status:

| Phase | Display Label |
|---|---|
| No route active | `Direct / idle` |
| Phase 1 | `Safe route: A2 -> saved corridor` |
| Phase 2 | `Safe route: move A1 at saved corridor` |
| Phase 3 | `Safe route: final A2/Z target` |

---

## Velocity Monitoring

Source: `06_MotionAndSafety.ino`

The firmware tracks peak joint velocities during each move to confirm that motion profiles are being applied correctly:

```cpp
float movePeakA1DegS;   // peak A1 speed in deg/s during current/last move
float movePeakA2DegS;   // peak A2 speed in deg/s
float movePeakZMmS;     // peak Z speed in mm/s
```

These are reset at the start of each move and updated every servo cycle by converting raw AccelStepper step rates to physical units:

```cpp
float a1DegS = fabs(arm1.speed()) / A1_STEPS_PER_DEG;
float a2DegS = fabs(arm2.speed()) / A2_STEPS_PER_DEG;
float zMmS   = fabs(zAxis.speed()) / Z_STEPS_PER_MM;
```

Peak values are visible in the web interface status panel and can be used to verify that speed percentage settings are having their intended effect.

---

## Tuning Parameters

All of the following are defined as `constexpr` in `SCARA_ROBOT_ARM_CODE.ino` and require a reflash to change.

| Parameter | Value | Effect |
|---|---|---|
| `L1` | 136.5 mm | Shoulder arm length — must match physical build |
| `A1_STEPS_PER_DEG` | 142.22 | A1 resolution — change if drive ratio changes |
| `A2_STEPS_PER_DEG` | 40.0 | A2 resolution |
| `Z_STEPS_PER_MM` | 400.0 | Z resolution |
| `IK_ROUNDING_JOINT_TOL_DEG` | 0.5° | Tolerance for IK limit clamping |
| `IK_MAX_CLAMP_POSITION_ERROR_MM` | 1.0 mm | Max FK verification error after IK clamping |
| `DEFAULT_ENVELOPE_MARGIN_DEG` | 0.0° | Default safety margin added to collision limits |
| `MOVE_TIMEOUT_MS` | 60000 ms | Emergency stop if a move takes longer than this |
| `POINT_TO_POINT_MOTION_SLICE_US` | 5000 µs | Web server yield interval during FK/IK moves |
| `REALTIME_MOTION_SLICE_US` | 1500 µs | Web server yield interval during real-time slider moves |

The following are editable at runtime via the web interface and saved to NVS:

| Parameter | Default | Where to Change |
|---|---|---|
| `toolTcpLengthMm` | 120.0 mm | Web → Tool Settings → TCP Length |
| `collisionSafeCorridorA2Deg` | 0.0° | Web → Collision → Safe Corridor |
| `collisionEnvelopeMarginDeg` | 0.0° | Web → Collision → Envelope Margin |
| `manualFkSpeedPct` | 60% | Web → Motion Profile → Manual FK |
| `realtimeSpeedPct` | 35% | Web → Motion Profile → Real-Time |
| `ikSpeedPct` | 60% | Web → Motion Profile → IK |

---

*Generated from firmware source: SCARA v10.22*
