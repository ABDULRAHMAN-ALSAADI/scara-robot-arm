/*
  Tab: 09_SerialDebug.ino
  Purpose: Provides Serial Monitor status and basic diagnostic commands.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Serial monitor
// =====================================================

void printStatus() {
  float a1 = actualA1();
  float a2 = actualA2();
  float z = actualZ();
  Position p = forwardKinematics(a1, a2, z);

  Serial.print("A1: "); Serial.print(a1, 2);
  Serial.print(" | A2: "); Serial.print(a2, 2);
  Serial.print(" | Z: "); Serial.print(z, 2);
  Serial.print(" | X: "); Serial.print(p.x, 2);
  Serial.print(" | Y: "); Serial.print(p.y, 2);
  Serial.print(" | Ready: "); Serial.print(isHomed);
  Serial.print(" | SetupRequired: "); Serial.print(setupRequired);
  Serial.print(" | Referenced Z/A1/A2: ");
  Serial.print(zReferenced); Serial.print("/");
  Serial.print(a1Referenced); Serial.print("/");
  Serial.print(a2Referenced);
  Serial.print(" | CollisionProtection: "); Serial.print(collisionProtectionEnabled);
  Serial.print(" | EnvLayers: "); Serial.print(envelopeLayerCount);
  Serial.print(" | Recipes: "); Serial.print(packageRecipeCount);
  Serial.print(" | RecipeState: "); Serial.print(recipeStateName(recipeState));
  Serial.print(" | StartupConfirmed: "); Serial.print(startupPoseConfirmed);
  Serial.print(" | Status: "); Serial.println(faultMessage);
}

void parseCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "CONFIRM_STARTUP") {
    startupPoseConfirmed = true;
    Serial.println("Operator-confirmed Safe Startup Pose for one HOME attempt.");
    return;
  }

  if (cmd == "HOME") {
    homeAll();
    return;
  }

  if (cmd == "WHERE") {
    printStatus();
    return;
  }

  if (cmd == "STOP" || cmd == "OFF") {
    emergencyStop("Serial STOP/OFF command.");
    return;
  }

  if (cmd.startsWith("MOVE ")) {
    float a1, a2, z;
    if (sscanf(cmd.c_str(), "MOVE %f %f %f", &a1, &a2, &z) == 3) {
      String error;
      if (!queueJointMove(a1, a2, z, PROFILE_MANUAL_FK, error)) {
        Serial.println(error);
      }
    }
    return;
  }

  Serial.println("Commands: CONFIRM_STARTUP, HOME, MOVE a1 a2 z, WHERE, STOP, OFF");
}
