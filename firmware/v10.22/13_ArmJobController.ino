/*
  Tab: 13_ArmJobController.ino
  Purpose:
    Autonomous job state machine above the tested Recipe Timeline.

  Manual integration test:
    Load Job -> Wait for AMR -> Simulate AMR Arrived -> recipe runs -> DONE.

  Future integration:
    MQTT replaces the web job commands.
    An IR sensor or AMR message replaces Simulate AMR Arrived.
*/

void copyJobText(char *destination, uint8_t capacity, const String &source, const String &fallback) {
  String clean = "";
  for (uint16_t i = 0; i < source.length() && clean.length() < capacity - 1; i++) {
    char c = source.charAt(i);
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ') clean += c;
  }
  clean.trim();
  if (clean.length() == 0) clean = fallback;
  clean.toCharArray(destination, capacity);
}

const char* armJobStateName(ArmJobState state) {
  switch (state) {
    case ARM_JOB_NOT_HOMED: return "NOT_HOMED";
    case ARM_JOB_READY: return "READY";
    case ARM_JOB_LOADED: return "JOB_LOADED";
    case ARM_JOB_WAITING_FOR_AMR: return "WAITING_FOR_AMR";
    case ARM_JOB_RUNNING: return "RUNNING";
    case ARM_JOB_DONE: return "DONE";
    case ARM_JOB_FAULT: return "FAULT";
    case ARM_JOB_STOPPED: return "STOPPED";
    default: return "UNKNOWN";
  }
}

void clearActiveArmJobRecord() {
  memset(&activeArmJob, 0, sizeof(ActiveArmJob));
  activeArmJob.recipeIndex = -1;
  activeArmJob.amrConfirmed = false;
}

bool armJobHasAssignedJob() {
  return activeArmJob.recipeIndex >= 0 && activeArmJob.recipeIndex < packageRecipeCount;
}

bool armJobLocksManualControl() {
  return armJobState == ARM_JOB_LOADED ||
         armJobState == ARM_JOB_WAITING_FOR_AMR ||
         armJobState == ARM_JOB_RUNNING;
}

String activeArmJobRecipeName() {
  if (!armJobHasAssignedJob()) return "NONE";
  return String(packageRecipes[activeArmJob.recipeIndex].name);
}

void initializeArmJobController() {
  clearActiveArmJobRecord();
  armJobState = ARM_JOB_NOT_HOMED;
  armJobMessage = "HOME required before loading a job.";
  armJobResult = "";
  armJobLastEvent = "No autonomous job loaded.";
}

void armJobOnHomeComplete() {
  clearActiveArmJobRecord();
  armJobState = ARM_JOB_READY;
  armJobMessage = "Arm ready for a job.";
  armJobResult = "";
  armJobLastEvent = "HOME complete. Job Controller READY.";
}

void armJobOnEmergencyStop(const String &reason) {
  if (armJobHasAssignedJob() || armJobState == ARM_JOB_RUNNING ||
      armJobState == ARM_JOB_WAITING_FOR_AMR || armJobState == ARM_JOB_LOADED) {
    armJobState = ARM_JOB_STOPPED;
    armJobResult = "ABORTED";
    armJobMessage = reason + " HOME required.";
    armJobLastEvent = "Job stopped: " + reason;
  } else {
    armJobState = ARM_JOB_NOT_HOMED;
    armJobMessage = reason + " HOME required.";
    armJobResult = "";
    armJobLastEvent = "Robot stopped outside a job.";
  }
}

void armJobOnPowerOff() {
  clearActiveArmJobRecord();
  armJobState = ARM_JOB_NOT_HOMED;
  armJobMessage = "Motors off. HOME required.";
  armJobResult = "";
  armJobLastEvent = "Job controller reset because motors were powered off.";
}

bool validateRecipeForJob(uint8_t recipeIndex, String &error) {
  if (recipeIndex >= packageRecipeCount) {
    error = "Selected recipe does not exist.";
    return false;
  }
  if (packageRecipes[recipeIndex].stepCount == 0) {
    error = "Selected recipe has no timeline steps.";
    return false;
  }
  return true;
}

bool loadArmJob(const String &jobId, const String &packageClass, uint8_t recipeIndex, String &error) {
  if (armJobState != ARM_JOB_READY) {
    error = "Arm is not READY. Clear the previous result or HOME the robot.";
    return false;
  }
  if (!isHomed || isHoming || faultActive) {
    error = "HOME the arm before loading a job.";
    return false;
  }
  if (!collisionProtectionEnabled) {
    error = "Enable collision protection before loading a job.";
    return false;
  }
  if (recipeRunning || anyMotion() || collisionSafeRouteActive) {
    error = "Wait until the arm is idle before loading a job.";
    return false;
  }
  if (!validateRecipeForJob(recipeIndex, error)) return false;

  clearActiveArmJobRecord();
  copyJobText(activeArmJob.jobId, JOB_ID_LENGTH, jobId, "JOB_001");
  copyJobText(activeArmJob.packageClass, PACKAGE_CLASS_LENGTH, packageClass, "UNCLASSIFIED_BOX");
  activeArmJob.recipeIndex = (int8_t)recipeIndex;

  armJobState = ARM_JOB_LOADED;
  armJobMessage = "Job loaded. Arm it to wait for AMR arrival.";
  armJobResult = "";
  armJobLastEvent = "Loaded " + String(activeArmJob.jobId) + " -> " + activeArmJobRecipeName() + ".";
  error = "Job loaded.";
  return true;
}

bool setArmJobWaitingForAmr(String &error) {
  if (armJobState != ARM_JOB_LOADED || !armJobHasAssignedJob()) {
    error = "Load a job before waiting for AMR.";
    return false;
  }
  if (!isHomed || faultActive || !collisionProtectionEnabled) {
    error = "Arm is not safe to wait for AMR.";
    return false;
  }
  armJobState = ARM_JOB_WAITING_FOR_AMR;
  armJobMessage = "Waiting for AMR at pickup station.";
  armJobLastEvent = "Job armed. Waiting for AMR arrival.";
  error = "Waiting for AMR.";
  return true;
}

bool confirmArmJobAmrArrival(String &error) {
  if (armJobState != ARM_JOB_WAITING_FOR_AMR || !armJobHasAssignedJob()) {
    error = "Arm is not waiting for AMR arrival.";
    return false;
  }
  if (!isHomed || faultActive || !collisionProtectionEnabled) {
    error = "Arm is not safe to start the job.";
    return false;
  }

  activeArmJob.amrConfirmed = true;
  String recipeError;
  if (!startPackageRecipe((uint8_t)activeArmJob.recipeIndex, 1, recipeError)) {
    armJobState = ARM_JOB_FAULT;
    armJobResult = "START_REJECTED";
    armJobMessage = recipeError;
    armJobLastEvent = "Job start rejected: " + recipeError;
    error = recipeError;
    return false;
  }

  armJobState = ARM_JOB_RUNNING;
  armJobMessage = "Executing assigned recipe.";
  armJobLastEvent = "AMR arrival confirmed. Recipe started.";
  error = "AMR arrival confirmed. Job running.";
  return true;
}

bool abortArmJob(String &error) {
  if (armJobState == ARM_JOB_RUNNING || recipeRunning) {
    emergencyStop("Autonomous job aborted by operator.");
    error = "Job aborted. HOME required before new motion.";
    return true;
  }

  if (armJobState == ARM_JOB_LOADED || armJobState == ARM_JOB_WAITING_FOR_AMR ||
      armJobState == ARM_JOB_DONE || armJobState == ARM_JOB_FAULT) {
    String oldId = String(activeArmJob.jobId);
    clearActiveArmJobRecord();
    armJobState = (isHomed && !faultActive) ? ARM_JOB_READY : ARM_JOB_NOT_HOMED;
    armJobResult = "CANCELLED";
    armJobMessage = (armJobState == ARM_JOB_READY) ? "Job cleared. Arm ready." : "Job cleared. HOME required.";
    armJobLastEvent = "Cleared job " + oldId + ".";
    error = armJobMessage;
    return true;
  }

  error = "No active job to abort.";
  return false;
}

bool clearArmJobResult(String &error) {
  if (armJobState == ARM_JOB_RUNNING || armJobState == ARM_JOB_WAITING_FOR_AMR ||
      armJobState == ARM_JOB_LOADED) {
    error = "Abort the active job first.";
    return false;
  }
  if (!isHomed || faultActive) {
    error = "HOME the arm before returning to READY.";
    return false;
  }
  clearActiveArmJobRecord();
  armJobState = ARM_JOB_READY;
  armJobMessage = "Arm ready for a job.";
  armJobResult = "";
  armJobLastEvent = "Previous job result cleared.";
  error = "Arm ready.";
  return true;
}

void serviceArmJobController() {
  if (armJobState != ARM_JOB_RUNNING) return;

  if (faultActive) {
    armJobState = ARM_JOB_FAULT;
    armJobResult = "FAILED";
    armJobMessage = faultMessage;
    armJobLastEvent = "Job fault: " + faultMessage;
    return;
  }
  if (!recipeRunning && recipeState == RECIPE_COMPLETE) {
    armJobState = ARM_JOB_DONE;
    armJobResult = "PLACED_SUCCESSFULLY";
    armJobMessage = "Recipe completed. Box placement finished.";
    armJobLastEvent = "Job complete: " + String(activeArmJob.jobId) + ".";
    return;
  }
  if (!recipeRunning && recipeState == RECIPE_FAULT) {
    armJobState = ARM_JOB_FAULT;
    armJobResult = "FAILED";
    armJobMessage = recipeStatusMessage;
    armJobLastEvent = "Job recipe failed.";
  }
}
