/*
  Tab: 01_DriverControl.ino
  Purpose: Controls DRV8825 enable/disable and the global emergency-stop response.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Driver control
// =====================================================

void enableMotors() {
  digitalWrite(A1_EN, LOW);
  digitalWrite(A2_EN, LOW);
  digitalWrite(Z_EN, LOW);
  motorsEnabled = true;
  lastMotionActivityTime = millis();
}

void disableMotors() {
  digitalWrite(A1_EN, HIGH);
  digitalWrite(A2_EN, HIGH);
  digitalWrite(Z_EN, HIGH);
  motorsEnabled = false;
}

void emergencyStop(const String &reason) {
  stopPackageRecipe(reason);

  arm1.moveTo(arm1.currentPosition());
  arm2.moveTo(arm2.currentPosition());
  zAxis.moveTo(zAxis.currentPosition());

  disableMotors();

  isHomed = false;
  isHoming = false;
  setupRequired = false;
  startupPoseConfirmed = false;
  collisionSafeRouteActive = false;
  collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
  lastMotionPlan = "Stopped";
  zReferenced = false;
  a1Referenced = false;
  a2Referenced = false;

  faultActive = true;
  faultMessage = reason;
  armJobOnEmergencyStop(reason);

  Serial.print("STOP: ");
  Serial.println(reason);
  Serial.println("HOME is required before any new movement.");
}

void enterSetupRequired(const String &reason) {
  // Not an emergency collision: the switches have already been safely released.
  // Hold normal robot operation blocked, but permit controlled Setup jogging and
  // calibration saving while the raw HOME references remain valid.
  isHomed = false;
  isHoming = false;
  setupRequired = true;
  collisionSafeRouteActive = false;
  collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
  faultActive = false;
  faultMessage = reason;
  lastMotionActivityTime = millis();
  applyMotionProfile(PROFILE_SETUP);

  Serial.println("REFERENCED SETUP REQUIRED:");
  Serial.println(reason);
  Serial.println("Normal FK/IK movement is blocked. Use Setup to repair calibration, then press HOME again.");
}

bool setupMovementAllowedForAxis(const String &axis) {
  if (axis == "a1") return a1Referenced;
  if (axis == "a2") return a2Referenced;
  if (axis == "z")  return zReferenced;
  return false;
}
