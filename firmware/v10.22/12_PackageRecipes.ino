/*
  Tab: 12_PackageRecipes.ino
  Purpose:
    Explicit command-timeline recipes for classified package handling.

  Step execution:
    MOVE_SAFE_XYZ   Uses queueJointMove(), exactly like FK.
    MOVE_A1         Moves only A1.
    MOVE_A2         Moves only A2.
    MOVE_Z          Moves only Z.
    MOVE_A1_A2      Moves A1 and A2 together while Z stays fixed.
    MOVE_XYZ_DIRECT Moves all axes directly together, without automatic reroute.
    GRIPPER         Commands the servo only.
    WAIT            Waits only.

  Each movement step also stores a gripper target. Movement completes first,
  then that stored gripper value is commanded, then the pause-after timer runs.
  This permits a same-position step with a changed gripper value to grip/drop.
*/

void copyRecipeText(char *destination, uint8_t capacity, const String &source, const String &fallback) {
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

const char* recipeStateName(RecipePlaybackState state) {
  switch (state) {
    case RECIPE_IDLE: return "IDLE";
    case RECIPE_START_STEP: return "RUNNING";
    case RECIPE_WAIT_FOR_MOTION: return "MOVING";
    case RECIPE_APPLY_STEP_GRIPPER: return "SETTING_GRIPPER";
    case RECIPE_WAIT_FOR_TIMER: return "WAITING";
    case RECIPE_COMPLETE: return "COMPLETE";
    case RECIPE_FAULT: return "FAULT";
    default: return "UNKNOWN";
  }
}

const char* recipeCommandName(uint8_t command) {
  switch (command) {
    case RECIPE_CMD_MOVE_SAFE_XYZ: return "MOVE_SAFE_XYZ";
    case RECIPE_CMD_MOVE_A1_ONLY: return "MOVE_A1";
    case RECIPE_CMD_MOVE_A2_ONLY: return "MOVE_A2";
    case RECIPE_CMD_MOVE_Z_ONLY: return "MOVE_Z";
    case RECIPE_CMD_MOVE_A1_A2_TOGETHER: return "MOVE_A1_A2";
    case RECIPE_CMD_MOVE_XYZ_DIRECT: return "MOVE_XYZ_DIRECT";
    case RECIPE_CMD_SET_GRIPPER: return "GRIPPER";
    case RECIPE_CMD_WAIT: return "WAIT";
    default: return "UNKNOWN";
  }
}

const char* recipeCommandDisplayName(uint8_t command) {
  switch (command) {
    case RECIPE_CMD_MOVE_SAFE_XYZ: return "Move Safe A1/A2/Z";
    case RECIPE_CMD_MOVE_A1_ONLY: return "Move A1 Only";
    case RECIPE_CMD_MOVE_A2_ONLY: return "Move A2 Only";
    case RECIPE_CMD_MOVE_Z_ONLY: return "Move Z Only";
    case RECIPE_CMD_MOVE_A1_A2_TOGETHER: return "Move A1+A2 Together";
    case RECIPE_CMD_MOVE_XYZ_DIRECT: return "Move A1+A2+Z Direct";
    case RECIPE_CMD_SET_GRIPPER: return "Set Gripper";
    case RECIPE_CMD_WAIT: return "Wait";
    default: return "Unknown";
  }
}

uint8_t parseRecipeCommand(const String &value) {
  if (value == "MOVE_A1") return RECIPE_CMD_MOVE_A1_ONLY;
  if (value == "MOVE_A2") return RECIPE_CMD_MOVE_A2_ONLY;
  if (value == "MOVE_Z") return RECIPE_CMD_MOVE_Z_ONLY;
  if (value == "MOVE_A1_A2") return RECIPE_CMD_MOVE_A1_A2_TOGETHER;
  if (value == "MOVE_XYZ_DIRECT") return RECIPE_CMD_MOVE_XYZ_DIRECT;
  if (value == "GRIPPER") return RECIPE_CMD_SET_GRIPPER;
  if (value == "WAIT") return RECIPE_CMD_WAIT;
  return RECIPE_CMD_MOVE_SAFE_XYZ;
}

String activePackageRecipeName() {
  if (recipeActiveIndex < 0 || recipeActiveIndex >= packageRecipeCount) return "NONE";
  return String(packageRecipes[recipeActiveIndex].name);
}

bool validPackageRecipeIndex(int index) {
  return index >= 0 && index < packageRecipeCount;
}

bool validRecipeStepIndex(int recipeIndex, int stepIndex) {
  return validPackageRecipeIndex(recipeIndex) &&
         stepIndex >= 0 && stepIndex < packageRecipes[recipeIndex].stepCount;
}

void clearRecipeStep(RecipeStep &step) {
  memset(&step, 0, sizeof(RecipeStep));
  step.command = RECIPE_CMD_MOVE_SAFE_XYZ;
  step.gripperDeg = gripperOpenDeg;
}

void setRecipeStep(RecipeStep &step, const String &name, uint8_t command,
                   float a1, float a2, float z, uint8_t gripper,
                   uint16_t waitMs, uint8_t fallbackNumber) {
  clearRecipeStep(step);
  copyRecipeText(step.name, RECIPE_STEP_NAME_LENGTH, name, "STEP_" + String(fallbackNumber));
  step.command = command;
  step.a1RawSteps = a1AngleToSteps(a1);
  step.a2RawSteps = a2AngleToSteps(a2);
  step.zRawSteps = zMmToSteps(z);
  step.gripperDeg = gripper;
  step.waitAfterMs = waitMs;
}

bool commandUsesA1(uint8_t command) {
  return command == RECIPE_CMD_MOVE_SAFE_XYZ || command == RECIPE_CMD_MOVE_A1_ONLY ||
         command == RECIPE_CMD_MOVE_A1_A2_TOGETHER || command == RECIPE_CMD_MOVE_XYZ_DIRECT;
}
bool commandUsesA2(uint8_t command) {
  return command == RECIPE_CMD_MOVE_SAFE_XYZ || command == RECIPE_CMD_MOVE_A2_ONLY ||
         command == RECIPE_CMD_MOVE_A1_A2_TOGETHER || command == RECIPE_CMD_MOVE_XYZ_DIRECT;
}
bool commandUsesZ(uint8_t command) {
  return command == RECIPE_CMD_MOVE_SAFE_XYZ || command == RECIPE_CMD_MOVE_Z_ONLY ||
         command == RECIPE_CMD_MOVE_XYZ_DIRECT;
}

// v10.22.3 legacy recipe compatibility:
// Recipes made in v10.9 may contain poses outside the current stored calibration
// because calibration values changed between versions.
// For stored recipes only, we use a legacy compatibility window.
// Manual FK/IK/JOG still uses the calibrated limits.
constexpr float RECIPE_LEGACY_A1_MIN_DEG = -180.0f;
constexpr float RECIPE_LEGACY_A1_MAX_DEG =  180.0f;
constexpr float RECIPE_LEGACY_A2_MIN_DEG = -155.0f;
constexpr float RECIPE_LEGACY_A2_MAX_DEG =  170.0f;
constexpr float RECIPE_LEGACY_Z_MIN_MM   = -160.0f;
constexpr float RECIPE_LEGACY_Z_MAX_MM   =   30.0f;

float recipeEffectiveA1MinDeg() { return fmin(a1MinDeg, RECIPE_LEGACY_A1_MIN_DEG); }
float recipeEffectiveA1MaxDeg() { return fmax(a1MaxDeg, RECIPE_LEGACY_A1_MAX_DEG); }
float recipeEffectiveA2MinDeg() { return fmin(a2MinDeg, RECIPE_LEGACY_A2_MIN_DEG); }
float recipeEffectiveA2MaxDeg() { return fmax(a2MaxDeg, RECIPE_LEGACY_A2_MAX_DEG); }
float recipeEffectiveZMinMm()   { return fmin(zMinMm, RECIPE_LEGACY_Z_MIN_MM); }
float recipeEffectiveZMaxMm()   { return fmax(zMaxMm, RECIPE_LEGACY_Z_MAX_MM); }

bool validateRecipeStepDefinition(const RecipeStep &step, String &error) {
  if (step.command > RECIPE_CMD_WAIT) { error = "Unknown recipe command."; return false; }
  float a1 = a1StepsToAngle(step.a1RawSteps);
  float a2 = a2StepsToAngle(step.a2RawSteps);
  float z = zStepsToMm(step.zRawSteps);

  const float allowedA1Min = recipeEffectiveA1MinDeg();
  const float allowedA1Max = recipeEffectiveA1MaxDeg();
  const float allowedA2Min = recipeEffectiveA2MinDeg();
  const float allowedA2Max = recipeEffectiveA2MaxDeg();
  const float allowedZMin  = recipeEffectiveZMinMm();
  const float allowedZMax  = recipeEffectiveZMaxMm();

  if (commandUsesA1(step.command) && (a1 < allowedA1Min - 0.01f || a1 > allowedA1Max + 0.01f)) {
    error = String(step.name) + ": A1 target outside legacy recipe range (allowed " +
            String(allowedA1Min, 1) + " to " + String(allowedA1Max, 1) + ")."; return false;
  }
  if (commandUsesA2(step.command) && (a2 < allowedA2Min - 0.01f || a2 > allowedA2Max + 0.01f)) {
    error = String(step.name) + ": A2 target outside legacy recipe range (allowed " +
            String(allowedA2Min, 1) + " to " + String(allowedA2Max, 1) + ")."; return false;
  }
  if (commandUsesZ(step.command) && (z < allowedZMin - 0.01f || z > allowedZMax + 0.01f)) {
    error = String(step.name) + ": Z target outside legacy recipe range (allowed " +
            String(allowedZMin, 1) + " to " + String(allowedZMax, 1) + ")."; return false;
  }
  if (step.command != RECIPE_CMD_WAIT &&
      (step.gripperDeg < gripperOpenDeg || step.gripperDeg > gripperMaxCloseDeg ||
       step.gripperDeg > GRIPPER_HARD_MAX_DEG)) {
    error = String(step.name) + ": gripper target outside calibration."; return false;
  }
  if (step.waitAfterMs > RECIPE_MAX_WAIT_MS) {
    error = String(step.name) + ": pause must be 30 seconds or less."; return false;
  }
  if (step.command == RECIPE_CMD_MOVE_SAFE_XYZ || step.command == RECIPE_CMD_MOVE_XYZ_DIRECT) {
    String targetError;
    if (!collisionPoseIsSafe(a1, a2, z, gripperHoldDeg, targetError)) {
      error = String(step.name) + ": target is forbidden. " + targetError; return false;
    }
  }
  return true;
}

String recipeKey(uint8_t recipeIndex, const char *suffix) {
  return "r" + String(recipeIndex) + String(suffix);
}

String recipeStepKey(uint8_t recipeIndex, uint8_t stepIndex, const char *suffix) {
  return "r" + String(recipeIndex) + "s" + String(stepIndex) + String(suffix);
}

void removeStoredStepKeys(uint8_t recipeIndex, uint8_t stepIndex) {
  // v10.20 atomic blob storage does not use per-step keys anymore.
}

void removeStoredRecipeKeys(uint8_t recipeIndex) {
  // v10.20 atomic blob storage does not use per-recipe keys anymore.
}


uint32_t recipeBlobHashBytes(uint32_t hash, const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619UL;
  }
  return hash;
}

size_t compactRecipeBytesForStepCount(uint8_t stepCount) {
  stepCount = constrain((int)stepCount, 0, (int)MAX_RECIPE_STEPS);
  return (size_t)PACKAGE_NAME_LENGTH + 1 + ((size_t)stepCount * sizeof(RecipeStep));
}

size_t compactRecipeBytes(const PackageRecipe &recipe) {
  return compactRecipeBytesForStepCount(recipe.stepCount);
}

uint32_t recipeBlobChecksum(uint8_t count, const PackageRecipe *recipes) {
  uint32_t hash = 2166136261UL;
  hash = recipeBlobHashBytes(hash, &count, sizeof(count));
  for (uint8_t r = 0; r < count && r < MAX_PACKAGE_RECIPES; r++) {
    uint8_t stepCount = constrain((int)recipes[r].stepCount, 0, (int)MAX_RECIPE_STEPS);
    hash = recipeBlobHashBytes(hash, (const uint8_t*)recipes[r].name, PACKAGE_NAME_LENGTH);
    hash = recipeBlobHashBytes(hash, &stepCount, sizeof(stepCount));
    hash = recipeBlobHashBytes(hash, (const uint8_t*)recipes[r].steps, sizeof(RecipeStep) * stepCount);
  }
  return hash;
}

void normalizeRecipesInRam() {
  packageRecipeCount = constrain((int)packageRecipeCount, 0, (int)MAX_PACKAGE_RECIPES);
  for (uint8_t r = 0; r < MAX_PACKAGE_RECIPES; r++) {
    PackageRecipe &recipe = packageRecipes[r];
    recipe.name[PACKAGE_NAME_LENGTH - 1] = '\0';
    recipe.stepCount = constrain((int)recipe.stepCount, 0, (int)MAX_RECIPE_STEPS);
    if (r >= packageRecipeCount) {
      memset(&packageRecipes[r], 0, sizeof(PackageRecipe));
      continue;
    }
    for (uint8_t s = 0; s < MAX_RECIPE_STEPS; s++) {
      RecipeStep &step = recipe.steps[s];
      step.name[RECIPE_STEP_NAME_LENGTH - 1] = '\0';
      if (s >= recipe.stepCount) {
        memset(&recipe.steps[s], 0, sizeof(RecipeStep));
        continue;
      }
      if (step.command > RECIPE_CMD_WAIT) step.command = RECIPE_CMD_MOVE_SAFE_XYZ;
      if (step.waitAfterMs > RECIPE_MAX_WAIT_MS) step.waitAfterMs = RECIPE_MAX_WAIT_MS;
      step.gripperDeg = constrain((int)step.gripperDeg, 0, 180);
    }
  }
}

// =====================================================
// v10.22 persistent recipe store
// =====================================================
// The previous NVS/Preferences methods failed on this project.
// v10.22 stores recipes in LittleFS as one atomic file:
//   /scara_recipes.dat
// Save flow:
//   write /scara_recipes.tmp -> verify -> replace /scara_recipes.dat
// If anything fails, the old file and the RAM recipe list are not erased.

const char *RECIPE_FILE_PATH = "/scara_recipes.dat";
const char *RECIPE_FILE_TMP_PATH = "/scara_recipes.tmp";
const char *RECIPE_FILE_BAK_PATH = "/scara_recipes.bak";
const char RECIPE_FILE_MAGIC[8] = {'S','C','A','R','A','R','2','2'};
bool recipeFsMounted = false;
bool recipeFsAttempted = false;
String recipeFsLastError = "Not mounted yet.";

bool ensureRecipeFileSystem() {
  if (recipeFsMounted) return true;
  if (!recipeFsAttempted) {
    recipeFsAttempted = true;
    recipeFsMounted = LittleFS.begin(true);
    if (recipeFsMounted) recipeFsLastError = "LittleFS mounted.";
    else recipeFsLastError = "LittleFS mount failed. Select a partition scheme with filesystem support.";
  }
  return recipeFsMounted;
}

bool recipeFileSystemReady() {
  return ensureRecipeFileSystem();
}

String recipeFileSystemError() {
  ensureRecipeFileSystem();
  return recipeFsLastError;
}

uint32_t recipeFsTotalBytes() {
  if (!ensureRecipeFileSystem()) return 0;
  return (uint32_t)LittleFS.totalBytes();
}

uint32_t recipeFsUsedBytes() {
  if (!ensureRecipeFileSystem()) return 0;
  return (uint32_t)LittleFS.usedBytes();
}

bool recipeFileExists() {
  if (!ensureRecipeFileSystem()) return false;
  return LittleFS.exists(RECIPE_FILE_PATH);
}

uint32_t storedRecipeTotalBytes() {
  if (!ensureRecipeFileSystem()) return 0;
  if (!LittleFS.exists(RECIPE_FILE_PATH)) return 0;
  File f = LittleFS.open(RECIPE_FILE_PATH, "r");
  if (!f) return 0;
  uint32_t size = (uint32_t)f.size();
  f.close();
  return size;
}

bool writeU8(File &f, uint8_t v) { return f.write(&v, 1) == 1; }
bool writeU16(File &f, uint16_t v) { return f.write((const uint8_t*)&v, sizeof(v)) == sizeof(v); }
bool writeU32(File &f, uint32_t v) { return f.write((const uint8_t*)&v, sizeof(v)) == sizeof(v); }
bool readU8(File &f, uint8_t &v) { return f.read(&v, 1) == 1; }
bool readU16(File &f, uint16_t &v) { return f.read((uint8_t*)&v, sizeof(v)) == sizeof(v); }
bool readU32(File &f, uint32_t &v) { return f.read((uint8_t*)&v, sizeof(v)) == sizeof(v); }

uint32_t compactPayloadBytes(uint8_t count, const PackageRecipe *recipes) {
  uint32_t total = 0;
  for (uint8_t r = 0; r < count && r < MAX_PACKAGE_RECIPES; r++) {
    total += 2;  // uint16_t length prefix
    total += (uint32_t)compactRecipeBytes(recipes[r]);
  }
  return total;
}

bool readStoredRecipesIntoScratchFromPath(const char *path, uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes);

bool writeRecipeFileAtomic() {
  normalizeRecipesInRam();
  if (!ensureRecipeFileSystem()) {
    recipeLastEvent = recipeFsLastError;
    return false;
  }

  uint8_t count = packageRecipeCount;
  uint32_t payloadBytes = compactPayloadBytes(count, packageRecipes);
  uint32_t crc = recipeBlobChecksum(count, packageRecipes);

  if (LittleFS.exists(RECIPE_FILE_TMP_PATH)) LittleFS.remove(RECIPE_FILE_TMP_PATH);

  File f = LittleFS.open(RECIPE_FILE_TMP_PATH, "w");
  if (!f) {
    recipeLastEvent = "Recipe save failed: cannot open temporary LittleFS file.";
    return false;
  }

  bool ok = true;
  ok &= f.write((const uint8_t*)RECIPE_FILE_MAGIC, sizeof(RECIPE_FILE_MAGIC)) == sizeof(RECIPE_FILE_MAGIC);
  ok &= writeU8(f, RECIPE_STORAGE_VERSION);
  ok &= writeU8(f, RECIPE_STORAGE_FORMAT_FIELDS);
  ok &= writeU8(f, count);
  ok &= writeU32(f, payloadBytes);
  ok &= writeU32(f, crc);

  for (uint8_t r = 0; r < count && ok; r++) {
    uint16_t len = (uint16_t)compactRecipeBytes(packageRecipes[r]);
    ok &= writeU16(f, len);
    ok &= f.write((const uint8_t*)&packageRecipes[r], len) == len;
  }

  f.flush();
  uint32_t fileBytes = (uint32_t)f.size();
  f.close();

  if (!ok || fileBytes < 18) {
    LittleFS.remove(RECIPE_FILE_TMP_PATH);
    recipeLastEvent = "Recipe save failed: LittleFS temporary file write was incomplete.";
    return false;
  }

  // Verify the temporary file before replacing the good file.
  uint8_t tmpCount = 0;
  uint32_t tmpCrc = 0;
  uint32_t tmpBytes = 0;
  if (!readStoredRecipesIntoScratchFromPath(RECIPE_FILE_TMP_PATH, tmpCount, tmpCrc, tmpBytes)) {
    LittleFS.remove(RECIPE_FILE_TMP_PATH);
    recipeLastEvent = "Recipe save failed: temporary file verification failed.";
    return false;
  }
  if (tmpCount != count || tmpCrc != crc || tmpBytes != fileBytes) {
    LittleFS.remove(RECIPE_FILE_TMP_PATH);
    recipeLastEvent = "Recipe save failed: temporary file checksum mismatch.";
    return false;
  }

  if (LittleFS.exists(RECIPE_FILE_BAK_PATH)) LittleFS.remove(RECIPE_FILE_BAK_PATH);
  if (LittleFS.exists(RECIPE_FILE_PATH)) {
    if (!LittleFS.rename(RECIPE_FILE_PATH, RECIPE_FILE_BAK_PATH)) {
      LittleFS.remove(RECIPE_FILE_TMP_PATH);
      recipeLastEvent = "Recipe save failed: cannot move old recipe file to backup.";
      return false;
    }
  }

  if (!LittleFS.rename(RECIPE_FILE_TMP_PATH, RECIPE_FILE_PATH)) {
    if (LittleFS.exists(RECIPE_FILE_BAK_PATH)) LittleFS.rename(RECIPE_FILE_BAK_PATH, RECIPE_FILE_PATH);
    recipeLastEvent = "Recipe save failed: cannot activate new recipe file.";
    return false;
  }

  if (LittleFS.exists(RECIPE_FILE_BAK_PATH)) LittleFS.remove(RECIPE_FILE_BAK_PATH);
  recipeLastEvent = "Recipes saved to LittleFS file /scara_recipes.dat.";
  return true;
}

bool readStoredRecipesIntoScratchFromPath(const char *path, uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes) {
  storedCount = 0;
  storedCrc = 0;
  storedFileBytes = 0;
  if (!ensureRecipeFileSystem()) return false;
  if (!LittleFS.exists(path)) return false;

  File f = LittleFS.open(path, "r");
  if (!f) return false;
  storedFileBytes = (uint32_t)f.size();

  char magic[8];
  if (f.read((uint8_t*)magic, sizeof(magic)) != sizeof(magic)) { f.close(); return false; }
  for (uint8_t i = 0; i < sizeof(RECIPE_FILE_MAGIC); i++) {
    if (magic[i] != RECIPE_FILE_MAGIC[i]) { f.close(); return false; }
  }

  uint8_t version = 0;
  uint8_t format = 0;
  uint8_t count = 0;
  uint32_t payloadBytes = 0;
  uint32_t expectedCrc = 0;
  if (!readU8(f, version) || !readU8(f, format) || !readU8(f, count) ||
      !readU32(f, payloadBytes) || !readU32(f, expectedCrc)) {
    f.close(); return false;
  }
  if (version != RECIPE_STORAGE_VERSION || format != RECIPE_STORAGE_FORMAT_FIELDS) { f.close(); return false; }
  if (count > MAX_PACKAGE_RECIPES) { f.close(); return false; }

  memset(recipeImportScratch, 0, sizeof(recipeImportScratch));
  uint32_t actualPayloadBytes = 0;

  for (uint8_t r = 0; r < count; r++) {
    uint16_t len = 0;
    if (!readU16(f, len)) { f.close(); return false; }
    actualPayloadBytes += 2;
    actualPayloadBytes += len;
    if (len < compactRecipeBytesForStepCount(0) || len > sizeof(PackageRecipe)) { f.close(); return false; }

    size_t readBytes = f.read((uint8_t*)&recipeImportScratch[r], len);
    if (readBytes != len) { f.close(); return false; }

    recipeImportScratch[r].name[PACKAGE_NAME_LENGTH - 1] = '\0';
    recipeImportScratch[r].stepCount = constrain((int)recipeImportScratch[r].stepCount, 0, (int)MAX_RECIPE_STEPS);
    size_t expectedLen = compactRecipeBytes(recipeImportScratch[r]);
    if (len != expectedLen) { f.close(); return false; }

    for (uint8_t s = 0; s < MAX_RECIPE_STEPS; s++) {
      recipeImportScratch[r].steps[s].name[RECIPE_STEP_NAME_LENGTH - 1] = '\0';
      if (s >= recipeImportScratch[r].stepCount) {
        memset(&recipeImportScratch[r].steps[s], 0, sizeof(RecipeStep));
      } else {
        if (recipeImportScratch[r].steps[s].command > RECIPE_CMD_WAIT) { f.close(); return false; }
        if (recipeImportScratch[r].steps[s].waitAfterMs > RECIPE_MAX_WAIT_MS) { f.close(); return false; }
        recipeImportScratch[r].steps[s].gripperDeg = constrain((int)recipeImportScratch[r].steps[s].gripperDeg, 0, 180);
      }
    }
  }

  if (f.available() != 0) { f.close(); return false; }
  f.close();

  storedCount = count;
  storedCrc = recipeBlobChecksum(storedCount, recipeImportScratch);
  if (actualPayloadBytes != payloadBytes) return false;
  if (storedCrc != expectedCrc) return false;
  return true;
}

bool readStoredRecipesIntoScratch(uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes) {
  return readStoredRecipesIntoScratchFromPath(RECIPE_FILE_PATH, storedCount, storedCrc, storedFileBytes);
}

bool savePackageRecipes() {
  bool ok = writeRecipeFileAtomic();
  if (!ok && recipeLastEvent.length() == 0) recipeLastEvent = "Recipe save failed.";
  return ok;
}

bool loadFieldPackageRecipes() {
  uint8_t count = 0;
  uint32_t crc = 0;
  uint32_t fileBytes = 0;
  if (!readStoredRecipesIntoScratch(count, crc, fileBytes)) return false;

  memset(packageRecipes, 0, sizeof(packageRecipes));
  for (uint8_t r = 0; r < count; r++) packageRecipes[r] = recipeImportScratch[r];
  packageRecipeCount = count;
  normalizeRecipesInRam();
  return true;
}

bool verifyStoredPackageRecipesAgainstRam(uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes) {
  normalizeRecipesInRam();
  if (!readStoredRecipesIntoScratch(storedCount, storedCrc, storedFileBytes)) return false;
  if (storedCount != packageRecipeCount) return false;
  uint32_t ramCrc = recipeBlobChecksum(packageRecipeCount, packageRecipes);
  return storedCrc == ramCrc;
}

bool getStoredRecipeInfo(uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes) {
  return readStoredRecipesIntoScratch(storedCount, storedCrc, storedFileBytes);
}

void resetRecipeRuntimeState() {
  recipeState = RECIPE_IDLE;
  recipeRunning = false;
  recipeInternalMotionCommand = false;
  recipeActiveIndex = -1;
  recipeStepIndex = 0;
  recipeCycle = 0;
  recipeRepeatTarget = 1;
  recipeTimerDurationMs = 0;
  recipeMotionDescription = "Idle.";
  recipeStatusMessage = "Idle.";
}

void loadPackageRecipes() {
  bool loadedFile = loadFieldPackageRecipes();

  if (!loadedFile) {
    packageRecipeCount = 0;
    memset(packageRecipes, 0, sizeof(packageRecipes));
  }

  resetRecipeRuntimeState();

  if (loadedFile && packageRecipeCount > 0) {
    recipeLastEvent = "Stored recipes loaded from LittleFS.";
  } else if (loadedFile && packageRecipeCount == 0) {
    recipeLastEvent = "Recipe file is valid but empty.";
  } else {
    recipeLastEvent = "No valid LittleFS recipe file. Library is empty.";
  }
}

void clearAllPackageRecipes(bool clearLegacyStorage) {
  if (recipeRunning) return;

  if (ensureRecipeFileSystem()) {
    if (LittleFS.exists(RECIPE_FILE_PATH)) LittleFS.remove(RECIPE_FILE_PATH);
    if (LittleFS.exists(RECIPE_FILE_TMP_PATH)) LittleFS.remove(RECIPE_FILE_TMP_PATH);
    if (LittleFS.exists(RECIPE_FILE_BAK_PATH)) LittleFS.remove(RECIPE_FILE_BAK_PATH);
  }

  packageRecipeCount = 0;
  memset(packageRecipes, 0, sizeof(packageRecipes));
  resetRecipeRuntimeState();

  if (clearLegacyStorage) {
    Preferences legacy1;
    legacy1.begin("scara_pkg1", false);
    legacy1.clear();
    legacy1.end();

    Preferences legacy2;
    legacy2.begin("scara_pkg2", false);
    legacy2.clear();
    legacy2.end();

    Preferences legacy3;
    legacy3.begin("scara_pkg3", false);
    legacy3.clear();
    legacy3.end();

    Preferences legacy4;
    legacy4.begin("scara_pkg4", false);
    legacy4.clear();
    legacy4.end();
  }

  recipeLastEvent = clearLegacyStorage ?
    "All saved recipes cleared. Old NVS recipe storage also cleared." :
    "All saved recipes cleared.";
}

bool reloadPackageRecipesFromEsp32(String &statusMessage) {
  if (recipeRunning || armJobLocksManualControl()) {
    statusMessage = "Stop playback/job before reloading recipes from ESP32.";
    return false;
  }
  loadPackageRecipes();
  statusMessage = recipeLastEvent;
  return true;
}

bool createPackageRecipe(const String &name, uint8_t &createdIndex, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (recipeRunning) { error = "Stop playback before editing recipes."; return false; }
  if (packageRecipeCount >= MAX_PACKAGE_RECIPES) { error = "Maximum of 12 recipes reached."; return false; }
  createdIndex = packageRecipeCount++;
  memset(&packageRecipes[createdIndex], 0, sizeof(PackageRecipe));
  copyRecipeText(packageRecipes[createdIndex].name, PACKAGE_NAME_LENGTH, name, "RECIPE_" + String(createdIndex + 1));
  savePackageRecipes();
  error = "Recipe created and saved inside ESP32.";
  return true;
}

bool renamePackageRecipe(uint8_t recipeIndex, const String &name, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validPackageRecipeIndex(recipeIndex)) { error = "Select a recipe first."; return false; }
  if (recipeRunning) { error = "Stop playback before editing recipes."; return false; }
  copyRecipeText(packageRecipes[recipeIndex].name, PACKAGE_NAME_LENGTH, name, "RECIPE_" + String(recipeIndex + 1));
  savePackageRecipes();
  error = "Recipe renamed and saved inside ESP32.";
  return true;
}

bool deletePackageRecipe(uint8_t recipeIndex, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validPackageRecipeIndex(recipeIndex)) { error = "Select a recipe first."; return false; }
  if (recipeRunning) { error = "Stop playback before editing recipes."; return false; }
  for (uint8_t r = recipeIndex; r + 1 < packageRecipeCount; r++) packageRecipes[r] = packageRecipes[r + 1];
  packageRecipeCount--;
  savePackageRecipes();
  error = "Recipe deleted and saved inside ESP32.";
  return true;
}

bool addRecipeStep(uint8_t recipeIndex, const RecipeStep &step, uint8_t &createdIndex, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validPackageRecipeIndex(recipeIndex)) { error = "Select a recipe first."; return false; }
  if (recipeRunning) { error = "Stop playback before editing steps."; return false; }
  if (packageRecipes[recipeIndex].stepCount >= MAX_RECIPE_STEPS) { error = "Recipe reached 24 steps."; return false; }
  if (!validateRecipeStepDefinition(step, error)) return false;
  createdIndex = packageRecipes[recipeIndex].stepCount++;
  packageRecipes[recipeIndex].steps[createdIndex] = step;
  savePackageRecipes();
  error = "Step added and saved inside ESP32.";
  return true;
}

bool updateRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, const RecipeStep &step, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validRecipeStepIndex(recipeIndex, stepIndex)) { error = "Select a step first."; return false; }
  if (recipeRunning) { error = "Stop playback before editing steps."; return false; }
  if (!validateRecipeStepDefinition(step, error)) return false;
  packageRecipes[recipeIndex].steps[stepIndex] = step;
  savePackageRecipes();
  error = "Step updated and saved inside ESP32.";
  return true;
}

bool deleteRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validRecipeStepIndex(recipeIndex, stepIndex)) { error = "Selected step does not exist."; return false; }
  if (recipeRunning) { error = "Stop playback before editing steps."; return false; }
  PackageRecipe &recipe = packageRecipes[recipeIndex];
  for (uint8_t s = stepIndex; s + 1 < recipe.stepCount; s++) recipe.steps[s] = recipe.steps[s + 1];
  recipe.stepCount--;
  savePackageRecipes();
  error = "Step deleted and saved inside ESP32.";
  return true;
}

bool reorderRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, bool moveUp, String &error) {
  if (armJobLocksManualControl()) { error = "Abort or complete the autonomous job before editing recipes."; return false; }
  if (!validRecipeStepIndex(recipeIndex, stepIndex)) { error = "Selected step does not exist."; return false; }
  if (recipeRunning) { error = "Stop playback before editing steps."; return false; }
  PackageRecipe &recipe = packageRecipes[recipeIndex];
  int target = moveUp ? (int)stepIndex - 1 : (int)stepIndex + 1;
  if (target < 0 || target >= recipe.stepCount) { error = "Step is already at that end."; return false; }
  RecipeStep temp = recipe.steps[stepIndex];
  recipe.steps[stepIndex] = recipe.steps[target];
  recipe.steps[target] = temp;
  savePackageRecipes();
  error = "Step order updated and saved inside ESP32.";
  return true;
}

bool queueExplicitDirectMove(float targetA1, float targetA2, float targetZ,
                             const String &description, String &error) {
  const float allowedA1Min = recipeInternalMotionCommand ? recipeEffectiveA1MinDeg() : a1MinDeg;
  const float allowedA1Max = recipeInternalMotionCommand ? recipeEffectiveA1MaxDeg() : a1MaxDeg;
  const float allowedA2Min = recipeInternalMotionCommand ? recipeEffectiveA2MinDeg() : a2MinDeg;
  const float allowedA2Max = recipeInternalMotionCommand ? recipeEffectiveA2MaxDeg() : a2MaxDeg;
  const float allowedZMin  = recipeInternalMotionCommand ? recipeEffectiveZMinMm()   : zMinMm;
  const float allowedZMax  = recipeInternalMotionCommand ? recipeEffectiveZMaxMm()   : zMaxMm;
  if (targetA1 < allowedA1Min || targetA1 > allowedA1Max ||
      targetA2 < allowedA2Min || targetA2 > allowedA2Max ||
      targetZ < allowedZMin || targetZ > allowedZMax) {
    error = description + ": target outside legacy recipe range."; return false;
  }
  String poseError;
  if (!collisionPoseIsSafe(targetA1, targetA2, targetZ, gripperCommandDeg, poseError)) {
    error = description + ": target blocked. " + poseError; return false;
  }
  String pathError;
  if (!collisionPathIsSafe(actualA1(), actualA2(), actualZ(),
                           targetA1, targetA2, targetZ, gripperCommandDeg, pathError)) {
    error = description + ": exact path blocked. " + pathError; return false;
  }
  String ignored;
  queueDirectJointMove(targetA1, targetA2, targetZ, PROFILE_MANUAL_FK, ignored);
  collisionSafeRouteActive = false;
  collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
  lastMotionPlan = "Recipe explicit step: " + description + ".";
  recipeMotionDescription = description;
  return true;
}

bool executeRecipeMovementStep(const RecipeStep &step, String &error) {
  float targetA1 = actualA1();
  float targetA2 = actualA2();
  float targetZ = actualZ();
  float storedA1 = a1StepsToAngle(step.a1RawSteps);
  float storedA2 = a2StepsToAngle(step.a2RawSteps);
  float storedZ = zStepsToMm(step.zRawSteps);
  String description = String(step.name) + " — " + recipeCommandDisplayName(step.command);

  if (step.command == RECIPE_CMD_MOVE_SAFE_XYZ) {
    recipeInternalMotionCommand = true;
    String plannerMessage;
    bool accepted = queueJointMove(storedA1, storedA2, storedZ, PROFILE_MANUAL_FK, plannerMessage);
    recipeInternalMotionCommand = false;
    if (!accepted) { error = description + ": " + plannerMessage; return false; }
    recipeMotionDescription = description + " — " + plannerMessage;
    return true;
  }

  if (step.command == RECIPE_CMD_MOVE_A1_ONLY) targetA1 = storedA1;
  else if (step.command == RECIPE_CMD_MOVE_A2_ONLY) targetA2 = storedA2;
  else if (step.command == RECIPE_CMD_MOVE_Z_ONLY) targetZ = storedZ;
  else if (step.command == RECIPE_CMD_MOVE_A1_A2_TOGETHER) { targetA1 = storedA1; targetA2 = storedA2; }
  else if (step.command == RECIPE_CMD_MOVE_XYZ_DIRECT) { targetA1 = storedA1; targetA2 = storedA2; targetZ = storedZ; }
  else { error = "Step is not a movement command."; return false; }

  recipeInternalMotionCommand = true;
  bool accepted = queueExplicitDirectMove(targetA1, targetA2, targetZ, description, error);
  recipeInternalMotionCommand = false;
  return accepted;
}

bool applyStoredStepGripper(const RecipeStep &step, String &error) {
  if (step.command == RECIPE_CMD_WAIT) return true;

  if (gripperCommandDeg == step.gripperDeg) {
    recipeMotionDescription = String(step.name) + " — gripper already at " +
                              String(step.gripperDeg) + " deg.";
    return true;
  }

  if (!commandGripperAngle(step.gripperDeg, error)) {
    error = String(step.name) + ": gripper command failed. " + error;
    return false;
  }

  recipeMotionDescription = String(step.name) + " — gripper set to " +
                            String(step.gripperDeg) + " deg.";
  recipeLastEvent = recipeMotionDescription;
  return true;
}

void advanceRecipeStep() {
  if (recipeStepIndex + 1 < packageRecipes[recipeActiveIndex].stepCount) {
    recipeStepIndex++;
    recipeState = RECIPE_START_STEP;
  } else if (recipeCycle < recipeRepeatTarget) {
    recipeCycle++;
    recipeStepIndex = 0;
    recipeState = RECIPE_START_STEP;
    recipeLastEvent = "Starting repeat cycle " + String(recipeCycle) + ".";
  } else {
    recipeRunning = false;
    recipeState = RECIPE_COMPLETE;
    recipeStatusMessage = "Recipe complete.";
    recipeMotionDescription = "Complete.";
    recipeLastEvent = "Recipe complete: " + activePackageRecipeName() + ".";
  }
}

void beginStepPause(const RecipeStep &step) {
  if (step.waitAfterMs == 0) { advanceRecipeStep(); return; }
  recipeTimerStartedMs = millis();
  recipeTimerDurationMs = step.waitAfterMs;
  recipeState = RECIPE_WAIT_FOR_TIMER;
  recipeStatusMessage = "Waiting " + String(step.waitAfterMs / 1000.0f, 1) + " s after " + String(step.name) + ".";
  recipeLastEvent = recipeStatusMessage;
}

bool startPackageRecipe(uint8_t recipeIndex, uint16_t repeats, String &error) {
  if (!validPackageRecipeIndex(recipeIndex)) { error = "Select a recipe to run."; return false; }
  if (recipeRunning || anyMotion() || collisionSafeRouteActive) { error = "Robot is already moving."; return false; }
  if (!isHomed || isHoming || faultActive) { error = "HOME the arm before playback."; return false; }
  if (!collisionProtectionEnabled) { error = "Enable collision protection before playback."; return false; }
  PackageRecipe &recipe = packageRecipes[recipeIndex];
  if (recipe.stepCount == 0) { error = "Selected recipe has no steps."; return false; }
  if (repeats < 1 || repeats > RECIPE_MAX_REPEATS) { error = "Repeat must be 1 to 100."; return false; }
  for (uint8_t s = 0; s < recipe.stepCount; s++) {
    if (!validateRecipeStepDefinition(recipe.steps[s], error)) return false;
  }
  recipeActiveIndex = recipeIndex;
  recipeStepIndex = 0;
  recipeCycle = 1;
  recipeRepeatTarget = repeats;
  recipeRunning = true;
  recipeState = RECIPE_START_STEP;
  recipeMotionDescription = "Starting.";
  recipeStatusMessage = "Running " + String(recipe.name) + ".";
  recipeLastEvent = "Recipe started: " + String(recipe.name) + ".";
  error = "Recipe started.";
  return true;
}

bool stopPackageRecipe(const String &reason) {
  if (!recipeRunning && recipeState != RECIPE_COMPLETE) return false;
  recipeRunning = false;
  recipeState = RECIPE_FAULT;
  recipeMotionDescription = "Stopped.";
  recipeStatusMessage = "Stopped: " + reason;
  recipeLastEvent = recipeStatusMessage;
  return true;
}

void resetRecipeAfterHome() {
  recipeRunning = false;
  recipeState = RECIPE_IDLE;
  recipeActiveIndex = -1;
  recipeStepIndex = 0;
  recipeCycle = 0;
  recipeRepeatTarget = 1;
  recipeTimerDurationMs = 0;
  recipeMotionDescription = "Idle.";
  recipeStatusMessage = "Ready.";
  recipeLastEvent = "HOME complete. Recipe system ready.";
}

bool resetRecipeStatus(String &error) {
  if (recipeRunning || anyMotion() || collisionSafeRouteActive) { error = "Stop movement before resetting status."; return false; }
  if (faultActive || !isHomed) { error = "HOME is required before resetting status."; return false; }
  resetRecipeAfterHome();
  error = "Recipe status reset.";
  return true;
}

void packageRecipeFault(const String &reason) {
  recipeRunning = false;
  recipeState = RECIPE_FAULT;
  recipeMotionDescription = "Fault.";
  recipeStatusMessage = "RECIPE FAULT: " + reason;
  recipeLastEvent = recipeStatusMessage;
  Serial.println(recipeStatusMessage);
}

void servicePackageRecipe() {
  if (!recipeRunning) return;
  if (faultActive) { packageRecipeFault("Robot fault: " + faultMessage); return; }
  if (!validRecipeStepIndex(recipeActiveIndex, recipeStepIndex)) { packageRecipeFault("Active recipe step is invalid."); return; }

  RecipeStep &step = packageRecipes[recipeActiveIndex].steps[recipeStepIndex];
  switch (recipeState) {
    case RECIPE_START_STEP: {
      recipeStatusMessage = "Step " + String(recipeStepIndex + 1) + ": " + String(step.name) + ".";

      if (step.command == RECIPE_CMD_WAIT) {
        beginStepPause(step);
        return;
      }

      if (step.command == RECIPE_CMD_SET_GRIPPER) {
        String error;
        if (!applyStoredStepGripper(step, error)) {
          packageRecipeFault(error);
          return;
        }
        beginStepPause(step);
        return;
      }

      String error;
      if (!executeRecipeMovementStep(step, error)) {
        packageRecipeFault(error);
        return;
      }
      recipeState = RECIPE_WAIT_FOR_MOTION;
      recipeLastEvent = "Moving: " + recipeMotionDescription + ".";
      return;
    }

    case RECIPE_WAIT_FOR_MOTION:
      if (!anyMotion() && !collisionSafeRouteActive) {
        recipeState = RECIPE_APPLY_STEP_GRIPPER;
      }
      return;

    case RECIPE_APPLY_STEP_GRIPPER: {
      String error;
      if (!applyStoredStepGripper(step, error)) {
        packageRecipeFault(error);
        return;
      }
      beginStepPause(step);
      return;
    }

    case RECIPE_WAIT_FOR_TIMER:
      if (millis() - recipeTimerStartedMs >= recipeTimerDurationMs) {
        recipeTimerDurationMs = 0;
        advanceRecipeStep();
      }
      return;

    case RECIPE_COMPLETE:
    case RECIPE_FAULT:
    case RECIPE_IDLE:
    default:
      return;
  }
}
