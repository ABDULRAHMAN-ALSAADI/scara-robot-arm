/*
  Tab: 08_WebApi.ino
  Purpose: Connects the webpage buttons and sliders to firmware actions through HTTP endpoints.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Web handlers
// =====================================================

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNetworkStatus() {
  String json = "{\"ok\":true";
  json += ",\"apSsid\":\"" + String(AP_SSID) + "\"";
  json += ",\"apIp\":\"" + WiFi.softAPIP().toString() + "\"";
  json += ",\"staSsid\":\"" + String(STA_SSID) + "\"";
  json += ",\"staConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  json += ",\"staIp\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"wifiStatus\":" + String((int)WiFi.status());
  json += "}";
  server.send(200, "application/json", json);
}

void handleHome() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Abort the loaded job before HOME.\"}");
    return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"A recipe is playing. Press STOP before HOME.\"}");
    return;
  }
  if (isHoming || homeRequested) {
    server.send(202, "application/json", "{\"ok\":true,\"started\":true,\"msg\":\"Homing already in progress\"}");
    return;
  }

  if (!startupPoseConfirmed) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"HOME blocked. Put the arm in the Safe Startup Pose and press Confirm Safe Startup Pose first.\"}");
    return;
  }

  collisionSafeRouteActive = false;
  collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
  homeRequested = true;
  homeRequestedAt = millis();
  faultActive = false;
  faultMessage = "";

  // Reply immediately. The physical HOME sequence starts from loop(),
  // so the browser request does not falsely time out while motors are moving.
  server.send(202, "application/json", "{\"ok\":true,\"started\":true,\"msg\":\"Homing started\"}");
}

void handleMove() {
  if (!server.hasArg("a1") || !server.hasArg("a2") || !server.hasArg("z")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing parameters\"}");
    return;
  }

  float a1 = server.arg("a1").toFloat();
  float a2 = server.arg("a2").toFloat();
  float z = server.arg("z").toFloat();

  String error;
  if (!queueJointMove(a1, a2, z, PROFILE_MANUAL_FK, error)) {
    server.send(409, "application/json",
      "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  Position target = forwardKinematics(a1, a2, z);

  String json = "{\"ok\":true,\"msg\":\"" + error + "\"";
  json += ",\"x\":" + String(target.x, 2);
  json += ",\"y\":" + String(target.y, 2);
  json += ",\"z\":" + String(target.z, 2);
  json += "}";

  server.send(200, "application/json", json);
}

void handleMoveRT() {
  if (!server.hasArg("a1") || !server.hasArg("a2") || !server.hasArg("z")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing parameters\"}");
    return;
  }

  float a1 = server.arg("a1").toFloat();
  float a2 = server.arg("a2").toFloat();
  float z = server.arg("z").toFloat();

  String error;
  if (!queueJointMove(a1, a2, z, PROFILE_REALTIME, error)) {
    server.send(409, "application/json",
      "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleIK() {
  if (!server.hasArg("x") || !server.hasArg("y") || !server.hasArg("z")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing parameters\"}");
    return;
  }

  if (!isHomed) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Home the robot first\"}");
    return;
  }

  float x = server.arg("x").toFloat();
  float y = server.arg("y").toFloat();
  float z = server.arg("z").toFloat();

  String configuration = server.hasArg("config") ? server.arg("config") : "nearest";
  IKSolution solution;

  if (configuration == "elbow_up") {
    solution = solveIK(x, y, false);
  } else if (configuration == "elbow_down") {
    solution = solveIK(x, y, true);
  } else {
    solution = selectNearestIKConfiguration(x, y);
  }

  if (!solution.ok) {
    server.send(200, "application/json",
      "{\"ok\":false,\"err\":\"" + solution.error + "\"}");
    return;
  }

  String selectedConfiguration = solution.a2 < 0.0f ? "Elbow-Up Configuration" : "Elbow-Down Configuration";

  String error;
  if (!queueJointMove(solution.a1, solution.a2, z, PROFILE_IK, error)) {
    server.send(409, "application/json",
      "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  String json = "{\"ok\":true";
  json += ",\"msg\":\"" + error + "\"";
  json += ",\"a1\":" + String(solution.a1, 2);
  json += ",\"a2\":" + String(solution.a2, 2);
  json += ",\"z\":" + String(z, 2);
  json += ",\"configuration\":\"" + selectedConfiguration + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handlePosition() {
  float a1 = actualA1();
  float a2 = actualA2();
  float z = actualZ();
  Position position = forwardKinematics(a1, a2, z);

  String json = "{";
  json += "\"ok\":true";
  json += ",\"a1\":" + String(a1, 2);
  json += ",\"a2\":" + String(a2, 2);
  json += ",\"z\":" + String(z, 2);
  json += ",\"x\":" + String(position.x, 4);
  json += ",\"y\":" + String(position.y, 4);
  json += ",\"homed\":" + String(isHomed ? "true" : "false");
  json += ",\"homing\":" + String((isHoming || homeRequested) ? "true" : "false");
  json += ",\"setupRequired\":" + String(setupRequired ? "true" : "false");
  json += ",\"zReferenced\":" + String(zReferenced ? "true" : "false");
  json += ",\"a1Referenced\":" + String(a1Referenced ? "true" : "false");
  json += ",\"a2Referenced\":" + String(a2Referenced ? "true" : "false");
  json += ",\"moving\":" + String(anyMotion() ? "true" : "false");
  json += ",\"motors\":" + String(motorsEnabled ? "true" : "false");
  json += ",\"fault\":" + String(faultActive ? "true" : "false");
  json += ",\"faultMessage\":\"" + faultMessage + "\"";
  json += ",\"a1Min\":" + String(a1MinDeg, 3);
  json += ",\"a1Max\":" + String(a1MaxDeg, 3);
  json += ",\"a2Min\":" + String(a2MinDeg, 3);
  json += ",\"a2Max\":" + String(a2MaxDeg, 3);
  json += ",\"zMin\":" + String(zMinMm, 3);
  json += ",\"zMax\":" + String(zMaxMm, 3);
  json += ",\"activeProfile\":\"" + String(getMotionProfileName(activeMotionProfile)) + "\"";
  json += ",\"activeSpeedPct\":" + String(activeMotionSpeedPct);
  json += ",\"manualSpeedPct\":" + String(manualFkSpeedPct);
  json += ",\"realtimeSpeedPct\":" + String(realtimeSpeedPct);
  json += ",\"ikSpeedPct\":" + String(ikSpeedPct);

  bool currentlyMoving = anyMotion();
  float a1Velocity = currentlyMoving ? fabs(arm1.speed()) / A1_STEPS_PER_DEG : 0.0f;
  float a2Velocity = currentlyMoving ? fabs(arm2.speed()) / A2_STEPS_PER_DEG : 0.0f;
  float zVelocity  = currentlyMoving ? fabs(zAxis.speed()) / Z_STEPS_PER_MM : 0.0f;

  json += ",\"a1VelocityDegS\":" + String(a1Velocity, 2);
  json += ",\"a2VelocityDegS\":" + String(a2Velocity, 2);
  json += ",\"zVelocityMmS\":" + String(zVelocity, 2);
  json += ",\"a1PeakDegS\":" + String(movePeakA1DegS, 2);
  json += ",\"a2PeakDegS\":" + String(movePeakA2DegS, 2);
  json += ",\"zPeakMmS\":" + String(movePeakZMmS, 2);
  json += ",\"stepPulseUs\":" + String(DRV8825_MIN_STEP_PULSE_US);
  json += ",\"toolTcpLengthMm\":" + String(toolTcpLengthMm, 3);
  json += ",\"a2HomeParkDeg\":" + String(a2HomeClearanceParkDeg, 3);
  json += ",\"gripperAttached\":" + String(gripperAttached ? "true" : "false");
  json += ",\"gripperCommandDeg\":" + String(gripperCommandDeg);
  json += ",\"gripperOpenDeg\":" + String(gripperOpenDeg);
  json += ",\"gripperHoldDeg\":" + String(gripperHoldDeg);
  json += ",\"gripperMaxCloseDeg\":" + String(gripperMaxCloseDeg);
  json += ",\"startupPoseConfirmed\":" + String(startupPoseConfirmed ? "true" : "false");
  json += ",\"startupParkSaved\":" + String(startupParkSaved ? "true" : "false");
  json += ",\"collisionEnabled\":" + String(collisionProtectionEnabled ? "true" : "false");
  json += ",\"collisionCorridorA2\":" + String(collisionSafeCorridorA2Deg, 3);
  json += ",\"activeEnvelopeLimits\":[";
  bool firstActiveLimit = true;
  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    float liveLimit;
    if (!envelopeLimitAtPose(layerIndex, a1, z, liveLimit)) continue;
    if (!firstActiveLimit) json += ",";
    firstActiveLimit = false;
    json += "{\"layer\":" + String(layerIndex);
    json += ",\"region\":\"" + String(layerUsesPositiveA1(envelopeLayers[layerIndex]) ? "positive" : "negative") + "\"";
    json += ",\"bound\":\"" + String(layerUsesMaximumA2(envelopeLayers[layerIndex]) ? "max" : "min") + "\"";
    json += ",\"limit\":" + String(liveLimit, 3) + "}";
  }
  json += "]";
  json += ",\"collisionRouteActive\":" + String(collisionSafeRouteActive ? "true" : "false");
  json += ",\"collisionRoutePhase\":\"" + String(collisionRoutePhaseName()) + "\"";
  json += ",\"motionPlan\":\"" + lastMotionPlan + "\"";
  json += ",\"recipeRunning\":" + String(recipeRunning ? "true" : "false");
  json += ",\"recipeState\":\"" + String(recipeStateName(recipeState)) + "\"";
  json += ",\"recipeActive\":\"" + activePackageRecipeName() + "\"";
  json += ",\"recipeStepIndex\":" + String(recipeRunning ? recipeStepIndex + 1 : 0);
  json += ",\"recipeCycle\":" + String(recipeCycle);
  json += ",\"recipeRepeats\":" + String(recipeRepeatTarget);
  json += ",\"recipeMotion\":\"" + recipeMotionDescription + "\"";
  json += ",\"recipeStatus\":\"" + recipeStatusMessage + "\"";
  json += ",\"recipeLastEvent\":\"" + recipeLastEvent + "\"";
  json += ",\"packageRecipeCount\":" + String(packageRecipeCount);
  json += ",\"jobState\":\"" + String(armJobStateName(armJobState)) + "\"";
  json += ",\"jobId\":\"" + String(activeArmJob.jobId) + "\"";
  json += ",\"jobPackageClass\":\"" + String(activeArmJob.packageClass) + "\"";
  json += ",\"jobRecipe\":\"" + activeArmJobRecipeName() + "\"";
  json += ",\"jobAmrConfirmed\":" + String(activeArmJob.amrConfirmed ? "true" : "false");
  json += ",\"jobResult\":\"" + armJobResult + "\"";
  json += ",\"jobMessage\":\"" + armJobMessage + "\"";
  json += ",\"jobLastEvent\":\"" + armJobLastEvent + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleStop() {
  homeRequested = false;
  emergencyStop("Operator STOP pressed.");
  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Stopped. HOME required.\"}");
}

void handleOff() {
  homeRequested = false;
  stopPackageRecipe("Motors powered off by operator.");
  arm1.moveTo(arm1.currentPosition());
  arm2.moveTo(arm2.currentPosition());
  zAxis.moveTo(zAxis.currentPosition());

  disableMotors();
  isHomed = false;
  setupRequired = false;
  startupPoseConfirmed = false;
  collisionSafeRouteActive = false;
  collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
  lastMotionPlan = "Motors off";
  zReferenced = false;
  a1Referenced = false;
  a2Referenced = false;
  faultActive = true;
  faultMessage = "Motors were powered off. HOME required.";

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Motors off. HOME required.\"}");
}

void handleConfig() {
  String json = "{\"ok\":true";
  json += ",\"a1Min\":" + String(a1MinDeg, 3);
  json += ",\"a1Max\":" + String(a1MaxDeg, 3);
  json += ",\"a2Min\":" + String(a2MinDeg, 3);
  json += ",\"a2Max\":" + String(a2MaxDeg, 3);
  json += ",\"a1ZeroSteps\":" + String(a1ZeroSteps);
  json += ",\"a2ZeroSteps\":" + String(a2ZeroSteps);
  json += ",\"a1NegativeLimitRawSteps\":" + String(a1NegativeLimitRawSteps);
  json += ",\"a1PositiveLimitRawSteps\":" + String(a1PositiveLimitRawSteps);
  json += ",\"a2NegativeLimitRawSteps\":" + String(a2NegativeLimitRawSteps);
  json += ",\"a2PositiveLimitRawSteps\":" + String(a2PositiveLimitRawSteps);
  json += ",\"a2HomeParkRawSteps\":" + String(a2HomeClearanceParkRawSteps);
  json += ",\"zZeroSteps\":" + String(zZeroSteps);
  json += ",\"zUpperLimitRawSteps\":" + String(zUpperLimitRawSteps);
  json += ",\"zLowerLimitRawSteps\":" + String(zLowerLimitRawSteps);
  json += ",\"zMin\":" + String(zMinMm, 3);
  json += ",\"zMax\":" + String(zMaxMm, 3);
  json += ",\"manualSpeedPct\":" + String(manualFkSpeedPct);
  json += ",\"realtimeSpeedPct\":" + String(realtimeSpeedPct);
  json += ",\"ikSpeedPct\":" + String(ikSpeedPct);
  json += ",\"a1HomeReleaseSteps\":" + String(A1_HOME_RELEASE_STEPS);
  json += ",\"a2HomeReleaseSteps\":" + String(A2_HOME_RELEASE_STEPS);
  json += ",\"toolTcpLengthMm\":" + String(toolTcpLengthMm, 3);
  json += ",\"a2HomeParkDeg\":" + String(a2HomeClearanceParkDeg, 3);
  json += ",\"gripperCommandDeg\":" + String(gripperCommandDeg);
  json += ",\"gripperOpenDeg\":" + String(gripperOpenDeg);
  json += ",\"gripperHoldDeg\":" + String(gripperHoldDeg);
  json += ",\"gripperMaxCloseDeg\":" + String(gripperMaxCloseDeg);
  json += ",\"gripperPin\":" + String(GRIPPER_SERVO_PIN);
  json += ",\"startupPoseConfirmed\":" + String(startupPoseConfirmed ? "true" : "false");
  json += ",\"startupParkSaved\":" + String(startupParkSaved ? "true" : "false");
  json += ",\"startupParkA1\":" + String(startupParkSaved ? a1StepsToAngle(startupParkA1RawSteps) : 0.0f, 3);
  json += ",\"startupParkA2\":" + String(startupParkSaved ? a2StepsToAngle(startupParkA2RawSteps) : 0.0f, 3);
  json += ",\"startupParkZ\":" + String(startupParkSaved ? zStepsToMm(startupParkZRawSteps) : 0.0f, 3);
  json += ",\"collisionEnabled\":" + String(collisionProtectionEnabled ? "true" : "false");
  json += ",\"collisionMarginDeg\":" + String(collisionEnvelopeMarginDeg, 3);
  json += ",\"collisionCorridorA2\":" + String(collisionSafeCorridorA2Deg, 3);
  json += ",\"envLayers\":[";
  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    if (layerIndex > 0) json += ",";
    CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    json += "{\"index\":" + String(layerIndex);
    json += ",\"region\":\"" + String(layerUsesPositiveA1(layer) ? "positive" : "negative") + "\"";
    json += ",\"bound\":\"" + String(layerUsesMaximumA2(layer) ? "max" : "min") + "\"";
    json += ",\"zFrom\":" + String(zStepsToMm(layer.zRawMin), 3);
    json += ",\"zTo\":" + String(zStepsToMm(layer.zRawMax), 3);
    json += ",\"zRawMin\":" + String(layer.zRawMin) + ",\"zRawMax\":" + String(layer.zRawMax);
    json += ",\"points\":[";
    for (uint8_t i = 0; i < layer.pointCount; i++) {
      if (i > 0) json += ",";
      json += "{\"a1\":" + String(a1StepsToAngle(layer.points[i].a1RawSteps), 3);
      json += ",\"a2Limit\":" + String(a2StepsToAngle(layer.points[i].a2RawLimitSteps), 3);
      json += ",\"a1Raw\":" + String(layer.points[i].a1RawSteps);
      json += ",\"a2Raw\":" + String(layer.points[i].a2RawLimitSteps) + "}";
    }
    json += "]}";
  }
  json += "],\"packageRecipes\":[";
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    if (r > 0) json += ",";
    json += "{\"index\":" + String(r) + ",\"name\":\"" + String(packageRecipes[r].name) + "\",\"steps\":[";
    for (uint8_t s = 0; s < packageRecipes[r].stepCount; s++) {
      if (s > 0) json += ",";
      RecipeStep &step = packageRecipes[r].steps[s];
      json += "{\"index\":" + String(s);
      json += ",\"name\":\"" + String(step.name) + "\"";
      json += ",\"command\":\"" + String(recipeCommandName(step.command)) + "\"";
      json += ",\"a1\":" + String(a1StepsToAngle(step.a1RawSteps), 3);
      json += ",\"a2\":" + String(a2StepsToAngle(step.a2RawSteps), 3);
      json += ",\"z\":" + String(zStepsToMm(step.zRawSteps), 3);
      json += ",\"gripper\":" + String(step.gripperDeg);
      json += ",\"waitMs\":" + String(step.waitAfterMs) + "}";
    }
    json += "]}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleCalibrationJog() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Setup movement is blocked while a job is loaded.\"}");
    return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Setup movement is blocked during recipe playback.\"}");
    return;
  }
  if (!server.hasArg("axis") || !server.hasArg("deg")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing axis or degree\"}");
    return;
  }

  String error;
  if (!queueCalibrationJog(server.arg("axis"), server.arg("deg").toFloat(), error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Setup jog accepted\"}");
}

void handleCalibrationSetZero() {
  if ((!isHomed && !setupRequired) || isHoming || anyMotion()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Run HOME/reference and wait until idle before setting zero\"}");
    return;
  }

  String axis = server.arg("axis");

  if (axis == "a1") {
    if (!a1Referenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"A1 has not been referenced by HOME yet\"}");
      return;
    }
    a1ZeroSteps = arm1.currentPosition();
  } else if (axis == "a2") {
    if (!a2Referenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 has not been referenced by HOME yet\"}");
      return;
    }
    a2ZeroSteps = arm2.currentPosition();
  } else if (axis == "z") {
    if (!zReferenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Z has not been referenced by HOME yet\"}");
      return;
    }
    zZeroSteps = zAxis.currentPosition();
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown axis\"}");
    return;
  }

  saveCalibration();
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Raw axis zero saved; displayed operating limits recalculated\"}");
}

void handleCalibrationSetLimit() {
  if ((!isHomed && !setupRequired) || isHoming || anyMotion()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Run HOME/reference and wait until idle before saving a limit\"}");
    return;
  }

  String axis = server.arg("axis");
  String side = server.arg("side");

  if (axis == "a1") {
    if (!a1Referenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"A1 has not been referenced by HOME yet\"}");
      return;
    }
    long raw = arm1.currentPosition();

    if (side == "min") {
      if (raw < A1_MIN_SAFE_RAW_CLEARANCE || raw >= a1PositiveLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"A1 negative endpoint is too close to HOME switch or crosses positive endpoint\"}");
        return;
      }
      a1NegativeLimitRawSteps = raw;
    } else if (side == "max") {
      if (raw <= a1NegativeLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"A1 positive endpoint must be farther from the negative endpoint\"}");
        return;
      }
      a1PositiveLimitRawSteps = raw;
    } else {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown side\"}");
      return;
    }

  } else if (axis == "a2") {
    if (!a2Referenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 has not been referenced by HOME yet\"}");
      return;
    }
    long raw = arm2.currentPosition();

    if (side == "min") {
      if (raw > -A2_MIN_SAFE_RAW_CLEARANCE || raw <= a2PositiveLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 negative endpoint is too close to HOME switch or crosses positive endpoint\"}");
        return;
      }
      a2NegativeLimitRawSteps = raw;
    } else if (side == "max") {
      if (raw >= a2NegativeLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 positive endpoint must be away from the negative endpoint\"}");
        return;
      }
      a2PositiveLimitRawSteps = raw;
    } else {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown side\"}");
      return;
    }

  } else if (axis == "z") {
    if (!zReferenced) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Z has not been referenced by HOME yet\"}");
      return;
    }
    long raw = zAxis.currentPosition();

    if (side == "top") {
      if (raw < Z_MIN_SAFE_RAW_CLEARANCE || raw >= zLowerLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Z upper endpoint is too close to HOME switch or crosses the lower endpoint\"}");
        return;
      }
      zUpperLimitRawSteps = raw;
    } else if (side == "bottom") {
      if (raw <= zUpperLimitRawSteps) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Z lower/ground endpoint must be below the upper endpoint\"}");
        return;
      }
      zLowerLimitRawSteps = raw;
    } else {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown Z side\"}");
      return;
    }
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown axis\"}");
    return;
  }

  saveCalibration();
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Physical raw endpoint saved; displayed operating limit recalculated\"}");
}

void handleCalibrationReset() {
  resetCalibration();
  resetCollisionSettings();
  emergencyStop("Calibration and collision safety reset. HOME required.");
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Calibration reset. HOME required.\"}");
}

void handleMotionProfile() {
  if (!server.hasArg("mode") || !server.hasArg("pct")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing mode or percentage\"}");
    return;
  }

  String mode = server.arg("mode");
  int requestedPercent = constrain(server.arg("pct").toInt(), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  MotionProfileMode changedProfile;

  if (mode == "manual") {
    manualFkSpeedPct = requestedPercent;
    changedProfile = PROFILE_MANUAL_FK;
  } else if (mode == "realtime") {
    realtimeSpeedPct = requestedPercent;
    changedProfile = PROFILE_REALTIME;
  } else if (mode == "ik") {
    ikSpeedPct = requestedPercent;
    changedProfile = PROFILE_IK;
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown motion profile\"}");
    return;
  }

  bool appliedNow = (activeMotionProfile == changedProfile);
  if (appliedNow) {
    // Changing the active profile while moving changes the AccelStepper
    // max-speed/acceleration ceiling immediately and smoothly.
    applyMotionProfile(changedProfile);
  }

  String json = "{\"ok\":true,\"msg\":\"Motion scale accepted\"";
  json += ",\"mode\":\"" + mode + "\"";
  json += ",\"pct\":" + String(requestedPercent);
  json += ",\"appliedNow\":" + String(appliedNow ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleMotionProfileSave() {
  saveMotionProfiles();
  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Motion speed profiles saved\"}");
}

void handleToolTCPSet() {
  if (!server.hasArg("mm")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing TCP length\"}");
    return;
  }

  float requested = server.arg("mm").toFloat();
  if (requested < 20.0f || requested > 250.0f) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"TCP length must be 20 to 250 mm\"}");
    return;
  }

  toolTcpLengthMm = requested;
  saveToolSettings();
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"TCP length saved for FK and IK\"}");
}

void handleA2HomeParkSet() {
  if ((!isHomed && !setupRequired) || isHoming || anyMotion() || !a2Referenced) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 must be referenced and idle first\"}");
    return;
  }

  long raw = arm2.currentPosition();
  if (raw >= a2ZeroSteps || raw > -A2_MIN_SAFE_RAW_CLEARANCE) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"A2 HOME-clearance park pose must be on the positive clearance side of A2 zero\"}");
    return;
  }

  a2HomeClearanceParkRawSteps = raw;
  saveCalibration();

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"A2 A1-HOME raw clearance pose saved\",\"deg\":" + String(a2HomeClearanceParkDeg, 3) + "}");
}


void handleGripperMove() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked while a job is loaded.\"}");
    return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked during recipe playback.\"}");
    return;
  }
  if (!server.hasArg("deg")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing gripper angle\"}");
    return;
  }

  if (isHoming || homeRequested) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked during HOME\"}");
    return;
  }

  String error;
  const int requestedDeg = server.arg("deg").toInt();
  if (requestedDeg > gripperOpenDeg &&
      !collisionPoseIsSafe(actualA1(), actualA2(), actualZ(), requestedDeg, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  if (!commandGripperAngle(requestedDeg, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Gripper angle commanded\",\"deg\":" + String(gripperCommandDeg) + "}");
}

void handleGripperOpen() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked while a job is loaded.\"}");
    return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked during recipe playback.\"}");
    return;
  }
  if (isHoming || homeRequested) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"HOME is already controlling the gripper\"}");
    return;
  }

  String error;
  if (!commandGripperOpen(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Gripper OPEN commanded\",\"deg\":" + String(gripperCommandDeg) + "}");
}

void handleGripperHold() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked while a job is loaded.\"}");
    return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Manual gripper control is blocked during recipe playback.\"}");
    return;
  }
  if (isHoming || homeRequested) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Gripper HOLD is blocked during HOME\"}");
    return;
  }

  String error;
  if (!collisionPoseIsSafe(actualA1(), actualA2(), actualZ(), gripperHoldDeg, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  if (!commandGripperHold(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Gripper BOX HOLD commanded\",\"deg\":" + String(gripperCommandDeg) + "}");
}

void handleGripperSetCalibration() {
  if (!server.hasArg("pose")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing gripper calibration pose\"}");
    return;
  }

  if (isHoming || homeRequested) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Cannot calibrate gripper during HOME\"}");
    return;
  }

  String pose = server.arg("pose");
  uint8_t newOpen = gripperOpenDeg;
  uint8_t newHold = gripperHoldDeg;
  uint8_t newMax = gripperMaxCloseDeg;

  if (pose == "open") {
    newOpen = gripperCommandDeg;
  } else if (pose == "hold") {
    newHold = gripperCommandDeg;
  } else if (pose == "maxclose") {
    newMax = gripperCommandDeg;
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Unknown gripper pose\"}");
    return;
  }

  String error;
  if (!validateGripperCalibration(newOpen, newHold, newMax, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  gripperOpenDeg = newOpen;
  gripperHoldDeg = newHold;
  gripperMaxCloseDeg = newMax;
  saveGripperSettings();

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Gripper calibration saved\",\"open\":" + String(gripperOpenDeg) +
    ",\"hold\":" + String(gripperHoldDeg) + ",\"maxclose\":" + String(gripperMaxCloseDeg) + "}");
}

void handleGripperResetCalibration() {
  if (isHoming || homeRequested) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Cannot reset gripper during HOME\"}");
    return;
  }

  resetGripperSettings();
  String error;
  commandGripperOpen(error);

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Gripper defaults restored: OPEN 0, HOLD 70, MAX CLOSE 90\"}");
}

void handleStartupConfirm() {
  if (isHoming || homeRequested || anyMotion()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Wait until the robot is idle before confirming startup pose\"}");
    return;
  }

  startupPoseConfirmed = true;
  faultActive = false;
  faultMessage = "";
  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Safe Startup Pose confirmed by operator. HOME is now enabled for this session.\"}");
}

void handleStartupParkSave() {
  String error;
  if (!saveCurrentStartupPark(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Safe Startup / Power-Off Park pose saved\"}");
}

void handleStartupParkGo() {
  if (!startupParkSaved) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"No Safe Startup / Power-Off Park pose is saved\"}");
    return;
  }

  if (!isHomed || isHoming || anyMotion() || faultActive) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Robot must be ready and idle before PARK FOR POWER-OFF\"}");
    return;
  }

  String error;
  if (!commandGripperOpen(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Cannot open gripper for park: " + error + "\"}");
    return;
  }

  delay(GRIPPER_OPEN_BEFORE_HOME_MS);

  const float parkA1 = a1StepsToAngle(startupParkA1RawSteps);
  const float parkA2 = a2StepsToAngle(startupParkA2RawSteps);
  const float parkZ = zStepsToMm(startupParkZRawSteps);

  if (!queueJointMove(parkA1, parkA2, parkZ, PROFILE_MANUAL_FK, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"PARK path rejected: " + error + "\"}");
    return;
  }

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Moving to Safe Startup / Power-Off Park. Wait until motion stops before powering off.\"}");
}

bool requestPackageRecipeIndex(uint8_t &recipeIndex, String &error) {
  if (!server.hasArg("recipe")) { error = "Select a recipe first."; return false; }
  int value = server.arg("recipe").toInt();
  if (!validPackageRecipeIndex(value)) { error = "Selected recipe does not exist."; return false; }
  recipeIndex = (uint8_t)value;
  return true;
}

bool requestRecipeStepIndex(uint8_t recipeIndex, uint8_t &stepIndex, String &error) {
  if (!server.hasArg("step")) { error = "Select a step first."; return false; }
  int value = server.arg("step").toInt();
  if (!validRecipeStepIndex(recipeIndex, value)) { error = "Selected step does not exist."; return false; }
  stepIndex = (uint8_t)value;
  return true;
}

bool parseRecipeStepRequest(RecipeStep &step, String &error) {
  if (!server.hasArg("name") || !server.hasArg("command") ||
      !server.hasArg("a1") || !server.hasArg("a2") || !server.hasArg("z") ||
      !server.hasArg("gripper") || !server.hasArg("waitms")) {
    error = "Step data is incomplete."; return false;
  }
  int waitValue = server.arg("waitms").toInt();
  if (waitValue < 0 || waitValue > RECIPE_MAX_WAIT_MS) {
    error = "Pause must be from 0.0 to 30.0 seconds."; return false;
  }
  setRecipeStep(step, server.arg("name"), parseRecipeCommand(server.arg("command")),
                server.arg("a1").toFloat(), server.arg("a2").toFloat(), server.arg("z").toFloat(),
                (uint8_t)constrain(server.arg("gripper").toInt(), 0, (int)GRIPPER_HARD_MAX_DEG),
                (uint16_t)waitValue, 1);
  return validateRecipeStepDefinition(step, error);
}

void handlePackageRecipeCreate() {
  uint8_t index; String error;
  if (!createPackageRecipe(server.hasArg("name") ? server.arg("name") : "", index, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\",\"recipe\":" + String(index) + "}");
}
void handlePackageRecipeRename() {
  uint8_t index; String error;
  if (!requestPackageRecipeIndex(index, error) ||
      !renamePackageRecipe(index, server.hasArg("name") ? server.arg("name") : "", error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handlePackageRecipeDelete() {
  uint8_t index; String error;
  if (!requestPackageRecipeIndex(index, error) || !deletePackageRecipe(index, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handleRecipeStepAdd() {
  uint8_t recipeIndex, stepIndex; String error; RecipeStep step;
  if (!requestPackageRecipeIndex(recipeIndex, error) || !parseRecipeStepRequest(step, error) ||
      !addRecipeStep(recipeIndex, step, stepIndex, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\",\"step\":" + String(stepIndex) + "}");
}
void handleRecipeStepUpdate() {
  uint8_t recipeIndex, stepIndex; String error; RecipeStep step;
  if (!requestPackageRecipeIndex(recipeIndex, error) || !requestRecipeStepIndex(recipeIndex, stepIndex, error) ||
      !parseRecipeStepRequest(step, error) || !updateRecipeStep(recipeIndex, stepIndex, step, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handleRecipeStepDelete() {
  uint8_t recipeIndex, stepIndex; String error;
  if (!requestPackageRecipeIndex(recipeIndex, error) || !requestRecipeStepIndex(recipeIndex, stepIndex, error) ||
      !deleteRecipeStep(recipeIndex, stepIndex, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handleRecipeStepReorder() {
  uint8_t recipeIndex, stepIndex; String error;
  if (!requestPackageRecipeIndex(recipeIndex, error) || !requestRecipeStepIndex(recipeIndex, stepIndex, error) ||
      !server.hasArg("direction") || !reorderRecipeStep(recipeIndex, stepIndex, server.arg("direction") == "up", error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handlePackageRecipePlay() {
  if (armJobLocksManualControl()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"A job is loaded. Use Jobs or abort it before direct recipe testing.\"}");
    return;
  }
  uint8_t recipeIndex; String error;
  int repeats = server.hasArg("repeat") ? server.arg("repeat").toInt() : 1;
  if (!requestPackageRecipeIndex(recipeIndex, error) || !startPackageRecipe(recipeIndex, (uint16_t)repeats, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handlePackageRecipeReset() {
  String error;
  if (!resetRecipeStatus(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}
void handlePackageRecipeExport() {
  String json = "{\"format\":\"SCARA_COMMAND_RECIPES_BACKUP\",\"version\":2,\"units\":\"degrees_mm_seconds\",\"recipes\":[";
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    if (r > 0) json += ",";
    json += "{\"name\":\"" + String(packageRecipes[r].name) + "\",\"steps\":[";
    for (uint8_t s = 0; s < packageRecipes[r].stepCount; s++) {
      if (s > 0) json += ",";
      RecipeStep &step = packageRecipes[r].steps[s];
      json += "{\"name\":\"" + String(step.name) + "\"";
      json += ",\"command\":\"" + String(recipeCommandName(step.command)) + "\"";
      json += ",\"a1Deg\":" + String(a1StepsToAngle(step.a1RawSteps), 3);
      json += ",\"a2Deg\":" + String(a2StepsToAngle(step.a2RawSteps), 3);
      json += ",\"zMm\":" + String(zStepsToMm(step.zRawSteps), 3);
      json += ",\"gripperDeg\":" + String(step.gripperDeg);
      json += ",\"waitMs\":" + String(step.waitAfterMs) + "}";
    }
    json += "]}";
  }
  json += "]}";
  server.sendHeader("Content-Disposition", "attachment; filename=SCARA_command_recipes_v10_6.json");
  server.send(200, "application/json", json);
}
void handlePackageRecipeImport() {
  if (armJobLocksManualControl() && !recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Abort the loaded job before restoring recipes.\"}"); return;
  }
  if (recipeRunning) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Stop playback before restoring recipes.\"}"); return;
  }
  if (!server.hasArg("recipeCount")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Recipe JSON is incomplete.\"}"); return;
  }
  int recipeCount = server.arg("recipeCount").toInt();
  if (recipeCount < 0 || recipeCount > MAX_PACKAGE_RECIPES) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Too many recipes in imported file.\"}"); return;
  }
  for (uint8_t r = 0; r < (uint8_t)recipeCount; r++) {
    PackageRecipe &recipe = recipeImportScratch[r];
    memset(&recipe, 0, sizeof(PackageRecipe));
    String rk = "r" + String(r);
    if (!server.hasArg(rk + "n") || !server.hasArg(rk + "c")) {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Imported recipe is incomplete.\"}"); return;
    }
    copyRecipeText(recipe.name, PACKAGE_NAME_LENGTH, server.arg(rk + "n"), "RECIPE_" + String(r + 1));
    int stepCount = server.arg(rk + "c").toInt();
    if (stepCount < 0 || stepCount > MAX_RECIPE_STEPS) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported recipe has too many steps.\"}"); return;
    }
    recipe.stepCount = (uint8_t)stepCount;
    for (uint8_t s = 0; s < recipe.stepCount; s++) {
      String sk = rk + "s" + String(s);
      if (!server.hasArg(sk + "n") || !server.hasArg(sk + "cmd") || !server.hasArg(sk + "a1") ||
          !server.hasArg(sk + "a2") || !server.hasArg(sk + "z") || !server.hasArg(sk + "g") ||
          !server.hasArg(sk + "w")) {
        server.send(400, "application/json", "{\"ok\":false,\"err\":\"Imported step is incomplete.\"}"); return;
      }
      int waitMs = server.arg(sk + "w").toInt();
      if (waitMs < 0 || waitMs > RECIPE_MAX_WAIT_MS) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported wait value is invalid.\"}"); return;
      }
      setRecipeStep(recipe.steps[s], server.arg(sk + "n"), parseRecipeCommand(server.arg(sk + "cmd")),
                    server.arg(sk + "a1").toFloat(), server.arg(sk + "a2").toFloat(), server.arg(sk + "z").toFloat(),
                    (uint8_t)server.arg(sk + "g").toInt(), (uint16_t)waitMs, s + 1);
      String error;
      if (!validateRecipeStepDefinition(recipe.steps[s], error)) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported step rejected: " + error + "\"}"); return;
      }
    }
  }
  packageRecipeCount = (uint8_t)recipeCount;
  for (uint8_t r = 0; r < packageRecipeCount; r++) packageRecipes[r] = recipeImportScratch[r];
  savePackageRecipes();
  recipeState = RECIPE_IDLE;
  recipeLastEvent = "Command recipes restored and saved inside ESP32 field library.";
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Recipes restored into RAM and saved inside ESP32 atomic store. Press Permanent Save + Verify, then restart to test persistence.\"}");
}


uint32_t recipeStorageChecksum() {
  uint32_t hash = 2166136261UL;
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    const uint8_t *bytes = (const uint8_t*)&packageRecipes[r];
    for (size_t i = 0; i < sizeof(PackageRecipe); i++) {
      hash ^= bytes[i];
      hash *= 16777619UL;
    }
  }
  return hash;
}

uint32_t recipeHashAddByte(uint32_t hash, uint8_t value) {
  hash ^= value;
  hash *= 16777619UL;
  return hash;
}

uint32_t recipeHashAddLong(uint32_t hash, long value) {
  for (uint8_t i = 0; i < 4; i++) hash = recipeHashAddByte(hash, (uint8_t)((value >> (i * 8)) & 0xFF));
  return hash;
}

uint32_t recipeHashAddUInt(uint32_t hash, uint16_t value) {
  hash = recipeHashAddByte(hash, (uint8_t)(value & 0xFF));
  hash = recipeHashAddByte(hash, (uint8_t)((value >> 8) & 0xFF));
  return hash;
}

uint32_t recipeHashAddText(uint32_t hash, const char *text) {
  if (!text) return recipeHashAddByte(hash, 0);
  while (*text) {
    hash = recipeHashAddByte(hash, (uint8_t)*text);
    text++;
  }
  return recipeHashAddByte(hash, 0);
}

uint32_t recipeLogicalChecksum() {
  uint32_t hash = 2166136261UL;
  hash = recipeHashAddByte(hash, packageRecipeCount);
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    PackageRecipe &recipe = packageRecipes[r];
    hash = recipeHashAddText(hash, recipe.name);
    hash = recipeHashAddByte(hash, recipe.stepCount);
    for (uint8_t s = 0; s < recipe.stepCount; s++) {
      RecipeStep &step = recipe.steps[s];
      hash = recipeHashAddText(hash, step.name);
      hash = recipeHashAddByte(hash, step.command);
      hash = recipeHashAddLong(hash, step.a1RawSteps);
      hash = recipeHashAddLong(hash, step.a2RawSteps);
      hash = recipeHashAddLong(hash, step.zRawSteps);
      hash = recipeHashAddByte(hash, step.gripperDeg);
      hash = recipeHashAddUInt(hash, step.waitAfterMs);
    }
  }
  return hash;
}


void handleRecipeSaveNow() {
  if (recipeRunning || armJobLocksManualControl()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Stop playback/job before saving recipes.\"}");
    return;
  }
  savePackageRecipes();
  String json = "{\"ok\":true";
  json += ",\"msg\":\"Recipes saved to ESP32 Preferences atomic blob store. Use Commit + Verify for readback test.\"";
  json += ",\"count\":" + String(packageRecipeCount);
  json += ",\"checksum\":" + String(recipeStorageChecksum());
  json += "}";
  server.send(200, "application/json", json);
}

void handleRecipeCommitVerify() {
  if (recipeRunning || armJobLocksManualControl()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Stop playback/job before committing recipes.\"}");
    return;
  }
  uint8_t countBefore = packageRecipeCount;
  uint32_t logicalBefore = recipeLogicalChecksum();
  bool saveOk = savePackageRecipes();

  uint8_t storedCount = 0;
  uint32_t storedCrc = 0;
  uint32_t storedBytes = 0;
  bool verifyOk = false;
  if (saveOk) verifyOk = verifyStoredPackageRecipesAgainstRam(storedCount, storedCrc, storedBytes);

  uint32_t logicalAfter = recipeLogicalChecksum();
  bool countOk = countBefore == packageRecipeCount && storedCount == packageRecipeCount;
  bool hashOk = logicalBefore == logicalAfter;
  bool ok = saveOk && verifyOk && countOk && hashOk;

  String json = "{\"ok\":";
  json += String(ok ? "true" : "false");
  json += ",\"msg\":\"" + String(ok ? "Recipes committed and verified. RAM was not erased." : "Commit verify failed. RAM recipe was kept; storage was not reloaded.") + "\"";
  json += ",\"count\":" + String(packageRecipeCount);
  json += ",\"storedCount\":" + String(storedCount);
  json += ",\"storedBytes\":" + String(storedBytes);
  json += ",\"logicalBefore\":" + String(logicalBefore);
  json += ",\"logicalAfter\":" + String(logicalAfter);
  json += ",\"storedCrc\":" + String(storedCrc);
  json += ",\"saveOk\":" + String(saveOk ? "true" : "false");
  json += ",\"verifyOk\":" + String(verifyOk ? "true" : "false");
  json += ",\"countOk\":" + String(countOk ? "true" : "false");
  json += ",\"hashOk\":" + String(hashOk ? "true" : "false");
  json += "}";
  server.send(ok ? 200 : 507, "application/json", json);
}

void handleRecipeStorageStatus() {
  uint8_t storedCount = 0;
  uint32_t storedCrc = 0;
  uint32_t storedBytes = 0;
  bool storedOk = getStoredRecipeInfo(storedCount, storedCrc, storedBytes);
  bool fsOk = recipeFileSystemReady();
  String json = "{\"ok\":true";
  json += ",\"storage\":\"LittleFS\"";
  json += ",\"file\":\"/scara_recipes.dat\"";
  json += ",\"format\":\"littlefs_atomic_file\"";
  json += ",\"fsOk\":" + String(fsOk ? "true" : "false");
  json += ",\"fileExists\":" + String(recipeFileExists() ? "true" : "false");
  json += ",\"storedOk\":" + String(storedOk ? "true" : "false");
  json += ",\"formatOk\":" + String(storedOk ? "true" : "false");
  json += ",\"versionOk\":" + String(storedOk ? "true" : "false");
  json += ",\"storedCount\":" + String(storedCount);
  json += ",\"count\":" + String(packageRecipeCount);
  json += ",\"blobBytes\":" + String(storedBytes);
  json += ",\"storedCrc\":" + String(storedCrc);
  json += ",\"checksum\":" + String(recipeStorageChecksum());
  json += ",\"logicalChecksum\":" + String(recipeLogicalChecksum());
  json += ",\"fsTotal\":" + String(recipeFsTotalBytes());
  json += ",\"fsUsed\":" + String(recipeFsUsedBytes());
  json += ",\"fsMessage\":\"" + recipeFileSystemError() + "\"";
  json += ",\"recipes\":[";
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    if (r > 0) json += ",";
    json += "{\"index\":" + String(r);
    json += ",\"name\":\"" + String(packageRecipes[r].name) + "\"";
    json += ",\"steps\":" + String(packageRecipes[r].stepCount) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleRecipeReloadStorage() {
  String message;
  if (!reloadPackageRecipesFromEsp32(message)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + message + "\"}");
    return;
  }
  String json = "{\"ok\":true";
  json += ",\"msg\":\"" + message + "\"";
  json += ",\"count\":" + String(packageRecipeCount);
  json += ",\"checksum\":" + String(recipeStorageChecksum());
  json += ",\"logicalChecksum\":" + String(recipeLogicalChecksum());
  json += "}";
  server.send(200, "application/json", json);
}


String recipeStepJson(uint8_t r, uint8_t s) {
  RecipeStep &step = packageRecipes[r].steps[s];
  String json = "{";
  json += "\"index\":" + String(s);
  json += ",\"name\":\"" + String(step.name) + "\"";
  json += ",\"command\":\"" + String(recipeCommandName(step.command)) + "\"";
  json += ",\"a1Deg\":" + String(a1StepsToAngle(step.a1RawSteps), 2);
  json += ",\"a2Deg\":" + String(a2StepsToAngle(step.a2RawSteps), 2);
  json += ",\"zMm\":" + String(zStepsToMm(step.zRawSteps), 2);
  json += ",\"gripperDeg\":" + String(step.gripperDeg);
  json += ",\"waitMs\":" + String(step.waitAfterMs);
  json += "}";
  return json;
}

void handleRecipeLibrary() {
  uint8_t storedCount = 0;
  uint32_t storedCrc = 0;
  uint32_t storedBytes = 0;
  bool storedOk = getStoredRecipeInfo(storedCount, storedCrc, storedBytes);
  String json = "{\"ok\":true";
  json += ",\"storage\":\"LittleFS\"";
  json += ",\"file\":\"/scara_recipes.dat\"";
  json += ",\"format\":\"littlefs_atomic_file\"";
  json += ",\"count\":" + String(packageRecipeCount);
  json += ",\"fsOk\":" + String(recipeFileSystemReady() ? "true" : "false");
  json += ",\"storedOk\":" + String(storedOk ? "true" : "false");
  json += ",\"formatOk\":" + String(storedOk ? "true" : "false");
  json += ",\"versionOk\":" + String(storedOk ? "true" : "false");
  json += ",\"storedCount\":" + String(storedCount);
  json += ",\"blobBytes\":" + String(storedBytes);
  json += ",\"storedCrc\":" + String(storedCrc);
  json += ",\"logicalChecksum\":" + String(recipeLogicalChecksum());
  json += ",\"fsTotal\":" + String(recipeFsTotalBytes());
  json += ",\"fsUsed\":" + String(recipeFsUsedBytes());
  json += ",\"recipes\":[";
  for (uint8_t r = 0; r < packageRecipeCount; r++) {
    if (r > 0) json += ",";
    json += "{\"index\":" + String(r);
    json += ",\"name\":\"" + String(packageRecipes[r].name) + "\"";
    json += ",\"steps\":[";
    for (uint8_t s = 0; s < packageRecipes[r].stepCount; s++) {
      if (s > 0) json += ",";
      json += recipeStepJson(r, s);
    }
    json += "]}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleRecipeClearAll() {
  if (recipeRunning || armJobLocksManualControl()) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Stop playback/job before clearing saved recipes.\"}");
    return;
  }
  clearAllPackageRecipes(true);
  String json = "{\"ok\":true";
  json += ",\"msg\":\"All saved recipes cleared from ESP32. Library is now empty.\"";
  json += ",\"count\":" + String(packageRecipeCount);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRecipeClearLegacy() {
  Preferences legacy;
  legacy.begin("scara_pkg1", false); legacy.clear(); legacy.end();
  legacy.begin("scara_pkg2", false); legacy.clear(); legacy.end();
  legacy.begin("scara_pkg3", false); legacy.clear(); legacy.end();
  legacy.begin("scara_pkg4", false); legacy.clear(); legacy.end();
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Old NVS recipe storage scara_pkg1/scara_pkg2/scara_pkg3/scara_pkg4 cleared. LittleFS recipe file is not deleted by this button.\"}");
}

void handleJobLoad() {
  if (!server.hasArg("recipe")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Select a recipe.\"}");
    return;
  }
  int requestedRecipe = server.arg("recipe").toInt();
  if (requestedRecipe < 0 || requestedRecipe >= packageRecipeCount) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Selected recipe does not exist.\"}");
    return;
  }

  String error;
  String jobId = server.hasArg("jobId") ? server.arg("jobId") : "JOB_001";
  String packageClass = server.hasArg("packageClass") ? server.arg("packageClass") : "UNCLASSIFIED_BOX";
  if (!loadArmJob(jobId, packageClass, (uint8_t)requestedRecipe, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleJobWaitForAmr() {
  String error;
  if (!setArmJobWaitingForAmr(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleJobAmrArrived() {
  String error;
  if (!confirmArmJobAmrArrival(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleJobAbort() {
  String error;
  if (!abortArmJob(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleJobClear() {
  String error;
  if (!clearArmJobResult(error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleJobStatus() {
  String json = "{\"ok\":true";
  json += ",\"state\":\"" + String(armJobStateName(armJobState)) + "\"";
  json += ",\"jobId\":\"" + String(activeArmJob.jobId) + "\"";
  json += ",\"packageClass\":\"" + String(activeArmJob.packageClass) + "\"";
  json += ",\"recipe\":\"" + activeArmJobRecipeName() + "\"";
  json += ",\"amrConfirmed\":" + String(activeArmJob.amrConfirmed ? "true" : "false");
  json += ",\"currentStep\":" + String((armJobState == ARM_JOB_RUNNING) ? recipeStepIndex + 1 : 0);
  json += ",\"result\":\"" + armJobResult + "\"";
  json += ",\"message\":\"" + armJobMessage + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

bool parseLayerIndex(uint8_t &layerIndex, String &error) {
  if (!server.hasArg("layer")) {
    error = "Select a Z envelope layer first.";
    return false;
  }
  int requested = server.arg("layer").toInt();
  if (requested < 0 || requested >= envelopeLayerCount) {
    error = "Selected Z envelope layer does not exist.";
    return false;
  }
  layerIndex = (uint8_t)requested;
  return true;
}

bool parseLayerDefinition(uint8_t &region, uint8_t &bound, String &error) {
  if (!server.hasArg("region") || !server.hasArg("bound")) {
    error = "Missing A1 side or A2 boundary direction.";
    return false;
  }
  region = server.arg("region") == "positive" ? A1_REGION_POSITIVE : A1_REGION_NEGATIVE;
  bound = server.arg("bound") == "max" ? A2_BOUND_MAXIMUM : A2_BOUND_MINIMUM;
  return true;
}

void handleCollisionLayerCreate() {
  if (!server.hasArg("zfrom") || !server.hasArg("zto")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing Z layer range\"}");
    return;
  }
  uint8_t region, bound, index;
  String error;
  if (!parseLayerDefinition(region, bound, error) ||
      !createCollisionEnvelopeLayer(region, bound, server.arg("zfrom").toFloat(),
                                    server.arg("zto").toFloat(), index, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"" + error + "\",\"layer\":" + String(index) + "}");
}

void handleCollisionLayerDelete() {
  uint8_t layerIndex;
  String error;
  if (!parseLayerIndex(layerIndex, error) || !deleteCollisionEnvelopeLayer(layerIndex, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeAdd() {
  uint8_t layerIndex;
  String error;
  if (!parseLayerIndex(layerIndex, error) || !server.hasArg("a1") || !server.hasArg("a2")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing selected layer, A1 or A2 limit\"}");
    return;
  }
  if (!addCollisionEnvelopePoint(layerIndex, server.arg("a1").toFloat(), server.arg("a2").toFloat(), error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeCapture() {
  uint8_t layerIndex;
  String error;
  if (!parseLayerIndex(layerIndex, error) || !captureCurrentCollisionEnvelopePoint(layerIndex, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeDelete() {
  uint8_t layerIndex;
  String error;
  if (!parseLayerIndex(layerIndex, error) || !server.hasArg("index") ||
      !deleteCollisionEnvelopePoint(layerIndex, (uint8_t)server.arg("index").toInt(), error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeClear() {
  uint8_t layerIndex;
  String error;
  if (!parseLayerIndex(layerIndex, error) || !clearCollisionEnvelopeLayer(layerIndex, error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeMargin() {
  if (!server.hasArg("deg")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing envelope margin\"}");
    return;
  }
  String error;
  if (!setCollisionEnvelopeMargin(server.arg("deg").toFloat(), error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionEnvelopeCorridor() {
  if (!server.hasArg("a2")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing A2 corridor angle\"}");
    return;
  }
  String error;
  if (!setCollisionSafeCorridor(server.arg("a2").toFloat(), error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionSetProtection() {
  if (!server.hasArg("enabled")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Missing protection state\"}");
    return;
  }
  String error;
  if (!setCollisionProtection(server.arg("enabled") == "1", error)) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"" + error + "\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + error + "\"}");
}

void handleCollisionLayerExport() {
  String json = "{\"format\":\"SCARA_COLLISION_LAYER_BACKUP\",\"version\":1,\"units\":\"degrees_mm\"";
  json += ",\"marginDeg\":" + String(collisionEnvelopeMarginDeg, 3);
  json += ",\"corridorA2Deg\":" + String(collisionSafeCorridorA2Deg, 3);
  json += ",\"sourceLimits\":{\"a1MinDeg\":" + String(a1MinDeg, 3);
  json += ",\"a1MaxDeg\":" + String(a1MaxDeg, 3);
  json += ",\"a2MinDeg\":" + String(a2MinDeg, 3);
  json += ",\"a2MaxDeg\":" + String(a2MaxDeg, 3);
  json += ",\"zMinMm\":" + String(zMinMm, 3);
  json += ",\"zMaxMm\":" + String(zMaxMm, 3) + "}";
  json += ",\"layers\":[";
  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    if (layerIndex > 0) json += ",";
    CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    float za = zStepsToMm(layer.zRawMin);
    float zb = zStepsToMm(layer.zRawMax);
    float zFrom = fmin(za, zb);
    float zTo = fmax(za, zb);
    json += "{\"region\":\"" + String(layerUsesPositiveA1(layer) ? "positive" : "negative") + "\"";
    json += ",\"bound\":\"" + String(layerUsesMaximumA2(layer) ? "max" : "min") + "\"";
    json += ",\"zFromMm\":" + String(zFrom, 3);
    json += ",\"zToMm\":" + String(zTo, 3);
    json += ",\"points\":[";
    for (uint8_t i = 0; i < layer.pointCount; i++) {
      if (i > 0) json += ",";
      json += "{\"a1Deg\":" + String(a1StepsToAngle(layer.points[i].a1RawSteps), 3);
      json += ",\"a2LimitDeg\":" + String(a2StepsToAngle(layer.points[i].a2RawLimitSteps), 3) + "}";
    }
    json += "]}";
  }
  json += "]}";

  server.sendHeader("Content-Disposition", "attachment; filename=SCARA_collision_layers_v9_12.json");
  server.send(200, "application/json", json);
}

void handleCollisionLayerImport() {
  if (!server.hasArg("layerCount")) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Collision layer file is missing layer data\"}");
    return;
  }

  int requestedCount = server.arg("layerCount").toInt();
  if (requestedCount < 0 || requestedCount > MAX_ENVELOPE_LAYERS) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Collision layer count is invalid\"}");
    return;
  }

  float importedMargin = server.hasArg("marginDeg") ? server.arg("marginDeg").toFloat() : DEFAULT_ENVELOPE_MARGIN_DEG;
  float importedCorridor = server.hasArg("corridorA2Deg") ? server.arg("corridorA2Deg").toFloat() : DEFAULT_SAFE_CORRIDOR_A2_DEG;
  if (importedMargin < 0.0f || importedMargin > 20.0f) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported A2 margin is invalid\"}");
    return;
  }
  if (importedCorridor < a2MinDeg || importedCorridor > a2MaxDeg) {
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported safe corridor is outside current A2 calibration\"}");
    return;
  }

  CollisionEnvelopeLayer importedLayers[MAX_ENVELOPE_LAYERS];

  for (uint8_t layerIndex = 0; layerIndex < (uint8_t)requestedCount; layerIndex++) {
    String base = "l" + String(layerIndex);
    String regionKey = base + "region";
    String boundKey = base + "bound";
    String zFromKey = base + "zFromMm";
    String zToKey = base + "zToMm";
    String countKey = base + "pointCount";

    if (!server.hasArg(regionKey) || !server.hasArg(boundKey) ||
        !server.hasArg(zFromKey) || !server.hasArg(zToKey) ||
        !server.hasArg(countKey)) {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Collision layer file is incomplete\"}");
      return;
    }

    String regionText = server.arg(regionKey);
    String boundText = server.arg(boundKey);
    uint8_t region;
    uint8_t bound;
    if (regionText == "positive") region = A1_REGION_POSITIVE;
    else if (regionText == "negative") region = A1_REGION_NEGATIVE;
    else {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported A1 side is invalid\"}");
      return;
    }
    if (boundText == "max") bound = A2_BOUND_MAXIMUM;
    else if (boundText == "min") bound = A2_BOUND_MINIMUM;
    else {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported A2 boundary direction is invalid\"}");
      return;
    }

    float zFrom = server.arg(zFromKey).toFloat();
    float zTo = server.arg(zToKey).toFloat();
    if (zFrom > zTo) { float t = zFrom; zFrom = zTo; zTo = t; }
    if (zFrom < zMinMm - 0.01f || zTo > zMaxMm + 0.01f || zTo - zFrom < 0.01f) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported Z layer is outside current Z calibration\"}");
      return;
    }

    int pointCount = server.arg(countKey).toInt();
    if (pointCount < 0 || pointCount > MAX_ENVELOPE_POINTS) {
      server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported boundary point count is invalid\"}");
      return;
    }

    CollisionEnvelopeLayer &layer = importedLayers[layerIndex];
    layer.a1Region = region;
    layer.a2Bound = bound;
    long zRawA = zMmToSteps(zFrom);
    long zRawB = zMmToSteps(zTo);
    layer.zRawMin = zRawA < zRawB ? zRawA : zRawB;
    layer.zRawMax = zRawA < zRawB ? zRawB : zRawA;
    layer.pointCount = (uint8_t)pointCount;

    for (uint8_t i = 0; i < layer.pointCount; i++) {
      String point = base + "p" + String(i);
      String a1Key = point + "a1Deg";
      String a2Key = point + "a2LimitDeg";
      if (!server.hasArg(a1Key) || !server.hasArg(a2Key)) {
        server.send(400, "application/json", "{\"ok\":false,\"err\":\"Imported collision point is incomplete\"}");
        return;
      }

      float a1Deg = server.arg(a1Key).toFloat();
      float a2Deg = server.arg(a2Key).toFloat();
      if (a1Deg < a1MinDeg - 0.01f || a1Deg > a1MaxDeg + 0.01f ||
          a2Deg < a2MinDeg - 0.01f || a2Deg > a2MaxDeg + 0.01f) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported collision point is outside current A1/A2 calibration\"}");
        return;
      }
      if ((region == A1_REGION_POSITIVE && a1Deg < -0.01f) ||
          (region == A1_REGION_NEGATIVE && a1Deg > 0.01f)) {
        server.send(409, "application/json", "{\"ok\":false,\"err\":\"Imported point does not match its A1 side\"}");
        return;
      }

      layer.points[i].a1RawSteps = a1AngleToSteps(a1Deg);
      layer.points[i].a2RawLimitSteps = a2AngleToSteps(a2Deg);
    }
    sortCollisionEnvelopePoints(layer.points, layer.pointCount);
  }

  // Replace only collision envelope data. All other robot calibration remains unchanged.
  envelopeLayerCount = (uint8_t)requestedCount;
  for (uint8_t i = 0; i < envelopeLayerCount; i++) envelopeLayers[i] = importedLayers[i];
  collisionEnvelopeMarginDeg = importedMargin;
  collisionSafeCorridorA2Deg = importedCorridor;
  collisionProtectionEnabled = false;  // must be verified and enabled manually after import
  saveCollisionSettings();

  server.send(200, "application/json",
    "{\"ok\":true,\"msg\":\"Collision layers restored. Protection is disabled until you verify the layers and enable it.\"}");
}

void handleCalibrationExport() {
  String json = "{\"format\":\"SCARA_RAW_CAL_BACKUP\",\"version\":6";
  json += ",\"a1zero\":" + String(a1ZeroSteps) + ",\"a1neg\":" + String(a1NegativeLimitRawSteps) + ",\"a1pos\":" + String(a1PositiveLimitRawSteps);
  json += ",\"a2zero\":" + String(a2ZeroSteps) + ",\"a2neg\":" + String(a2NegativeLimitRawSteps) + ",\"a2pos\":" + String(a2PositiveLimitRawSteps) + ",\"a2park\":" + String(a2HomeClearanceParkRawSteps);
  json += ",\"zzero\":" + String(zZeroSteps) + ",\"ztop\":" + String(zUpperLimitRawSteps) + ",\"zbottom\":" + String(zLowerLimitRawSteps);
  json += ",\"tcpMm\":" + String(toolTcpLengthMm, 4) + ",\"manualPct\":" + String(manualFkSpeedPct) + ",\"realtimePct\":" + String(realtimeSpeedPct) + ",\"ikPct\":" + String(ikSpeedPct);
  json += ",\"gripOpen\":" + String(gripperOpenDeg) + ",\"gripHold\":" + String(gripperHoldDeg) + ",\"gripMax\":" + String(gripperMaxCloseDeg);
  json += ",\"envEnabled\":" + String(collisionProtectionEnabled ? "true" : "false");
  json += ",\"envMargin\":" + String(collisionEnvelopeMarginDeg, 3);
  json += ",\"envCorridor\":" + String(collisionSafeCorridorA2Deg, 3);
  json += ",\"parkValid\":" + String(startupParkSaved ? "true" : "false") + ",\"parkA1\":" + String(startupParkA1RawSteps) + ",\"parkA2\":" + String(startupParkA2RawSteps) + ",\"parkZ\":" + String(startupParkZRawSteps);
  json += ",\"layerCount\":" + String(envelopeLayerCount);
  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    String base = "l" + String(layerIndex);
    json += ",\"" + base + "region\":" + String(layer.a1Region);
    json += ",\"" + base + "bound\":" + String(layer.a2Bound);
    json += ",\"" + base + "zmin\":" + String(layer.zRawMin);
    json += ",\"" + base + "zmax\":" + String(layer.zRawMax);
    json += ",\"" + base + "count\":" + String(layer.pointCount);
    for (uint8_t i = 0; i < layer.pointCount; i++) {
      String point = base + "p" + String(i);
      json += ",\"" + point + "a\":" + String(layer.points[i].a1RawSteps);
      json += ",\"" + point + "b\":" + String(layer.points[i].a2RawLimitSteps);
    }
  }
  json += "}";
  server.sendHeader("Content-Disposition", "attachment; filename=SCARA_calibration_backup_v9_11.json");
  server.send(200, "application/json", json);
}

void handleCalibrationImport() {
  const char* required[] = {"a1zero","a1neg","a1pos","a2zero","a2neg","a2pos","a2park","zzero","ztop","zbottom","tcpMm","manualPct","realtimePct","ikPct"};
  for (uint8_t i = 0; i < sizeof(required) / sizeof(required[0]); i++) {
    if (!server.hasArg(required[i])) {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Backup missing robot calibration values\"}");
      return;
    }
  }

  long a1z = server.arg("a1zero").toInt(), a1n = server.arg("a1neg").toInt(), a1p = server.arg("a1pos").toInt();
  long a2z = server.arg("a2zero").toInt(), a2n = server.arg("a2neg").toInt(), a2p = server.arg("a2pos").toInt(), a2park = server.arg("a2park").toInt();
  long zz = server.arg("zzero").toInt(), zt = server.arg("ztop").toInt(), zb = server.arg("zbottom").toInt();
  float tcp = server.arg("tcpMm").toFloat();

  if (a1z <= A1_MIN_SAFE_RAW_CLEARANCE || a1n < A1_MIN_SAFE_RAW_CLEARANCE || a1p <= a1n || a1z < a1n || a1z > a1p) { server.send(409, "application/json", "{\"ok\":false,\"err\":\"Unsafe A1 backup\"}"); return; }
  if (a2z >= -A2_MIN_SAFE_RAW_CLEARANCE || a2n > -A2_MIN_SAFE_RAW_CLEARANCE || a2p >= a2n || a2z > a2n || a2z < a2p || a2park < a2p || a2park > a2n || a2park >= a2z) { server.send(409, "application/json", "{\"ok\":false,\"err\":\"Unsafe A2 backup\"}"); return; }
  if (zz <= Z_MIN_SAFE_RAW_CLEARANCE || zt < Z_MIN_SAFE_RAW_CLEARANCE || zb <= zt || zz < zt || zz > zb) { server.send(409, "application/json", "{\"ok\":false,\"err\":\"Unsafe Z backup\"}"); return; }
  if (tcp < 20.0f || tcp > 250.0f) { server.send(409, "application/json", "{\"ok\":false,\"err\":\"Invalid TCP backup\"}"); return; }

  uint8_t go = server.hasArg("gripOpen") ? (uint8_t)server.arg("gripOpen").toInt() : DEFAULT_GRIPPER_OPEN_DEG;
  uint8_t gh = server.hasArg("gripHold") ? (uint8_t)server.arg("gripHold").toInt() : DEFAULT_GRIPPER_HOLD_DEG;
  uint8_t gm = server.hasArg("gripMax") ? (uint8_t)server.arg("gripMax").toInt() : DEFAULT_GRIPPER_MAX_CLOSE_DEG;
  String error;
  if (!validateGripperCalibration(go, gh, gm, error)) { server.send(409, "application/json", "{\"ok\":false,\"err\":\"Invalid gripper backup\"}"); return; }

  CollisionEnvelopeLayer importedLayers[MAX_ENVELOPE_LAYERS];
  uint8_t importedLayerCount = 0;

  if (server.hasArg("layerCount")) {
    importedLayerCount = constrain(server.arg("layerCount").toInt(), 0, (int)MAX_ENVELOPE_LAYERS);
    for (uint8_t layerIndex = 0; layerIndex < importedLayerCount; layerIndex++) {
      String base = "l" + String(layerIndex);
      if (!server.hasArg(base + "region") || !server.hasArg(base + "bound") ||
          !server.hasArg(base + "zmin") || !server.hasArg(base + "zmax") ||
          !server.hasArg(base + "count")) {
        server.send(400, "application/json", "{\"ok\":false,\"err\":\"Backup missing Z-layer envelope values\"}");
        return;
      }
      CollisionEnvelopeLayer &layer = importedLayers[layerIndex];
      layer.a1Region = (uint8_t)server.arg(base + "region").toInt();
      layer.a2Bound = (uint8_t)server.arg(base + "bound").toInt();
      layer.zRawMin = server.arg(base + "zmin").toInt();
      layer.zRawMax = server.arg(base + "zmax").toInt();
      layer.pointCount = constrain(server.arg(base + "count").toInt(), 0, (int)MAX_ENVELOPE_POINTS);
      for (uint8_t i = 0; i < layer.pointCount; i++) {
        String point = base + "p" + String(i);
        if (!server.hasArg(point + "a") || !server.hasArg(point + "b")) {
          server.send(400, "application/json", "{\"ok\":false,\"err\":\"Backup missing boundary point values\"}");
          return;
        }
        layer.points[i].a1RawSteps = server.arg(point + "a").toInt();
        layer.points[i].a2RawLimitSteps = server.arg(point + "b").toInt();
      }
    }
  } else {
    // v9.10 JSON import: place directional tables into the requested base Z layer.
    const char* oldPrefix[4] = {"px","pn","nx","nn"};
    const uint8_t oldRegion[4] = {A1_REGION_POSITIVE, A1_REGION_POSITIVE, A1_REGION_NEGATIVE, A1_REGION_NEGATIVE};
    const uint8_t oldBound[4] = {A2_BOUND_MAXIMUM, A2_BOUND_MINIMUM, A2_BOUND_MAXIMUM, A2_BOUND_MINIMUM};
    float zFrom = constrain(MIGRATION_BASE_Z_FROM_MM, zMinMm, zMaxMm);
    float zTo = constrain(MIGRATION_BASE_Z_TO_MM, zMinMm, zMaxMm);
    if (zFrom > zTo) { float temp = zFrom; zFrom = zTo; zTo = temp; }
    for (uint8_t table = 0; table < 4 && importedLayerCount < MAX_ENVELOPE_LAYERS; table++) {
      String prefix = String(oldPrefix[table]);
      String countKey = prefix + "Count";
      uint8_t count = server.hasArg(countKey) ? constrain(server.arg(countKey).toInt(), 0, (int)MAX_ENVELOPE_POINTS) : 0;
      if (count == 0) continue;
      CollisionEnvelopeLayer &layer = importedLayers[importedLayerCount];
      layer.a1Region = oldRegion[table];
      layer.a2Bound = oldBound[table];
      orderRawPair(zMmToSteps(zFrom), zMmToSteps(zTo), layer.zRawMin, layer.zRawMax);
      layer.pointCount = count;
      for (uint8_t i = 0; i < count; i++) {
        String point = prefix + String(i);
        if (!server.hasArg(point + "a") || !server.hasArg(point + "b")) {
          server.send(400, "application/json", "{\"ok\":false,\"err\":\"Backup missing old envelope point values\"}");
          return;
        }
        layer.points[i].a1RawSteps = server.arg(point + "a").toInt();
        layer.points[i].a2RawLimitSteps = server.arg(point + "b").toInt();
      }
      importedLayerCount++;
    }
  }

  a1ZeroSteps = a1z; a1NegativeLimitRawSteps = a1n; a1PositiveLimitRawSteps = a1p;
  a2ZeroSteps = a2z; a2NegativeLimitRawSteps = a2n; a2PositiveLimitRawSteps = a2p; a2HomeClearanceParkRawSteps = a2park;
  zZeroSteps = zz; zUpperLimitRawSteps = zt; zLowerLimitRawSteps = zb; toolTcpLengthMm = tcp;
  manualFkSpeedPct = constrain(server.arg("manualPct").toInt(), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  realtimeSpeedPct = constrain(server.arg("realtimePct").toInt(), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  ikSpeedPct = constrain(server.arg("ikPct").toInt(), MIN_USER_SPEED_PCT, MAX_USER_SPEED_PCT);
  gripperOpenDeg = go; gripperHoldDeg = gh; gripperMaxCloseDeg = gm;

  envelopeLayerCount = importedLayerCount;
  for (uint8_t i = 0; i < importedLayerCount; i++) {
    envelopeLayers[i] = importedLayers[i];
    sortCollisionEnvelopePoints(envelopeLayers[i].points, envelopeLayers[i].pointCount);
  }
  collisionEnvelopeMarginDeg = server.hasArg("envMargin") ? constrain(server.arg("envMargin").toFloat(), 0.0f, 20.0f) : DEFAULT_ENVELOPE_MARGIN_DEG;
  collisionSafeCorridorA2Deg = server.hasArg("envCorridor") ? server.arg("envCorridor").toFloat() : DEFAULT_SAFE_CORRIDOR_A2_DEG;
  collisionProtectionEnabled = server.hasArg("envEnabled") && server.arg("envEnabled") == "true" && envelopeLayerCount > 0;
  startupParkSaved = server.hasArg("parkValid") && server.arg("parkValid") == "true";
  startupParkA1RawSteps = startupParkSaved ? server.arg("parkA1").toInt() : 0L;
  startupParkA2RawSteps = startupParkSaved ? server.arg("parkA2").toInt() : 0L;
  startupParkZRawSteps = startupParkSaved ? server.arg("parkZ").toInt() : 0L;
  startupPoseConfirmed = false;

  saveCalibration(); saveToolSettings(); saveMotionProfiles(); saveGripperSettings(); saveCollisionSettings();
  String ignored; commandGripperOpen(ignored);
  emergencyStop("Calibration backup restored. HOME required.");
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Calibration backup restored. Press HOME.\"}");
}
