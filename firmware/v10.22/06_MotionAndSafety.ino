/*
  Tab: 06_MotionAndSafety.ino
  Purpose: Queues normal moves protected by the interpolated collision envelope
           and routes permitted targets through saved A2 corridor when direct motion is blocked.
*/

bool queueDirectJointMove(float a1Deg, float a2Deg, float zMM,
                          MotionProfileMode profile, String &message) {
  applyMotionProfile(profile);
  enableMotors();
  resetMoveVelocityMonitor();

  arm1.moveTo(a1AngleToSteps(a1Deg));
  arm2.moveTo(a2AngleToSteps(a2Deg));
  zAxis.moveTo(zMmToSteps(zMM));

  motionStartTime = millis();
  message = "Direct target accepted.";
  return true;
}

const char* collisionRoutePhaseName() {
  if (!collisionSafeRouteActive) return "Direct / idle";
  if (collisionSafeRoutePhase == COLLISION_ROUTE_CLEAR_A2_TO_ZERO) return "Safe route: A2 -> saved corridor";
  if (collisionSafeRoutePhase == COLLISION_ROUTE_MOVE_A1_AT_ZERO) return "Safe route: move A1 at saved corridor";
  if (collisionSafeRoutePhase == COLLISION_ROUTE_FINAL_TARGET) return "Safe route: final A2/Z target";
  return "Safe route";
}

bool startCollisionSafeRoute(float a1Deg, float a2Deg, float zMM,
                             MotionProfileMode profile, String &message) {
  const float startA1 = actualA1();
  const float startA2 = actualA2();
  const float startZ = actualZ();
  String stageError;

  if (!collisionPathIsSafe(startA1, startA2, startZ,
                           startA1, collisionSafeCorridorA2Deg, startZ,
                           gripperHoldDeg, stageError)) {
    message = "Cannot route safely to saved A2 corridor. " + stageError;
    return false;
  }
  if (!collisionPathIsSafe(startA1, collisionSafeCorridorA2Deg, startZ,
                           a1Deg, collisionSafeCorridorA2Deg, startZ,
                           gripperHoldDeg, stageError)) {
    message = "Cannot route A1 safely at saved A2 corridor. " + stageError;
    return false;
  }
  if (!collisionPathIsSafe(a1Deg, collisionSafeCorridorA2Deg, startZ,
                           a1Deg, a2Deg, zMM,
                           gripperHoldDeg, stageError)) {
    message = "Cannot complete routed final movement. " + stageError;
    return false;
  }

  collisionRouteTargetA1Deg = a1Deg;
  collisionRouteTargetA2Deg = a2Deg;
  collisionRouteTargetZMm = zMM;
  collisionRouteTransitZMm = startZ;
  collisionRouteProfile = profile;
  collisionSafeRouteActive = true;

  if (fabs(startA2 - collisionSafeCorridorA2Deg) > 0.05f) {
    collisionSafeRoutePhase = COLLISION_ROUTE_CLEAR_A2_TO_ZERO;
    queueDirectJointMove(startA1, collisionSafeCorridorA2Deg, startZ, profile, stageError);
  } else if (fabs(startA1 - a1Deg) > 0.05f) {
    collisionSafeRoutePhase = COLLISION_ROUTE_MOVE_A1_AT_ZERO;
    queueDirectJointMove(a1Deg, collisionSafeCorridorA2Deg, startZ, profile, stageError);
  } else {
    collisionSafeRoutePhase = COLLISION_ROUTE_FINAL_TARGET;
    queueDirectJointMove(a1Deg, a2Deg, zMM, profile, stageError);
  }

  lastMotionPlan = "AUTO SAFE ROUTE via A2 corridor " + String(collisionSafeCorridorA2Deg, 2) + " deg.";
  message = "Target accepted with AUTO SAFE ROUTE via A2 corridor " + String(collisionSafeCorridorA2Deg, 2) + " deg.";
  return true;
}

bool queueJointMove(float a1Deg, float a2Deg, float zMM,
                    MotionProfileMode profile, String &message) {
  if (recipeRunning && !recipeInternalMotionCommand) {
    message = "A recipe is playing. Press STOP before manual motion.";
    return false;
  }
  if (armJobLocksManualControl() && !recipeInternalMotionCommand) {
    message = "An autonomous job is loaded or active. Abort or finish the job before manual motion.";
    return false;
  }
  if (!isHomed) {
    message = setupRequired ? "Setup required: repair calibration, then HOME again." : "Home the robot first.";
    return false;
  }
  if (isHoming) { message = "Robot is homing."; return false; }
  if (faultActive) { message = "Fault active. Press HOME."; return false; }
  if (collisionSafeRouteActive) { message = "AUTO SAFE ROUTE is already moving. Wait until it completes."; return false; }
  const float allowedA1Min = recipeInternalMotionCommand ? recipeEffectiveA1MinDeg() : a1MinDeg;
  const float allowedA1Max = recipeInternalMotionCommand ? recipeEffectiveA1MaxDeg() : a1MaxDeg;
  const float allowedA2Min = recipeInternalMotionCommand ? recipeEffectiveA2MinDeg() : a2MinDeg;
  const float allowedA2Max = recipeInternalMotionCommand ? recipeEffectiveA2MaxDeg() : a2MaxDeg;
  const float allowedZMin  = recipeInternalMotionCommand ? recipeEffectiveZMinMm()   : zMinMm;
  const float allowedZMax  = recipeInternalMotionCommand ? recipeEffectiveZMaxMm()   : zMaxMm;
  if (a1Deg < allowedA1Min || a1Deg > allowedA1Max) { message = "A1 outside safe range."; return false; }
  if (a2Deg < allowedA2Min || a2Deg > allowedA2Max) { message = "A2 outside safe range."; return false; }
  if (zMM < allowedZMin || zMM > allowedZMax) { message = "Z outside calibrated safe range."; return false; }

  String targetError;
  if (!collisionPoseIsSafe(a1Deg, a2Deg, zMM, gripperHoldDeg, targetError)) {
    message = "Target is forbidden. " + targetError;
    return false;
  }

  String directPathError;
  if (collisionPathIsSafe(actualA1(), actualA2(), actualZ(),
                          a1Deg, a2Deg, zMM, gripperHoldDeg, directPathError)) {
    collisionSafeRouteActive = false;
    collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
    lastMotionPlan = "Direct safe movement.";
    return queueDirectJointMove(a1Deg, a2Deg, zMM, profile, message);
  }

  return startCollisionSafeRoute(a1Deg, a2Deg, zMM, profile, message);
}

bool anyMotion() {
  return arm1.distanceToGo() != 0 || arm2.distanceToGo() != 0 || zAxis.distanceToGo() != 0;
}

void serviceCollisionSafeRoute() {
  if (!collisionSafeRouteActive || anyMotion() || faultActive || isHoming) return;
  String ignoredMessage;

  if (collisionSafeRoutePhase == COLLISION_ROUTE_CLEAR_A2_TO_ZERO) {
    collisionSafeRoutePhase = COLLISION_ROUTE_MOVE_A1_AT_ZERO;
    queueDirectJointMove(collisionRouteTargetA1Deg, collisionSafeCorridorA2Deg,
                         collisionRouteTransitZMm, collisionRouteProfile, ignoredMessage);
    return;
  }
  if (collisionSafeRoutePhase == COLLISION_ROUTE_MOVE_A1_AT_ZERO) {
    collisionSafeRoutePhase = COLLISION_ROUTE_FINAL_TARGET;
    queueDirectJointMove(collisionRouteTargetA1Deg, collisionRouteTargetA2Deg,
                         collisionRouteTargetZMm, collisionRouteProfile, ignoredMessage);
    return;
  }
  if (collisionSafeRoutePhase == COLLISION_ROUTE_FINAL_TARGET) {
    collisionSafeRouteActive = false;
    collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
    lastMotionPlan = "AUTO SAFE ROUTE complete.";
  }
}

void checkLimitSafety() {
  if ((!isHomed && !setupRequired) || isHoming || faultActive || !anyMotion()) return;
  if (switchPressed(A1_LIMIT)) { emergencyStop("A1 limit touched outside HOME."); return; }
  if (switchPressed(A2_LIMIT)) { emergencyStop("A2 limit touched outside HOME."); return; }
  if (switchPressed(Z_LIMIT)) { emergencyStop("Z limit touched outside HOME."); return; }
}

void resetMoveVelocityMonitor() {
  movePeakA1DegS = 0.0f;
  movePeakA2DegS = 0.0f;
  movePeakZMmS = 0.0f;
}

void updateMoveVelocityMonitor() {
  float a1DegS = fabs(arm1.speed()) / A1_STEPS_PER_DEG;
  float a2DegS = fabs(arm2.speed()) / A2_STEPS_PER_DEG;
  float zMmS   = fabs(zAxis.speed()) / Z_STEPS_PER_MM;
  if (a1DegS > movePeakA1DegS) movePeakA1DegS = a1DegS;
  if (a2DegS > movePeakA2DegS) movePeakA2DegS = a2DegS;
  if (zMmS > movePeakZMmS) movePeakZMmS = zMmS;
}

void runMotionTask() {
  if (!anyMotion() && collisionSafeRouteActive) serviceCollisionSafeRoute();

  if (anyMotion()) {
    checkLimitSafety();
    if (faultActive) return;
    arm1.run();
    arm2.run();
    zAxis.run();
    updateMoveVelocityMonitor();
    lastMotionActivityTime = millis();
    if (millis() - motionStartTime > MOVE_TIMEOUT_MS) emergencyStop("Movement timeout.");
    return;
  }

  if (motorsEnabled && !isHoming && millis() - lastMotionActivityTime >= MOTOR_IDLE_OFF_MS) disableMotors();
}

void runMotionServiceSlice() {
  if (isHoming || !anyMotion()) {
    runMotionTask();
    return;
  }
  const unsigned long sliceUs =
    (activeMotionProfile == PROFILE_REALTIME) ? REALTIME_MOTION_SLICE_US : POINT_TO_POINT_MOTION_SLICE_US;
  const unsigned long startedUs = micros();
  do {
    runMotionTask();
    if (faultActive || isHoming || !anyMotion()) break;
  } while ((unsigned long)(micros() - startedUs) < sliceUs);
}
