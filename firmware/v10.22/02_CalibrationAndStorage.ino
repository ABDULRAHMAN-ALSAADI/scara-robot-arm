/*
  Tab: 02_CalibrationAndStorage.ino
  Purpose: Stores physical machine calibration, motion profiles, TCP length,
           and controlled setup-jog behavior.

  Design rule:
    A physical endpoint is saved as a raw step position from the HOME switch.
    It is not saved as a degree value. Degree limits are derived from the
    current zero calibration and the raw endpoints.
*/

// =====================================================
// Raw machine calibration storage
// =====================================================

void updateDerivedJointCoordinates() {
  a1MinDeg = a1StepsToAngle(a1NegativeLimitRawSteps);
  a1MaxDeg = a1StepsToAngle(a1PositiveLimitRawSteps);
  a2MinDeg = a2StepsToAngle(a2NegativeLimitRawSteps);
  a2MaxDeg = a2StepsToAngle(a2PositiveLimitRawSteps);
  a2HomeClearanceParkDeg = a2StepsToAngle(a2HomeClearanceParkRawSteps);
  zMinMm = zStepsToMm(zUpperLimitRawSteps);
  zMaxMm = zStepsToMm(zLowerLimitRawSteps);
}

void loadCalibration() {
  // New namespace: old angle-based endpoint storage is intentionally ignored.
  // This prevents physically correct endpoints from being reinterpreted through
  // a changed geometric zero.
  calibrationStore.begin("scara_raw_v2", false);

  a1ZeroSteps = calibrationStore.getLong("a1zero", DEFAULT_A1_ZERO_RAW_STEPS);
  a2ZeroSteps = calibrationStore.getLong("a2zero", DEFAULT_A2_ZERO_RAW_STEPS);

  a1NegativeLimitRawSteps = calibrationStore.getLong("a1neg", DEFAULT_A1_NEG_RAW_STEPS);
  a1PositiveLimitRawSteps = calibrationStore.getLong("a1pos", DEFAULT_A1_POS_RAW_STEPS);
  a2NegativeLimitRawSteps = calibrationStore.getLong("a2neg", DEFAULT_A2_NEG_RAW_STEPS);
  a2PositiveLimitRawSteps = calibrationStore.getLong("a2pos", DEFAULT_A2_POS_RAW_STEPS);
  a2HomeClearanceParkRawSteps = calibrationStore.getLong("a2park", DEFAULT_A2_PARK_RAW_STEPS);

  // New in v9.4. Existing A1/A2 raw calibration remains in the same namespace.
  zZeroSteps = calibrationStore.getLong("zzero", DEFAULT_Z_ZERO_RAW_STEPS);
  zUpperLimitRawSteps = calibrationStore.getLong("ztop", DEFAULT_Z_UPPER_RAW_STEPS);
  zLowerLimitRawSteps = calibrationStore.getLong("zbottom", DEFAULT_Z_LOWER_RAW_STEPS);

  updateDerivedJointCoordinates();
}

void saveCalibration() {
  calibrationStore.putLong("a1zero", a1ZeroSteps);
  calibrationStore.putLong("a2zero", a2ZeroSteps);
  calibrationStore.putLong("a1neg", a1NegativeLimitRawSteps);
  calibrationStore.putLong("a1pos", a1PositiveLimitRawSteps);
  calibrationStore.putLong("a2neg", a2NegativeLimitRawSteps);
  calibrationStore.putLong("a2pos", a2PositiveLimitRawSteps);
  calibrationStore.putLong("a2park", a2HomeClearanceParkRawSteps);
  calibrationStore.putLong("zzero", zZeroSteps);
  calibrationStore.putLong("ztop", zUpperLimitRawSteps);
  calibrationStore.putLong("zbottom", zLowerLimitRawSteps);

  updateDerivedJointCoordinates();
}

void resetCalibration() {
  a1ZeroSteps = DEFAULT_A1_ZERO_RAW_STEPS;
  a2ZeroSteps = DEFAULT_A2_ZERO_RAW_STEPS;
  a1NegativeLimitRawSteps = DEFAULT_A1_NEG_RAW_STEPS;
  a1PositiveLimitRawSteps = DEFAULT_A1_POS_RAW_STEPS;
  a2NegativeLimitRawSteps = DEFAULT_A2_NEG_RAW_STEPS;
  a2PositiveLimitRawSteps = DEFAULT_A2_POS_RAW_STEPS;
  a2HomeClearanceParkRawSteps = DEFAULT_A2_PARK_RAW_STEPS;
  zZeroSteps = DEFAULT_Z_ZERO_RAW_STEPS;
  zUpperLimitRawSteps = DEFAULT_Z_UPPER_RAW_STEPS;
  zLowerLimitRawSteps = DEFAULT_Z_LOWER_RAW_STEPS;
  saveCalibration();
}

// =====================================================
// Motion profile storage
// =====================================================

void loadMotionProfiles() {
  motionStore.begin("scara_motion", false);
  manualFkSpeedPct = constrain((int)motionStore.getUChar("manual", 60), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  realtimeSpeedPct = constrain((int)motionStore.getUChar("realtime", 35), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  ikSpeedPct       = constrain((int)motionStore.getUChar("ik", 60), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
}

void saveMotionProfiles() {
  motionStore.putUChar("manual", manualFkSpeedPct);
  motionStore.putUChar("realtime", realtimeSpeedPct);
  motionStore.putUChar("ik", ikSpeedPct);
}

// =====================================================
// Tool Center Point storage
// =====================================================

void loadToolSettings() {
  toolStore.begin("scara_tool", false);
  toolTcpLengthMm = toolStore.getFloat("l2tcp", 120.0f);
  toolTcpLengthMm = constrain(toolTcpLengthMm, 20.0f, 250.0f);
}

void saveToolSettings() {
  toolStore.putFloat("l2tcp", toolTcpLengthMm);
}

// =====================================================
// Motion profile application
// =====================================================

uint8_t getMotionProfilePercent(MotionProfileMode profile) {
  if (profile == PROFILE_REALTIME) return realtimeSpeedPct;
  if (profile == PROFILE_IK) return ikSpeedPct;
  if (profile == PROFILE_SETUP) return SETUP_JOG_SPEED_PCT;
  return manualFkSpeedPct;
}

const char* getMotionProfileName(MotionProfileMode profile) {
  if (profile == PROFILE_REALTIME) return "Real-Time";
  if (profile == PROFILE_IK) return "IK";
  if (profile == PROFILE_SETUP) return "Setup Jog";
  return "Manual / FK";
}

void applyMotionProfile(MotionProfileMode profile) {
  uint8_t percent = getMotionProfilePercent(profile);
  float factor = (float)percent / 100.0f;

  arm1.setMaxSpeed(A1_MAX_SPEED * factor);
  arm2.setMaxSpeed(A2_MAX_SPEED * factor);
  zAxis.setMaxSpeed(Z_MAX_SPEED * factor);

  arm1.setAcceleration(fmax(1000.0f, A1_ACCEL * factor));
  arm2.setAcceleration(fmax(800.0f, A2_ACCEL * factor));
  zAxis.setAcceleration(fmax(600.0f, Z_ACCEL * factor));

  activeMotionProfile = profile;
  activeMotionSpeedPct = percent;
}

void applyFixedHomingMotionSettings() {
  // Locked safety values. User motion sliders never change HOME speed.
  arm1.setMaxSpeed(A1_MAX_SPEED);
  arm1.setAcceleration(A1_ACCEL);
  arm2.setMaxSpeed(A2_MAX_SPEED);
  arm2.setAcceleration(A2_ACCEL);
  zAxis.setMaxSpeed(Z_MAX_SPEED);
  zAxis.setAcceleration(Z_ACCEL);
}

// =====================================================
// Raw calibration validation
// =====================================================

bool zeroTargetsAreSafe(String &reason) {
  // These targets are needed to return the machine to geometric zero after HOME.
  // Endpoint validity must not prevent this zero return.
  if (a1ZeroSteps <= A1_MIN_SAFE_RAW_CLEARANCE) {
    reason = "A1 geometric zero raw position is unsafe. Setup repair required.";
    return false;
  }

  if (a2ZeroSteps >= -A2_MIN_SAFE_RAW_CLEARANCE) {
    reason = "A2 geometric zero raw position is unsafe. Setup repair required.";
    return false;
  }

  if (zZeroSteps <= Z_MIN_SAFE_RAW_CLEARANCE) {
    reason = "Z work-zero raw position is unsafe. Setup repair required.";
    return false;
  }

  reason = "";
  return true;
}

bool a2ParkingMappingIsSafe(String &reason) {
  // A2 raw zero and gripper-clearance park are the only stored targets required
  // before A1 may safely home.
  if (a2ZeroSteps >= -A2_MIN_SAFE_RAW_CLEARANCE) {
    reason = "A2 geometric zero raw position is unsafe.";
    return false;
  }

  if (a2HomeClearanceParkRawSteps >= -A2_MIN_SAFE_RAW_CLEARANCE ||
      a2HomeClearanceParkRawSteps >= a2ZeroSteps) {
    reason = "A2 HOME-clearance park raw position is unsafe or not on the positive clearance side.";
    return false;
  }

  reason = "";
  return true;
}

bool operatingRawLimitsAreSafe(String &reason) {
  // A1 safe raw travel: switch is at 0; operating motion remains on positive side.
  if (a1NegativeLimitRawSteps < A1_MIN_SAFE_RAW_CLEARANCE ||
      a1PositiveLimitRawSteps <= a1NegativeLimitRawSteps ||
      a1ZeroSteps < a1NegativeLimitRawSteps ||
      a1ZeroSteps > a1PositiveLimitRawSteps) {
    reason = "A1 raw endpoint calibration invalid. Set A1 negative and positive endpoints in Setup.";
    return false;
  }

  // A2 safe raw travel: switch is at 0; operating motion remains on negative side.
  // Positive A2 direction is more negative raw travel.
  if (a2NegativeLimitRawSteps > -A2_MIN_SAFE_RAW_CLEARANCE ||
      a2PositiveLimitRawSteps >= a2NegativeLimitRawSteps ||
      a2ZeroSteps > a2NegativeLimitRawSteps ||
      a2ZeroSteps < a2PositiveLimitRawSteps) {
    reason = "A2 raw endpoint calibration invalid. Set A2 negative and positive endpoints in Setup.";
    return false;
  }

  if (a2HomeClearanceParkRawSteps < a2PositiveLimitRawSteps ||
      a2HomeClearanceParkRawSteps > a2NegativeLimitRawSteps ||
      a2HomeClearanceParkRawSteps >= a2ZeroSteps) {
    reason = "A2 HOME-clearance park is outside the saved safe endpoints.";
    return false;
  }

  // Z HOME is at raw 0. Upper limit remains below HOME; lower limit protects
  // the gripper from reaching the table or ground.
  if (zUpperLimitRawSteps < Z_MIN_SAFE_RAW_CLEARANCE ||
      zLowerLimitRawSteps <= zUpperLimitRawSteps ||
      zZeroSteps < zUpperLimitRawSteps ||
      zZeroSteps > zLowerLimitRawSteps) {
    reason = "Z raw endpoint calibration invalid. Set Z upper and ground/lower endpoints in Setup.";
    return false;
  }

  reason = "";
  return true;
}

// =====================================================
// Controlled setup jogging
// =====================================================

bool queueCalibrationJog(const String &axis, float amountToMove, String &error) {
  if (!isHomed && !setupRequired) {
    error = "HOME the robot before setup jog.";
    return false;
  }
  if (isHoming || anyMotion()) {
    error = "Wait until the robot is idle.";
    return false;
  }
  if (faultActive) {
    error = "Fault active. HOME is required.";
    return false;
  }
  if (setupRequired && !setupMovementAllowedForAxis(axis)) {
    error = "This axis has not been safely referenced yet.";
    return false;
  }

  if (axis == "z") {
    amountToMove = constrain(amountToMove, -20.0f, 20.0f);  // millimeters
  } else {
    amountToMove = constrain(amountToMove, -10.0f, 10.0f);  // degrees
  }

  if (fabs(amountToMove) < 0.01f) {
    error = "Jog distance is zero.";
    return false;
  }

  applyMotionProfile(PROFILE_SETUP);
  enableMotors();

  if (axis == "a1") {
    arm1.move(lround(amountToMove * A1_STEPS_PER_DEG));
  } else if (axis == "a2") {
    arm2.move(lround(-amountToMove * A2_STEPS_PER_DEG));
  } else if (axis == "z") {
    // Existing convention: Z positive moves downward; Z negative moves upward.
    zAxis.move(lround(amountToMove * Z_STEPS_PER_MM));
  } else {
    error = "Unknown calibration axis.";
    return false;
  }

  motionStartTime = millis();
  error = "Setup jog accepted.";
  return true;
}
