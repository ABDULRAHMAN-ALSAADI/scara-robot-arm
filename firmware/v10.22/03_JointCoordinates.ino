/*
  Tab: 03_JointCoordinates.ino
  Purpose: Converts between real joint angles or Z millimeters and stepper raw step positions.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Correct joint-coordinate to raw-step mapping
// =====================================================
//
// Arm 1:
//   physical switch is the negative end.
//   Home sets raw switch position = 0.
//   Moving away from the switch increases raw steps.
//
// Arm 2:
//   physical switch is also the negative angle end.
//   Your old formula was reversed and made +160 go to the switch.
//   Home sets raw switch position = 0.
//   Moving away from the switch decreases raw steps.
//
// Z:
//   top switch is negative Z.
//   Home sets raw switch position = 0.
//   Moving downward increases raw steps.
// =====================================================

long a1AngleToSteps(float angleDeg) {
  return a1ZeroSteps + lround(angleDeg * A1_STEPS_PER_DEG);
}

long a2AngleToSteps(float angleDeg) {
  return a2ZeroSteps - lround(angleDeg * A2_STEPS_PER_DEG);
}

long zMmToSteps(float zMM) {
  return zZeroSteps + lround(zMM * Z_STEPS_PER_MM);
}

float a1StepsToAngle(long steps) {
  return ((float)(steps - a1ZeroSteps) / A1_STEPS_PER_DEG);
}

float a2StepsToAngle(long steps) {
  return ((float)(a2ZeroSteps - steps) / A2_STEPS_PER_DEG);
}

float zStepsToMm(long steps) {
  return ((float)(steps - zZeroSteps) / Z_STEPS_PER_MM);
}

float actualA1() {
  return a1StepsToAngle(arm1.currentPosition());
}

float actualA2() {
  return a2StepsToAngle(arm2.currentPosition());
}

float actualZ() {
  return zStepsToMm(zAxis.currentPosition());
}
