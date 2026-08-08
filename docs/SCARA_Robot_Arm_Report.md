# SCARA Robot Arm — Engineering Report

**Graduation Project — Abdulrahman Alsaadi**

## 1. Project Objective & Specifications

This project covers the design, construction, and control of a custom-built **3-axis (3-DOF) SCARA robotic arm with a servo gripper**. The project explores robotic kinematics, mechanical transmission, and microcontroller-based automation using accessible 3D-printed components.

The SCARA configuration was selected for mechanical efficiency. The main arm motion is constrained to the horizontal plane while the structural load is carried by bearings and a rigid Z-axis pillar. Three NEMA 17 stepper motors provide the three controlled robot axes, while an MG90S servo actuates the gripper as the end effector.

### Core dimensions

- Vertical Z stroke: **290 mm**
- Fixed base offset: **L1 = 205 mm**
- Rotational link: **L2 = 222 mm**
- Rotational link: **L3 = 156.8 mm**
- Approximate maximum reach: **584 mm**

## 2. Mechanical Design

### 2.1 Z-axis: actuation and deflection mitigation

A long 3D-printed arm is vulnerable to cantilever deflection when extended. To reduce sag and prevent the lifting mechanism from jamming, the Z-axis separates lifting from structural support.

- An **8 mm lead screw and nut** provide vertical actuation.
- Two **10 mm steel linear shafts** carry bending loads.
- **LM10UU linear bearings** guide the moving structure.
- The stepper motor therefore focuses on lead-screw actuation instead of directly carrying structural bending load.

For an estimated moving mass of about 2.5 kg, the vertical system must support approximately 24.52 N of gravitational force.

### 2.2 Link 1 to Link 2: compound belt transmission

Power from Motor 2 is transferred across the fixed 205 mm L1 section using a two-stage GT2 belt reduction rather than a long train of printed gears.

The transmission includes:

- 400 mm GT2 belt
- Compound 23/80-tooth pulley supported by 608 bearings
- 300 mm GT2 belt
- 92-tooth pulley at the L2 joint

The reduction increases available torque and angular resolution while reducing backlash compared with a long printed gear train.

### 2.3 Link 2 to Link 3

Motor 3 is mounted close to Joint 2 instead of at the arm tip to reduce swing weight and rotational inertia. A 400 mm GT2 belt transfers motion to a 90-tooth pulley at Joint 3, which is connected to a custom 3D-printed coupler supporting Link 3 and the gripper assembly.

### 2.4 Joint bearings

The major rotary joints combine axial and radial bearing functions:

- **35 × 52 × 12 mm thrust bearings** carry downward axial load.
- **30 × 42 × 7 mm radial ball bearings** keep the rotating shaft centered.

This arrangement prevents the stepper motor shafts from directly supporting the full vertical load of the arm.

## 3. Electronic Architecture and Control

### 3.1 Power distribution

The system uses separate power levels for motor actuation and low-voltage electronics.

- A **12 V switched-mode power supply** feeds the stepper motor drivers.
- A **5 V DC-DC buck converter** supplies the low-voltage control electronics and MG90S servo gripper.
- **100 µF electrolytic capacitors** are installed across the motor-power input of the stepper drivers to reduce voltage-spike risk.

### 3.2 Main controller

The **ESP32** is the main controller. Its processing capability supports the trigonometric calculations required for forward and inverse kinematics, while integrated Wi-Fi enables browser-based operation and communication with external systems.

### 3.3 Motor control

The ESP32 sends STEP and DIR signals to **DRV8825** stepper motor drivers, which control the three NEMA 17 motors. The MG90S gripper servo is controlled with PWM and is treated as the end-effector actuator rather than an additional robot axis.

### 3.4 Homing and calibration

Because the stepper motors operate open-loop, limit switches provide physical reference points after startup. The homing process establishes a known coordinate reference before normal joint-space or Cartesian motion.

## 4. Software Architecture

The embedded software combines motion control, kinematics, monitoring, calibration, stored operations, and communication services.

### Motion Control Module

- Joint movement control
- Stepper pulse generation
- Speed and acceleration control
- Homing execution
- Position tracking

### Kinematics Module

- Forward Kinematics (FK)
- Inverse Kinematics (IK)
- Cartesian coordinate conversion
- Joint coordinate conversion

The robot can therefore be commanded using joint coordinates or Cartesian coordinates.

### Recipe Management

The project architecture supports saved robot operations consisting of positions and actions that can be replayed. Persistent storage is used to retain configuration and saved operations after power cycling.

### Embedded Web Server

The ESP32 web interface provides:

- Real-time robot monitoring
- Manual joint control
- Cartesian control
- Recipe management
- System status
- Calibration controls

### State Management

Representative operating states include:

- Not Homed
- Ready
- Running Recipe
- Paused
- Error
- Emergency Stop

### Communication

The software architecture supports HTTP API communication, dashboard integration, remote commands, status reporting, and preparation for wider robotic-cell integration using MQTT concepts.

## 5. Project Outcome

The completed arm demonstrates the integration of mechanical design, 3D printing, belt transmission design, electronics, embedded programming, motion control, kinematics, calibration, and web-based robot operation in one **3-axis SCARA subsystem with a servo gripper**.

## Complete 3D Model

The complete downloadable 3D design is available at:

**[SCARA Robot Arm on Cults3D](https://cults3d.com/en/3d-model/gadget/scara-robot-arm-alsaadi)**
