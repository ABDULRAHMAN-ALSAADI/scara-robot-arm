/*
  Tab: 04_ForwardKinematics.ino
  Purpose: Calculates end-effector X, Y, Z from the current joint angles and TCP length.

  Keep edits in this tab focused on this subsystem. Public function
  declarations and shared hardware/state definitions are in the main tab.
*/

// =====================================================
// Forward Kinematics
// =====================================================

Position forwardKinematics(float a1Deg, float a2Deg, float zMM) {
  float a1 = radians(a1Deg);
  float a2 = radians(a2Deg);

  Position p;
  p.x = L1 * cos(a1) + toolTcpLengthMm * cos(a1 + a2);
  p.y = L1 * sin(a1) + toolTcpLengthMm * sin(a1 + a2);
  p.z = zMM;
  return p;
}

float normalizeAngle(float angleDeg) {
  while (angleDeg > 180.0f) angleDeg -= 360.0f;
  while (angleDeg < -180.0f) angleDeg += 360.0f;
  return angleDeg;
}
