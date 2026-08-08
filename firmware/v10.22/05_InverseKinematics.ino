/*
  Tab: 05_InverseKinematics.ino
  Purpose: Calculates safe Elbow-Up / Elbow-Down joint solutions from a requested X, Y position.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Inverse Kinematics
// =====================================================

IKSolution solveIK(float x, float y, bool elbowDownConfiguration) {
  IKSolution result;
  result.ok = false;
  result.a1 = 0.0f;
  result.a2 = 0.0f;
  result.error = "";

  float distanceSquared = x * x + y * y;
  float distance = sqrt(distanceSquared);
  float maxReach = L1 + toolTcpLengthMm;
  float minReach = fabs(L1 - toolTcpLengthMm);

  if (distance > maxReach + 0.01f || distance < minReach - 0.01f) {
    result.error = "Target is outside XY reach.";
    return result;
  }

  float cosA2 = (distanceSquared - L1 * L1 - toolTcpLengthMm * toolTcpLengthMm) / (2.0f * L1 * toolTcpLengthMm);
  cosA2 = constrain(cosA2, -1.0f, 1.0f);

  float sinMagnitude = sqrt(fmax(0.0f, 1.0f - cosA2 * cosA2));
  // With the robot coordinate convention used by FK:
  //   theta2 > 0 is the Elbow-Down configuration.
  //   theta2 < 0 is the Elbow-Up configuration.
  float sinA2 = elbowDownConfiguration ? sinMagnitude : -sinMagnitude;

  float a2Rad = atan2(sinA2, cosA2);
  float a1Rad = atan2(y, x) - atan2(toolTcpLengthMm * sinA2, L1 + toolTcpLengthMm * cosA2);

  float a2Deg = degrees(a2Rad);
  float baseA1 = normalizeAngle(degrees(a1Rad));

  // At the ±180 wrap boundary, the same XY orientation can be represented
  // by -180 or +180. Select the safe equivalent nearest to the current posture.
  float candidates[3] = { baseA1 - 360.0f, baseA1, baseA1 + 360.0f };
  bool foundA1 = false;
  float selectedA1 = 0.0f;
  float bestDistance = 1.0e9f;
  float currentA1 = actualA1();

  for (int i = 0; i < 3; i++) {
    float c = candidates[i];
    if (c >= a1MinDeg - IK_ROUNDING_JOINT_TOL_DEG && c <= a1MaxDeg + IK_ROUNDING_JOINT_TOL_DEG) {
      c = constrain(c, a1MinDeg, a1MaxDeg);
      float delta = fabs(c - currentA1);

      // In a tie, prefer the positive-side equivalent because A1 negative
      // side is the switch side and is deliberately restricted.
      if (!foundA1 || delta < bestDistance - 0.001f ||
          (fabs(delta - bestDistance) < 0.001f && c > selectedA1)) {
        selectedA1 = c;
        bestDistance = delta;
        foundA1 = true;
      }
    }
  }

  if (!foundA1) {
    result.error = "IK needs A1 outside safe limits.";
    return result;
  }

  if (a2Deg < a2MinDeg - IK_ROUNDING_JOINT_TOL_DEG || a2Deg > a2MaxDeg + IK_ROUNDING_JOINT_TOL_DEG) {
    result.error = "IK needs A2 outside safe limits.";
    return result;
  }

  a2Deg = constrain(a2Deg, a2MinDeg, a2MaxDeg);

  Position verify = forwardKinematics(selectedA1, a2Deg, 0.0f);
  float error = sqrt((verify.x - x) * (verify.x - x) + (verify.y - y) * (verify.y - y));

  if (error > IK_MAX_CLAMP_POSITION_ERROR_MM) {
    result.error = "IK verification failed.";
    return result;
  }

  result.ok = true;
  result.a1 = selectedA1;
  result.a2 = a2Deg;
  result.error = "OK";
  return result;
}

IKSolution selectNearestIKConfiguration(float x, float y) {
  IKSolution elbowDown = solveIK(x, y, true);
  IKSolution elbowUp   = solveIK(x, y, false);

  if (elbowDown.ok && !elbowUp.ok) return elbowDown;
  if (elbowUp.ok && !elbowDown.ok) return elbowUp;

  if (!elbowDown.ok && !elbowUp.ok) {
    IKSolution failed;
    failed.ok = false;
    failed.a1 = 0.0f;
    failed.a2 = 0.0f;
    failed.error = "Both elbow configurations are outside safe joint limits.";
    return failed;
  }

  float currentA1 = actualA1();
  float currentA2 = actualA2();

  float downDistance = fabs(elbowDown.a1 - currentA1) + fabs(elbowDown.a2 - currentA2);
  float upDistance   = fabs(elbowUp.a1 - currentA1)   + fabs(elbowUp.a2 - currentA2);

  return (upDistance < downDistance) ? elbowUp : elbowDown;
}
