/*
  Tab: 10_GripperControl.ino
  Purpose: Drives and calibrates the servo gripper.

  Electrical:
    Servo signal -> GPIO 13
    Servo V+     -> external regulated 5 V supply
    Servo GND    -> external supply ground and ESP32 common ground

  Safety:
    The operational CLOSE / HOLD command uses the saved box-holding angle.
    The slider cannot move past the saved maximum-close angle.
    The hard software maximum is 120 degrees; a 180-degree command is rejected.
*/

// =====================================================
// Stored gripper positions
// =====================================================

bool validateGripperCalibration(uint8_t openDeg, uint8_t holdDeg, uint8_t maxCloseDeg, String &error) {
  if (maxCloseDeg > GRIPPER_HARD_MAX_DEG) {
    error = "Maximum close angle exceeds the hard gripper limit.";
    return false;
  }

  if (openDeg > holdDeg || holdDeg > maxCloseDeg) {
    error = "Gripper settings must satisfy OPEN <= HOLD <= MAX CLOSE.";
    return false;
  }

  error = "";
  return true;
}

void loadGripperSettings() {
  gripperStore.begin("scara_grip", false);
  gripperOpenDeg = gripperStore.getUChar("open", DEFAULT_GRIPPER_OPEN_DEG);
  gripperHoldDeg = gripperStore.getUChar("hold", DEFAULT_GRIPPER_HOLD_DEG);
  gripperMaxCloseDeg = gripperStore.getUChar("maxclose", DEFAULT_GRIPPER_MAX_CLOSE_DEG);

  String error;
  if (!validateGripperCalibration(gripperOpenDeg, gripperHoldDeg, gripperMaxCloseDeg, error)) {
    resetGripperSettings();
  }

  gripperCommandDeg = gripperOpenDeg;
}

void saveGripperSettings() {
  gripperStore.putUChar("open", gripperOpenDeg);
  gripperStore.putUChar("hold", gripperHoldDeg);
  gripperStore.putUChar("maxclose", gripperMaxCloseDeg);
}

void resetGripperSettings() {
  gripperOpenDeg = DEFAULT_GRIPPER_OPEN_DEG;
  gripperHoldDeg = DEFAULT_GRIPPER_HOLD_DEG;
  gripperMaxCloseDeg = DEFAULT_GRIPPER_MAX_CLOSE_DEG;
  saveGripperSettings();
}

// =====================================================
// PWM command generation
// =====================================================

uint32_t gripperAngleToDuty(uint8_t angleDeg) {
  const uint32_t maxDuty = (1UL << GRIPPER_PWM_RESOLUTION_BITS) - 1UL;
  const uint32_t periodUs = 1000000UL / GRIPPER_PWM_FREQUENCY_HZ;
  const uint32_t pulseUs = GRIPPER_SERVO_MIN_PULSE_US +
    ((uint32_t)angleDeg * (GRIPPER_SERVO_MAX_PULSE_US - GRIPPER_SERVO_MIN_PULSE_US)) / 180UL;

  return (pulseUs * maxDuty) / periodUs;
}

bool gripperBegin(String &error) {
  if (!ledcAttach(GRIPPER_SERVO_PIN, GRIPPER_PWM_FREQUENCY_HZ, GRIPPER_PWM_RESOLUTION_BITS)) {
    gripperAttached = false;
    error = "Could not attach servo PWM on GPIO 13.";
    return false;
  }

  gripperAttached = true;
  return commandGripperOpen(error);
}

bool commandGripperAngle(int angleDeg, String &error) {
  if (!gripperAttached) {
    error = "Gripper PWM is not attached.";
    return false;
  }

  if (angleDeg < gripperOpenDeg || angleDeg > gripperMaxCloseDeg) {
    error = "Command outside saved OPEN to MAX CLOSE range.";
    return false;
  }

  if (angleDeg > GRIPPER_HARD_MAX_DEG) {
    error = "Command rejected by hard close protection.";
    return false;
  }

  if (!ledcWrite(GRIPPER_SERVO_PIN, gripperAngleToDuty((uint8_t)angleDeg))) {
    error = "Could not write servo PWM command.";
    return false;
  }

  gripperCommandDeg = (uint8_t)angleDeg;
  error = "";
  return true;
}

bool commandGripperOpen(String &error) {
  return commandGripperAngle(gripperOpenDeg, error);
}

bool commandGripperHold(String &error) {
  return commandGripperAngle(gripperHoldDeg, error);
}
