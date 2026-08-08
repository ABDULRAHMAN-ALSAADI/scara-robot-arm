/*
  Tab: 11_CollisionSafety.ino
  Purpose: Enforces Z-layered directional collision envelopes.

  Each layer contains:
    A1 region: POSITIVE or NEGATIVE
    A2 boundary: MAXIMUM (A2 <= boundary) or MINIMUM (A2 >= boundary)
    Z active range
    Boundary points: A1 -> allowed A2 limit

  Layers are stored in raw motor coordinates so physical collision maps remain
  aligned when display coordinates are later corrected.
*/

bool layerUsesPositiveA1(const CollisionEnvelopeLayer &layer) {
  return layer.a1Region == A1_REGION_POSITIVE;
}

bool layerUsesMaximumA2(const CollisionEnvelopeLayer &layer) {
  return layer.a2Bound == A2_BOUND_MAXIMUM;
}

String layerLabel(const CollisionEnvelopeLayer &layer) {
  String label = layerUsesPositiveA1(layer) ? "Positive A1 / " : "Negative A1 / ";
  label += layerUsesMaximumA2(layer) ? "Block A2 Above" : "Block A2 Below";
  return label;
}

void orderRawPair(long a, long b, long &low, long &high) {
  low = a < b ? a : b;
  high = a < b ? b : a;
}

bool rawInside(long value, long low, long high) {
  return value >= low && value <= high;
}

bool rawRangesOverlap(long lowA, long highA, long lowB, long highB) {
  return lowA <= highB && highA >= lowB;
}

float clampedZForMigration(float value) {
  return constrain(value, zMinMm, zMaxMm);
}

void sortCollisionEnvelopePoints(CollisionEnvelopePoint points[], uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    for (uint8_t j = i + 1; j < count; j++) {
      if (a1StepsToAngle(points[j].a1RawSteps) < a1StepsToAngle(points[i].a1RawSteps)) {
        CollisionEnvelopePoint temp = points[i];
        points[i] = points[j];
        points[j] = temp;
      }
    }
  }
}

bool assignLayerDefinition(CollisionEnvelopeLayer &layer, uint8_t a1Region, uint8_t a2Bound,
                           float zFromMm, float zToMm, String &error) {
  if (a1Region > A1_REGION_NEGATIVE || a2Bound > A2_BOUND_MINIMUM) {
    error = "Unknown envelope layer type.";
    return false;
  }
  if (zFromMm > zToMm) { float temp = zFromMm; zFromMm = zToMm; zToMm = temp; }
  if (zFromMm < zMinMm - 0.001f || zToMm > zMaxMm + 0.001f) {
    error = "Z layer range must remain inside calibrated Z endpoints.";
    return false;
  }
  if (zToMm - zFromMm < 0.01f) {
    error = "Z layer needs a non-zero range.";
    return false;
  }
  layer.a1Region = a1Region;
  layer.a2Bound = a2Bound;
  orderRawPair(zMmToSteps(zFromMm), zMmToSteps(zToMm), layer.zRawMin, layer.zRawMax);
  layer.pointCount = 0;
  error = "";
  return true;
}

void migrateDirectionalV10IntoZLayers() {
  legacyCollisionStore.begin("scara_env2", true);

  startupParkSaved = legacyCollisionStore.getBool("parkvalid", false);
  startupParkA1RawSteps = legacyCollisionStore.getLong("park_a1", 0L);
  startupParkA2RawSteps = legacyCollisionStore.getLong("park_a2", 0L);
  startupParkZRawSteps = legacyCollisionStore.getLong("park_z", 0L);
  collisionEnvelopeMarginDeg = constrain(legacyCollisionStore.getFloat("margin", DEFAULT_ENVELOPE_MARGIN_DEG), 0.0f, 20.0f);
  collisionSafeCorridorA2Deg = legacyCollisionStore.getFloat("corridor", DEFAULT_SAFE_CORRIDOR_A2_DEG);

  const char* oldPrefix[4] = {"px", "pn", "nx", "nn"};
  const uint8_t oldRegion[4] = {A1_REGION_POSITIVE, A1_REGION_POSITIVE, A1_REGION_NEGATIVE, A1_REGION_NEGATIVE};
  const uint8_t oldBound[4] = {A2_BOUND_MAXIMUM, A2_BOUND_MINIMUM, A2_BOUND_MAXIMUM, A2_BOUND_MINIMUM};

  float migrationZFrom = clampedZForMigration(MIGRATION_BASE_Z_FROM_MM);
  float migrationZTo = clampedZForMigration(MIGRATION_BASE_Z_TO_MM);
  if (migrationZFrom > migrationZTo) { float temp = migrationZFrom; migrationZFrom = migrationZTo; migrationZTo = temp; }
  if (fabs(migrationZTo - migrationZFrom) < 0.01f) {
    migrationZFrom = zMinMm;
    migrationZTo = zMaxMm;
  }

  envelopeLayerCount = 0;
  for (uint8_t table = 0; table < 4 && envelopeLayerCount < MAX_ENVELOPE_LAYERS; table++) {
    String prefix = String(oldPrefix[table]);
    uint8_t count = constrain((int)legacyCollisionStore.getUChar((prefix + "c").c_str(), 0), 0, (int)MAX_ENVELOPE_POINTS);
    if (count == 0) continue;

    CollisionEnvelopeLayer &layer = envelopeLayers[envelopeLayerCount];
    String ignored;
    assignLayerDefinition(layer, oldRegion[table], oldBound[table], migrationZFrom, migrationZTo, ignored);
    layer.pointCount = count;
    for (uint8_t i = 0; i < count; i++) {
      String base = prefix + String(i);
      layer.points[i].a1RawSteps = legacyCollisionStore.getLong((base + "a").c_str(), 0L);
      layer.points[i].a2RawLimitSteps = legacyCollisionStore.getLong((base + "b").c_str(), 0L);
    }
    sortCollisionEnvelopePoints(layer.points, layer.pointCount);
    envelopeLayerCount++;
  }

  legacyCollisionStore.end();
  collisionProtectionEnabled = false;
}

void loadCollisionSettings() {
  collisionStore.begin("scara_env3", false);
  if (!collisionStore.getBool("init", false)) {
    migrateDirectionalV10IntoZLayers();
    saveCollisionSettings();
    return;
  }

  collisionProtectionEnabled = collisionStore.getBool("enabled", false);
  collisionEnvelopeMarginDeg = constrain(collisionStore.getFloat("margin", DEFAULT_ENVELOPE_MARGIN_DEG), 0.0f, 20.0f);
  collisionSafeCorridorA2Deg = collisionStore.getFloat("corridor", DEFAULT_SAFE_CORRIDOR_A2_DEG);
  startupParkSaved = collisionStore.getBool("parkvalid", false);
  startupParkA1RawSteps = collisionStore.getLong("park_a1", 0L);
  startupParkA2RawSteps = collisionStore.getLong("park_a2", 0L);
  startupParkZRawSteps = collisionStore.getLong("park_z", 0L);

  envelopeLayerCount = constrain((int)collisionStore.getUChar("lcount", 0), 0, (int)MAX_ENVELOPE_LAYERS);
  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    String prefix = "l" + String(layerIndex);
    CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    layer.a1Region = collisionStore.getUChar((prefix + "r").c_str(), A1_REGION_POSITIVE);
    layer.a2Bound = collisionStore.getUChar((prefix + "b").c_str(), A2_BOUND_MAXIMUM);
    layer.zRawMin = collisionStore.getLong((prefix + "zl").c_str(), zMmToSteps(zMinMm));
    layer.zRawMax = collisionStore.getLong((prefix + "zh").c_str(), zMmToSteps(zMaxMm));
    layer.pointCount = constrain((int)collisionStore.getUChar((prefix + "c").c_str(), 0), 0, (int)MAX_ENVELOPE_POINTS);
    for (uint8_t i = 0; i < layer.pointCount; i++) {
      String point = prefix + "p" + String(i);
      layer.points[i].a1RawSteps = collisionStore.getLong((point + "a").c_str(), 0L);
      layer.points[i].a2RawLimitSteps = collisionStore.getLong((point + "b").c_str(), 0L);
    }
    sortCollisionEnvelopePoints(layer.points, layer.pointCount);
  }
  if (envelopeLayerCount == 0) collisionProtectionEnabled = false;
}

void saveCollisionSettings() {
  collisionStore.putBool("init", true);
  collisionStore.putBool("enabled", collisionProtectionEnabled);
  collisionStore.putFloat("margin", collisionEnvelopeMarginDeg);
  collisionStore.putFloat("corridor", collisionSafeCorridorA2Deg);
  collisionStore.putBool("parkvalid", startupParkSaved);
  collisionStore.putLong("park_a1", startupParkA1RawSteps);
  collisionStore.putLong("park_a2", startupParkA2RawSteps);
  collisionStore.putLong("park_z", startupParkZRawSteps);
  collisionStore.putUChar("lcount", envelopeLayerCount);

  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    String prefix = "l" + String(layerIndex);
    CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    collisionStore.putUChar((prefix + "r").c_str(), layer.a1Region);
    collisionStore.putUChar((prefix + "b").c_str(), layer.a2Bound);
    collisionStore.putLong((prefix + "zl").c_str(), layer.zRawMin);
    collisionStore.putLong((prefix + "zh").c_str(), layer.zRawMax);
    collisionStore.putUChar((prefix + "c").c_str(), layer.pointCount);
    for (uint8_t i = 0; i < layer.pointCount; i++) {
      String point = prefix + "p" + String(i);
      collisionStore.putLong((point + "a").c_str(), layer.points[i].a1RawSteps);
      collisionStore.putLong((point + "b").c_str(), layer.points[i].a2RawLimitSteps);
    }
  }
}

void resetCollisionSettings() {
  envelopeLayerCount = 0;
  collisionProtectionEnabled = false;
  collisionEnvelopeMarginDeg = DEFAULT_ENVELOPE_MARGIN_DEG;
  collisionSafeCorridorA2Deg = DEFAULT_SAFE_CORRIDOR_A2_DEG;
  startupParkSaved = false;
  startupParkA1RawSteps = 0L;
  startupParkA2RawSteps = 0L;
  startupParkZRawSteps = 0L;
  startupPoseConfirmed = false;
  saveCollisionSettings();
}

bool createCollisionEnvelopeLayer(uint8_t a1Region, uint8_t a2Bound,
                                  float zFromMm, float zToMm,
                                  uint8_t &createdIndex, String &error) {
  if (envelopeLayerCount >= MAX_ENVELOPE_LAYERS) {
    error = "Maximum number of Z envelope layers reached.";
    return false;
  }
  CollisionEnvelopeLayer layer;
  if (!assignLayerDefinition(layer, a1Region, a2Bound, zFromMm, zToMm, error)) return false;
  envelopeLayers[envelopeLayerCount] = layer;
  createdIndex = envelopeLayerCount;
  envelopeLayerCount++;
  saveCollisionSettings();
  error = "Z envelope layer created.";
  return true;
}

bool deleteCollisionEnvelopeLayer(uint8_t layerIndex, String &error) {
  if (layerIndex >= envelopeLayerCount) {
    error = "Layer does not exist.";
    return false;
  }
  for (uint8_t i = layerIndex; i + 1 < envelopeLayerCount; i++) {
    envelopeLayers[i] = envelopeLayers[i + 1];
  }
  envelopeLayerCount--;
  if (envelopeLayerCount == 0) collisionProtectionEnabled = false;
  saveCollisionSettings();
  error = "Envelope layer deleted.";
  return true;
}

bool addCollisionEnvelopePoint(uint8_t layerIndex, float a1Deg, float a2LimitDeg, String &error) {
  if (layerIndex >= envelopeLayerCount) {
    error = "Select a Z envelope layer first.";
    return false;
  }
  CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
  if (a1Deg < a1MinDeg || a1Deg > a1MaxDeg || a2LimitDeg < a2MinDeg || a2LimitDeg > a2MaxDeg) {
    error = "Boundary point must be inside calibrated A1/A2 endpoints.";
    return false;
  }
  if (layerUsesPositiveA1(layer) && a1Deg < -0.001f) {
    error = "This layer accepts only positive A1 boundary positions.";
    return false;
  }
  if (!layerUsesPositiveA1(layer) && a1Deg > 0.001f) {
    error = "This layer accepts only negative A1 boundary positions.";
    return false;
  }

  const long a1Raw = a1AngleToSteps(a1Deg);
  const long a2Raw = a2AngleToSteps(a2LimitDeg);
  for (uint8_t i = 0; i < layer.pointCount; i++) {
    if (fabs(a1StepsToAngle(layer.points[i].a1RawSteps) - a1Deg) <= 0.25f) {
      layer.points[i].a1RawSteps = a1Raw;
      layer.points[i].a2RawLimitSteps = a2Raw;
      sortCollisionEnvelopePoints(layer.points, layer.pointCount);
      saveCollisionSettings();
      error = "Boundary point updated in selected Z layer.";
      return true;
    }
  }

  if (layer.pointCount >= MAX_ENVELOPE_POINTS) {
    error = "Selected Z envelope layer is full.";
    return false;
  }
  layer.points[layer.pointCount].a1RawSteps = a1Raw;
  layer.points[layer.pointCount].a2RawLimitSteps = a2Raw;
  layer.pointCount++;
  sortCollisionEnvelopePoints(layer.points, layer.pointCount);
  saveCollisionSettings();
  error = "Boundary point saved in selected Z layer.";
  return true;
}

bool captureCurrentCollisionEnvelopePoint(uint8_t layerIndex, String &error) {
  if (!isHomed || isHoming || anyMotion() || faultActive) {
    error = "HOME the robot and wait until idle before capturing a point.";
    return false;
  }
  return addCollisionEnvelopePoint(layerIndex, actualA1(), actualA2(), error);
}

bool deleteCollisionEnvelopePoint(uint8_t layerIndex, uint8_t pointIndex, String &error) {
  if (layerIndex >= envelopeLayerCount || pointIndex >= envelopeLayers[layerIndex].pointCount) {
    error = "Boundary point does not exist.";
    return false;
  }
  CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
  for (uint8_t i = pointIndex; i + 1 < layer.pointCount; i++) layer.points[i] = layer.points[i + 1];
  layer.pointCount--;
  saveCollisionSettings();
  error = "Boundary point deleted.";
  return true;
}

bool clearCollisionEnvelopeLayer(uint8_t layerIndex, String &error) {
  if (layerIndex >= envelopeLayerCount) {
    error = "Select an envelope layer first.";
    return false;
  }
  envelopeLayers[layerIndex].pointCount = 0;
  saveCollisionSettings();
  error = "Boundary points cleared from selected Z layer.";
  return true;
}

bool setCollisionEnvelopeMargin(float marginDeg, String &error) {
  if (marginDeg < 0.0f || marginDeg > 20.0f) {
    error = "Extra margin must be from 0 to 20 degrees.";
    return false;
  }
  collisionEnvelopeMarginDeg = marginDeg;
  saveCollisionSettings();
  error = "Envelope margin saved.";
  return true;
}

bool setCollisionSafeCorridor(float corridorDeg, String &error) {
  if (corridorDeg < a2MinDeg || corridorDeg > a2MaxDeg) {
    error = "Safe corridor must remain inside calibrated A2 endpoints.";
    return false;
  }
  collisionSafeCorridorA2Deg = corridorDeg;
  saveCollisionSettings();
  error = "Safe-route A2 corridor saved.";
  return true;
}

bool layerActiveAtZ(const CollisionEnvelopeLayer &layer, float zMm) {
  return rawInside(zMmToSteps(zMm), layer.zRawMin, layer.zRawMax);
}

bool layerOverlapsZMove(const CollisionEnvelopeLayer &layer, float startZMm, float targetZMm) {
  long low, high;
  orderRawPair(zMmToSteps(startZMm), zMmToSteps(targetZMm), low, high);
  return rawRangesOverlap(low, high, layer.zRawMin, layer.zRawMax);
}

bool interpolateLayerAtA1(const CollisionEnvelopeLayer &layer, float a1Deg, float &limitDeg) {
  const uint8_t count = layer.pointCount;
  if (count == 0) return false;

  const bool positive = layerUsesPositiveA1(layer);
  const bool maximum = layerUsesMaximumA2(layer);
  const float firstA1 = a1StepsToAngle(layer.points[0].a1RawSteps);
  const float lastA1 = a1StepsToAngle(layer.points[count - 1].a1RawSteps);

  if (positive && a1Deg < firstA1 - 0.001f) return false;
  if (!positive && a1Deg > lastA1 + 0.001f) return false;

  if (count == 1 || (positive && a1Deg >= lastA1) || (!positive && a1Deg <= firstA1)) {
    const uint8_t edge = positive ? count - 1 : 0;
    limitDeg = a2StepsToAngle(layer.points[edge].a2RawLimitSteps);
  } else {
    bool found = false;
    for (uint8_t i = 0; i + 1 < count; i++) {
      const float a1A = a1StepsToAngle(layer.points[i].a1RawSteps);
      const float a1B = a1StepsToAngle(layer.points[i + 1].a1RawSteps);
      if (a1Deg >= a1A - 0.001f && a1Deg <= a1B + 0.001f) {
        const float a2A = a2StepsToAngle(layer.points[i].a2RawLimitSteps);
        const float a2B = a2StepsToAngle(layer.points[i + 1].a2RawLimitSteps);
        const float f = fabs(a1B - a1A) < 0.001f ? 0.0f : (a1Deg - a1A) / (a1B - a1A);
        limitDeg = a2A + f * (a2B - a2A);
        found = true;
        break;
      }
    }
    if (!found) return false;
  }

  if (maximum) limitDeg -= collisionEnvelopeMarginDeg;
  else limitDeg += collisionEnvelopeMarginDeg;
  return true;
}

bool envelopeLimitAtPose(uint8_t layerIndex, float a1Deg, float zMm, float &limitDeg) {
  if (layerIndex >= envelopeLayerCount) return false;
  const CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
  if (!layerActiveAtZ(layer, zMm)) return false;
  return interpolateLayerAtA1(layer, a1Deg, limitDeg);
}

bool setCollisionProtection(bool enabled, String &error) {
  uint16_t points = 0;
  for (uint8_t i = 0; i < envelopeLayerCount; i++) points += envelopeLayers[i].pointCount;
  if (enabled && points == 0) {
    error = "Save at least one boundary point first.";
    return false;
  }
  if (enabled) {
    bool previous = collisionProtectionEnabled;
    collisionProtectionEnabled = true;
    String poseError;
    bool safe = collisionPoseIsSafe(actualA1(), actualA2(), actualZ(), gripperHoldDeg, poseError);
    collisionProtectionEnabled = previous;
    if (!safe) {
      error = "Current pose violates an active Z-layer envelope. Move to a permitted pose first. " + poseError;
      return false;
    }
  }
  collisionProtectionEnabled = enabled;
  saveCollisionSettings();
  error = enabled ? "Z-layered collision protection enabled." : "Collision protection disabled.";
  return true;
}

bool saveCurrentStartupPark(String &error) {
  if (!isHomed || isHoming || anyMotion() || faultActive) {
    error = "HOME and wait until idle before saving the startup park pose.";
    return false;
  }
  if (gripperCommandDeg != gripperOpenDeg) {
    error = "Press OPEN before saving the startup park pose.";
    return false;
  }
  startupParkA1RawSteps = arm1.currentPosition();
  startupParkA2RawSteps = arm2.currentPosition();
  startupParkZRawSteps = zAxis.currentPosition();
  startupParkSaved = true;
  saveCollisionSettings();
  error = "Safe Startup / Power-Off Park pose saved.";
  return true;
}

bool collisionPoseIsSafe(float a1Deg, float a2Deg, float zMm, int gripperDeg, String &error) {
  (void)gripperDeg; // Current envelope is conservative for the held-box condition.
  if (!collisionProtectionEnabled) return true;

  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    const CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    float limit;
    if (!envelopeLimitAtPose(layerIndex, a1Deg, zMm, limit)) continue;

    if (layerUsesMaximumA2(layer) && a2Deg > limit + 0.001f) {
      error = "Layer #" + String(layerIndex + 1) + " " + layerLabel(layer) +
        ": at Z " + String(zMm, 2) + " mm and A1 " + String(a1Deg, 2) +
        " deg, A2 must be <= " + String(limit, 2) + " deg.";
      return false;
    }
    if (!layerUsesMaximumA2(layer) && a2Deg < limit - 0.001f) {
      error = "Layer #" + String(layerIndex + 1) + " " + layerLabel(layer) +
        ": at Z " + String(zMm, 2) + " mm and A1 " + String(a1Deg, 2) +
        " deg, A2 must be >= " + String(limit, 2) + " deg.";
      return false;
    }
  }
  return true;
}

bool tightestLayerLimitForA1Sweep(const CollisionEnvelopeLayer &layer,
                                  float a1Low, float a1High, float &tightest) {
  bool found = false;
  float value;
  const bool maximum = layerUsesMaximumA2(layer);

  if (interpolateLayerAtA1(layer, a1Low, value)) { tightest = value; found = true; }
  if (interpolateLayerAtA1(layer, a1High, value)) {
    tightest = found ? (maximum ? fmin(tightest, value) : fmax(tightest, value)) : value;
    found = true;
  }
  for (uint8_t i = 0; i < layer.pointCount; i++) {
    float knot = a1StepsToAngle(layer.points[i].a1RawSteps);
    if (knot >= a1Low && knot <= a1High && interpolateLayerAtA1(layer, knot, value)) {
      tightest = found ? (maximum ? fmin(tightest, value) : fmax(tightest, value)) : value;
      found = true;
    }
  }
  return found;
}

bool collisionPathIsSafe(float startA1Deg, float startA2Deg, float startZMm,
                         float targetA1Deg, float targetA2Deg, float targetZMm,
                         int gripperDeg, String &error) {
  (void)gripperDeg;
  if (!collisionProtectionEnabled) return true;

  const float a1Low = fmin(startA1Deg, targetA1Deg);
  const float a1High = fmax(startA1Deg, targetA1Deg);
  const float a2Low = fmin(startA2Deg, targetA2Deg);
  const float a2High = fmax(startA2Deg, targetA2Deg);

  for (uint8_t layerIndex = 0; layerIndex < envelopeLayerCount; layerIndex++) {
    const CollisionEnvelopeLayer &layer = envelopeLayers[layerIndex];
    if (!layerOverlapsZMove(layer, startZMm, targetZMm)) continue;

    float limit;
    if (!tightestLayerLimitForA1Sweep(layer, a1Low, a1High, limit)) continue;

    if (layerUsesMaximumA2(layer) && a2High > limit + 0.001f) {
      error = "Move crosses layer #" + String(layerIndex + 1) + " " + layerLabel(layer) +
              " while Z path overlaps its active range; A2 limit <= " + String(limit, 2) + " deg.";
      return false;
    }
    if (!layerUsesMaximumA2(layer) && a2Low < limit - 0.001f) {
      error = "Move crosses layer #" + String(layerIndex + 1) + " " + layerLabel(layer) +
              " while Z path overlaps its active range; A2 limit >= " + String(limit, 2) + " deg.";
      return false;
    }
  }
  return true;
}
