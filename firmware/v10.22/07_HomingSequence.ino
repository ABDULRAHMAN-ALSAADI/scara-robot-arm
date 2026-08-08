/*
  Tab: 07_HomingSequence.ino
  Purpose: Performs collision-safe HOME: Z, then A2 clearance parking, then A1, then return to calibrated zero.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Limit switches and homing
// =====================================================

bool switchPressed(int pin) {
  if (digitalRead(pin) != LOW) return false;
  delayMicroseconds(400);
  return digitalRead(pin) == LOW;
}

bool runSingleBlocking(AccelStepper &motor, const char* name, unsigned long timeoutMS) {
  unsigned long start = millis();

  while (motor.distanceToGo() != 0) {
    motor.run();

    if (millis() - start > timeoutMS) {
      Serial.print(name);
      Serial.println(" move timeout.");
      emergencyStop(String(name) + " move timeout.");
      return false;
    }
  }

  return true;
}

bool seekSwitch(
  AccelStepper &motor,
  int limitPin,
  const char* name,
  float towardSwitchSpeed,
  long maxTravelSteps
) {
  unsigned long startTime = millis();
  long startPosition = motor.currentPosition();

  motor.setSpeed(towardSwitchSpeed);

  while (!switchPressed(limitPin)) {
    motor.runSpeed();

    long travelled = labs(motor.currentPosition() - startPosition);
    if (travelled > maxTravelSteps) {
      emergencyStop(String(name) + " homing max travel exceeded.");
      return false;
    }

    if (millis() - startTime > HOME_TIMEOUT_MS) {
      emergencyStop(String(name) + " homing timeout.");
      return false;
    }
  }

  motor.setCurrentPosition(0);
  return true;
}

bool homeZ() {
  Serial.println("Homing Z...");
  enableMotors();

  if (!seekSwitch(zAxis, Z_LIMIT, "Z", -Z_HOME_SPEED, Z_HOME_MAX_TRAVEL_STEPS)) {
    return false;
  }

  // Raw release: independent of any user coordinate or saved calibration.
  zAxis.moveTo(Z_HOME_RELEASE_STEPS);
  if (!runSingleBlocking(zAxis, "Z raw release", HOME_TIMEOUT_MS)) return false;

  if (switchPressed(Z_LIMIT)) {
    emergencyStop("Z switch did not release after raw HOME backoff.");
    return false;
  }

  zReferenced = true;
  Serial.println("Z switch released safely. Raw Z reference established.");
  return true;
}

bool homeA1() {
  Serial.println("Homing A1...");
  enableMotors();

  if (!seekSwitch(arm1, A1_LIMIT, "A1", -A1_HOME_SPEED, A1_HOME_MAX_TRAVEL_STEPS)) {
    return false;
  }

  // CRITICAL: after switch touch, release only in verified raw away direction.
  // Do not use a calibrated angle here. A bad/moved calibration must never drive
  // the joint harder into its switch during HOME.
  arm1.moveTo(A1_HOME_RELEASE_STEPS);
  if (!runSingleBlocking(arm1, "A1 raw release", HOME_TIMEOUT_MS)) return false;

  if (switchPressed(A1_LIMIT)) {
    emergencyStop("A1 switch did not release after raw HOME backoff.");
    return false;
  }

  a1Referenced = true;
  Serial.println("A1 switch released safely. Raw A1 reference established.");
  return true;
}

bool homeA2() {
  Serial.println("Homing A2...");
  enableMotors();

  // Your A2 physical switch is the negative-angle side.
  // Raw positive movement goes toward that switch.
  if (!seekSwitch(arm2, A2_LIMIT, "A2", A2_HOME_SPEED, A2_HOME_MAX_TRAVEL_STEPS)) {
    return false;
  }

  // Raw release in verified away direction. This is independent of saved limits.
  arm2.moveTo(-A2_HOME_RELEASE_STEPS);
  if (!runSingleBlocking(arm2, "A2 raw release", HOME_TIMEOUT_MS)) return false;

  if (switchPressed(A2_LIMIT)) {
    emergencyStop("A2 switch did not release after raw HOME backoff.");
    return false;
  }

  a2Referenced = true;
  Serial.println("A2 switch released safely. Raw A2 reference established.");
  return true;
}

bool homeAll() {
  Serial.println("Starting HOME.");

  if (!startupPoseConfirmed) {
    Serial.println("HOME blocked: physically place the arm in the saved Safe Startup Pose, then confirm it on the webpage.");
    faultMessage = "Confirm Safe Startup Pose before HOME.";
    return false;
  }

  // Confirmation is one-shot. Any future HOME requires a new visual check or
  // a controlled PARK FOR POWER-OFF move followed by confirmation.
  startupPoseConfirmed = false;

  // Open the gripper before any axis moves so the added fingers have the
  // maximum available clearance during the collision-safe HOME path.
  String gripperHomeError;
  Serial.println("Opening gripper before HOME movement.");
  if (!commandGripperOpen(gripperHomeError)) {
    emergencyStop("Cannot open gripper for HOME: " + gripperHomeError);
    return false;
  }
  delay(GRIPPER_OPEN_BEFORE_HOME_MS);

  // Restore locked homing/release movement settings regardless of user profiles.
  applyFixedHomingMotionSettings();

  faultActive = false;
  faultMessage = "";
  isHomed = false;
  setupRequired = false;
  zReferenced = false;
  a1Referenced = false;
  a2Referenced = false;
  isHoming = true;

  enableMotors();

  // Stage 1: Z homes first, matching the required vertical safety sequence.
  if (!homeZ()) return false;
  delay(200);

  // Stage 2: Home A2 before A1 because the fitted gripper can collide with the
  // robot base while A1 rotates if A2 is straight or on the wrong side.
  if (!homeA2()) return false;
  delay(200);

  String a2SafetyFault;
  if (!a2ParkingMappingIsSafe(a2SafetyFault)) {
    enterSetupRequired(a2SafetyFault + " A2 is referenced; repair A2 zero/limits/clearance pose in Setup.");
    return false;
  }

  Serial.println("A2 -> calibrated raw zero before clearance parking.");
  arm2.moveTo(a2ZeroSteps);
  if (!runSingleBlocking(arm2, "A2 move to raw zero before A1 HOME", MOVE_TIMEOUT_MS)) return false;

  Serial.print("A2 -> A1-HOME clearance park pose: ");
  Serial.print(a2HomeClearanceParkDeg, 2);
  Serial.println(" deg (stored raw machine target).");
  arm2.moveTo(a2HomeClearanceParkRawSteps);
  if (!runSingleBlocking(arm2, "A2 HOME clearance park", MOVE_TIMEOUT_MS)) return false;

  if (switchPressed(A2_LIMIT)) {
    emergencyStop("A2 switch touched while entering A1-HOME clearance pose.");
    return false;
  }

  // Stage 3: With the gripper parked clear, A1 may safely home.
  if (!homeA1()) return false;
  delay(200);

  String mappingFault;
  if (!zeroTargetsAreSafe(mappingFault)) {
    enterSetupRequired(mappingFault + " Axes are referenced; repair geometric zero in Setup.");
    return false;
  }

  Serial.println("A1 -> calibrated raw zero while A2 remains parked.");
  arm1.moveTo(a1ZeroSteps);
  if (!runSingleBlocking(arm1, "A1 move to raw zero", MOVE_TIMEOUT_MS)) return false;

  Serial.println("A2 returns from clearance park to calibrated raw zero.");
  arm2.moveTo(a2ZeroSteps);
  if (!runSingleBlocking(arm2, "A2 return to raw zero", MOVE_TIMEOUT_MS)) return false;

  // Requested HOME finish behavior:
  // The old v10.22 HOME sequence moved Z down to calibrated work zero here.
  // That is removed. Z stays at the HOME release/top-clearance position.
  // After the arm is fully stretched to calibrated zero, move the two rotary
  // axes to the requested ready pose: A1 +130 deg, A2 -136 deg.
  if (HOME_FINISH_KEEP_Z_AT_TOP) {
    Serial.println("Z stays at HOME top-release position. No move down to calibrated Z=0.");
  } else {
    Serial.println("Z -> calibrated raw work zero.");
    zAxis.moveTo(zZeroSteps);
    if (!runSingleBlocking(zAxis, "Z move to zero", MOVE_TIMEOUT_MS)) return false;
  }

  if (HOME_FINISH_A1_DEG < a1MinDeg || HOME_FINISH_A1_DEG > a1MaxDeg) {
    enterSetupRequired("HOME finish A1 target is outside calibrated A1 limits.");
    return false;
  }
  if (HOME_FINISH_A2_DEG < a2MinDeg || HOME_FINISH_A2_DEG > a2MaxDeg) {
    enterSetupRequired("HOME finish A2 target is outside calibrated A2 limits.");
    return false;
  }

  Serial.print("A1 -> HOME finish pose: ");
  Serial.print(HOME_FINISH_A1_DEG, 2);
  Serial.println(" deg.");
  arm1.moveTo(a1AngleToSteps(HOME_FINISH_A1_DEG));
  if (!runSingleBlocking(arm1, "A1 HOME finish pose", MOVE_TIMEOUT_MS)) return false;

  Serial.print("A2 -> HOME finish pose: ");
  Serial.print(HOME_FINISH_A2_DEG, 2);
  Serial.println(" deg.");
  arm2.moveTo(a2AngleToSteps(HOME_FINISH_A2_DEG));
  if (!runSingleBlocking(arm2, "A2 HOME finish pose", MOVE_TIMEOUT_MS)) return false;

  if (switchPressed(Z_LIMIT)) {
    emergencyStop("Z switch is still pressed at HOME finish top-release pose.");
    return false;
  }
  if (switchPressed(A1_LIMIT)) {
    emergencyStop("A1 switch touched at HOME finish pose.");
    return false;
  }
  if (switchPressed(A2_LIMIT)) {
    emergencyStop("A2 switch touched at HOME finish pose.");
    return false;
  }

  // Physical endpoints are operation limits, not HOME-return targets.
  // If they need repair, the machine is already referenced and parked at the
  // requested HOME finish pose.
  String endpointFault;
  if (!operatingRawLimitsAreSafe(endpointFault)) {
    enterSetupRequired(endpointFault + " Robot is referenced at HOME finish pose; repair endpoints in Setup, then HOME again.");
    return false;
  }

  isHoming = false;
  isHomed = true;
  setupRequired = false;
  lastMotionActivityTime = millis();

  // Prepare normal operation with the saved Manual/FK profile.
  applyMotionProfile(PROFILE_MANUAL_FK);

  Serial.print("HOME complete at finish pose: A1="); Serial.print(HOME_FINISH_A1_DEG, 2);
  Serial.print(" deg, A2="); Serial.print(HOME_FINISH_A2_DEG, 2);
  Serial.println(" deg, Z kept near top HOME switch.");
  Serial.println("Motors will release after idle timeout.");
  Serial.println("Do not move the arm by hand while motors are released; HOME again if moved.");
  Serial.print("A1 safe range: "); Serial.print(a1MinDeg, 2); Serial.print(" to "); Serial.println(a1MaxDeg, 2);
  Serial.print("A2 safe range: "); Serial.print(a2MinDeg, 2); Serial.print(" to "); Serial.println(a2MaxDeg, 2);
  Serial.print("A2 A1-HOME clearance park pose: "); Serial.print(a2HomeClearanceParkDeg, 2); Serial.println(" deg");
  Serial.print("Tool Center Point L2: "); Serial.print(toolTcpLengthMm, 2); Serial.println(" mm");
  Serial.print("Z safe range: "); Serial.print(zMinMm, 2); Serial.print(" to "); Serial.print(zMaxMm, 2); Serial.println(" mm");
  resetRecipeAfterHome();
  armJobOnHomeComplete();
  return true;
}
