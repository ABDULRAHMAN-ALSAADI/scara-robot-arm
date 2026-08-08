#include <AccelStepper.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <LittleFS.h>
#include "webpage.h"

/*
  SCARA Robot Arm — Structured Firmware v10.22 LittleFS Recipe Store
  ============================================================================

  This version stores real physical endpoints as raw step positions measured
  from HOME. A changed geometric zero does not move saved physical endpoints.
  v9.1 fixed missing raw-calibration declarations.
  v9.2 fixed browser-to-controller speed-slider synchronization.
  v9.3 gives active normal movements high-frequency motor service while
  retaining web status and STOP response.
  v9.4 adds raw Z travel calibration, calibration export/restore, and
  FK-to-IK posture-preserving verification.
  v9.5 adds a calibrated servo gripper with OPEN / HOLD controls and
  safe maximum-close protection.
  v9.6 adds a Safe Startup interlock and Power-Off Park pose.
  v9.7 replaces the boundary-curve concept with an explicit forbidden
  A1/A2/Z zone table edited from calibrated sliders and applied to all moves.
  v9.7.1 restores the missing browser startup-confirmation status function.
  v9.8 clarifies fixed collision rules and stored Preferences inspection.
  v9.9 replaces repeated fixed rules with interpolated A1-to-A2 collision envelopes.
  v9.10 separates A1 side from the blocked A2 direction.
  v9.11 adds Z-layered envelope maps: different A1/A2 collision boundaries can
  be active at different Z heights.
  v9.12 adds separate collision-layer JSON export/import using degrees and mm,
  so collision maps can be restored without overwriting robot calibration.
  v10.2 adds package recipes for the future automated cell:
    - One recipe for each detected box type / shelf destination.
    - Each recipe is taught by moving FK/Real-Time and the gripper, then saving.
    - Recipe Run tests the selected recipe before MQTT and IR integration.
  v10.3 stores the motion method used to reach every pose. Recipe Run executes
  the taught staged motion exactly and does not silently inject AUTO SAFE ROUTE.
  v10.4 makes each transition directly editable in Saved Order. Movement order
  can be saved without recapturing or changing stored A1/A2/Z/gripper values.
  v10.5 corrects Recipe Run to use the same AUTO SAFE ROUTE planner as FK by
  default. Exact order is represented by recording intermediate poses.
  v10.6 replaces pose-path selection with an explicit command timeline:
    move individual axes, move combined axes, set the gripper, and wait after steps.
  v10.8 displays the stored gripper value inline and executes each movement
  step's gripper target after its axis movement finishes.
  v10.9 adds an Arm Job Controller above the Recipe Timeline:
    load a classified-box job, wait for AMR arrival, run its recipe, and report result.
  v10.10 changes WiFi to AP+STA:
    backup setup AP remains available, while the arm also joins ALSAADI hotspot.
  v10.11 forces a clean WiFi restart, starts a visible backup AP first,
    then joins ALSAADI. This is for diagnosing AP-not-visible problems.
  v10.13 adds safe driver shutdown at the first line of setup and explicit
    recipe storage verification endpoints.
  v10.18 adds a commit-and-verify recipe save path so taught recipes remain
    stored in ESP32 Preferences and survive restart without JSON re-upload.
  v10.19 removes automatic legacy recipe migration, adds a clean saved-recipe
    library manager, clears ghost recipes on request, and fixes the browser HOME
    status sticking on "Homing in progress".

  Architecture:
    - This main tab contains hardware configuration, global state, function declarations,
      setup(), and loop().
    - Other tabs contain one subsystem each.
    - webpage.h remains one separate tab and includes the safety-calibration controls.

  Important:
    - The working v9.5 raw calibration, gripper, motion profiles and FK/IK logic
      are preserved; v9.6 layers collision checking and startup interlocks above them.
    - Upload normally with "Erase All Flash Before Sketch Upload" disabled so saved
      joint calibration and tool settings remain in ESP32 non-volatile memory.
*/

// ── Arm 1 (Shoulder) ──
#define A1_STEP   17
#define A1_DIR    16
#define A1_EN     5
#define A1_LIMIT  14

// ── Arm 2 (Elbow) ──
#define A2_STEP   19
#define A2_DIR    18
#define A2_EN     21
#define A2_LIMIT  27

// ── Z Axis ──
#define Z_STEP    2
#define Z_DIR     15
#define Z_EN      4
#define Z_LIMIT   26

// ── Servo Gripper ──
// GPIO 13 is unused by the present stepper and limit-switch wiring.
// Connect the servo SIGNAL wire here; power the servo from an external 5 V supply.
#define GRIPPER_SERVO_PIN 13

// =====================================================
// Mechanical constants
// =====================================================

constexpr float A1_STEPS_PER_DEG = 142.22f;
constexpr float A2_STEPS_PER_DEG = 40.0f;
constexpr float Z_STEPS_PER_MM   = 400.0f;

constexpr float L1 = 136.5f;

// Tool Center Point (TCP): distance from the A2 rotation axis to the point
// where the object is actually gripped. The new gripper drawing indicates
// approximately 120.0 mm to the grasp center. This is editable in Setup.
float toolTcpLengthMm = 120.0f;

// Servo gripper command and calibration.
// The displayed gripper angle is commanded position; there is no feedback sensor.
constexpr uint32_t GRIPPER_PWM_FREQUENCY_HZ = 50;
constexpr uint8_t GRIPPER_PWM_RESOLUTION_BITS = 16;
constexpr uint16_t GRIPPER_SERVO_MIN_PULSE_US = 500;
constexpr uint16_t GRIPPER_SERVO_MAX_PULSE_US = 2500;
constexpr uint8_t GRIPPER_HARD_MAX_DEG = 120;  // 180 deg is deliberately prohibited

constexpr uint8_t DEFAULT_GRIPPER_OPEN_DEG = 0;
constexpr uint8_t DEFAULT_GRIPPER_HOLD_DEG = 70;
constexpr uint8_t DEFAULT_GRIPPER_MAX_CLOSE_DEG = 90;

uint8_t gripperOpenDeg = DEFAULT_GRIPPER_OPEN_DEG;
uint8_t gripperHoldDeg = DEFAULT_GRIPPER_HOLD_DEG;
uint8_t gripperMaxCloseDeg = DEFAULT_GRIPPER_MAX_CLOSE_DEG;
uint8_t gripperCommandDeg = DEFAULT_GRIPPER_OPEN_DEG;
bool gripperAttached = false;

constexpr unsigned long GRIPPER_OPEN_BEFORE_HOME_MS = 700UL;

// =====================================================
// Collision-safety calibration: Z-layered directional envelopes
// =====================================================
//
// One layer contains:
//   - A1 side: POSITIVE or NEGATIVE FK space
//   - blocked A2 direction: ABOVE boundary or BELOW boundary
//   - active Z range
//   - interpolated points: A1 -> A2 permitted boundary
//
// The same A1 side can have different boundary curves at different Z heights.

constexpr uint8_t MAX_ENVELOPE_POINTS = 16;
constexpr uint8_t MAX_ENVELOPE_LAYERS = 12;
constexpr uint8_t A1_REGION_POSITIVE = 0;
constexpr uint8_t A1_REGION_NEGATIVE = 1;
constexpr uint8_t A2_BOUND_MAXIMUM = 0;   // A2 must stay <= boundary
constexpr uint8_t A2_BOUND_MINIMUM = 1;   // A2 must stay >= boundary

constexpr float DEFAULT_ENVELOPE_MARGIN_DEG = 0.0f;
constexpr float DEFAULT_SAFE_CORRIDOR_A2_DEG = 0.0f;
constexpr float MIGRATION_BASE_Z_FROM_MM = -135.0f;
constexpr float MIGRATION_BASE_Z_TO_MM = 0.0f;

struct CollisionEnvelopePoint {
  long a1RawSteps;
  long a2RawLimitSteps;
};

struct CollisionEnvelopeLayer {
  uint8_t a1Region;
  uint8_t a2Bound;
  long zRawMin;
  long zRawMax;
  uint8_t pointCount;
  CollisionEnvelopePoint points[MAX_ENVELOPE_POINTS];
};

CollisionEnvelopeLayer envelopeLayers[MAX_ENVELOPE_LAYERS];
uint8_t envelopeLayerCount = 0;
bool collisionProtectionEnabled = false;
float collisionEnvelopeMarginDeg = DEFAULT_ENVELOPE_MARGIN_DEG;
float collisionSafeCorridorA2Deg = DEFAULT_SAFE_CORRIDOR_A2_DEG;

bool startupParkSaved = false;
long startupParkA1RawSteps = 0L;
long startupParkA2RawSteps = 0L;
long startupParkZRawSteps = 0L;

// RAM-only confirmation: without absolute encoders software cannot prove the
// physical startup posture after power-off or manual movement.
bool startupPoseConfirmed = false;

// =====================================================
// Package Recipe Command Timeline
// =====================================================
//
// One recipe represents one classified package and its destination shelf.
// Every recipe is a list of explicit commands. A command can move a selected
// axis, move combined axes, operate the gripper, or wait for a chosen time.

constexpr uint8_t MAX_PACKAGE_RECIPES = 12;
constexpr uint8_t MAX_RECIPE_STEPS = 24;
constexpr uint8_t PACKAGE_NAME_LENGTH = 38;
constexpr uint8_t RECIPE_STEP_NAME_LENGTH = 32;
constexpr uint16_t RECIPE_MAX_REPEATS = 100;
constexpr uint16_t RECIPE_MAX_WAIT_MS = 30000;
constexpr uint8_t RECIPE_STORAGE_VERSION = 5;
constexpr uint8_t RECIPE_STORAGE_FORMAT_FIELDS = 5;  // v10.22: LittleFS atomic recipe file store

enum RecipeCommand : uint8_t {
  RECIPE_CMD_MOVE_SAFE_XYZ = 0,
  RECIPE_CMD_MOVE_A1_ONLY,
  RECIPE_CMD_MOVE_A2_ONLY,
  RECIPE_CMD_MOVE_Z_ONLY,
  RECIPE_CMD_MOVE_A1_A2_TOGETHER,
  RECIPE_CMD_MOVE_XYZ_DIRECT,
  RECIPE_CMD_SET_GRIPPER,
  RECIPE_CMD_WAIT
};

struct __attribute__((packed)) RecipeStep {
  char name[RECIPE_STEP_NAME_LENGTH];
  uint8_t command;
  long a1RawSteps;
  long a2RawSteps;
  long zRawSteps;
  uint8_t gripperDeg;
  uint16_t waitAfterMs;
};

struct __attribute__((packed)) PackageRecipe {
  char name[PACKAGE_NAME_LENGTH];
  uint8_t stepCount;
  RecipeStep steps[MAX_RECIPE_STEPS];
};

PackageRecipe packageRecipes[MAX_PACKAGE_RECIPES];
PackageRecipe recipeImportScratch[MAX_PACKAGE_RECIPES];
uint8_t packageRecipeCount = 0;

enum RecipePlaybackState : uint8_t {
  RECIPE_IDLE = 0,
  RECIPE_START_STEP,
  RECIPE_WAIT_FOR_MOTION,
  RECIPE_APPLY_STEP_GRIPPER,
  RECIPE_WAIT_FOR_TIMER,
  RECIPE_COMPLETE,
  RECIPE_FAULT
};

RecipePlaybackState recipeState = RECIPE_IDLE;
bool recipeRunning = false;
bool recipeInternalMotionCommand = false;
int8_t recipeActiveIndex = -1;
uint8_t recipeStepIndex = 0;
uint16_t recipeCycle = 0;
uint16_t recipeRepeatTarget = 1;
unsigned long recipeTimerStartedMs = 0UL;
uint16_t recipeTimerDurationMs = 0;
String recipeMotionDescription = "Idle.";
String recipeStatusMessage = "Idle.";
String recipeLastEvent = "No recipe executed.";


// =====================================================
// Arm Job Controller
// =====================================================
//
// This layer is the arm subsystem interface. The conveyor/coordinator supplies
// the package class and selected recipe. For this version AMR arrival is
// simulated from the web interface; no IR sensor or MQTT is active yet.

constexpr uint8_t JOB_ID_LENGTH = 30;
constexpr uint8_t PACKAGE_CLASS_LENGTH = 34;

enum ArmJobState : uint8_t {
  ARM_JOB_NOT_HOMED = 0,
  ARM_JOB_READY,
  ARM_JOB_LOADED,
  ARM_JOB_WAITING_FOR_AMR,
  ARM_JOB_RUNNING,
  ARM_JOB_DONE,
  ARM_JOB_FAULT,
  ARM_JOB_STOPPED
};

struct ActiveArmJob {
  char jobId[JOB_ID_LENGTH];
  char packageClass[PACKAGE_CLASS_LENGTH];
  int8_t recipeIndex;
  bool amrConfirmed;
};

ActiveArmJob activeArmJob;
ArmJobState armJobState = ARM_JOB_NOT_HOMED;
String armJobMessage = "HOME required.";
String armJobResult = "";
String armJobLastEvent = "No autonomous job loaded.";


// Z now follows the same raw-machine calibration principle as A1/A2.
// The top HOME switch is raw step 0. Positive raw travel moves downward.
constexpr long DEFAULT_Z_ZERO_RAW_STEPS  = 55000L;   // preserves former Z = 0 location
constexpr long DEFAULT_Z_UPPER_RAW_STEPS = 1000L;    // safe clearance below top switch
constexpr long DEFAULT_Z_LOWER_RAW_STEPS = 109000L;  // former +135 mm lower range

// ----------------------------------------------------------------------
// Clean raw-machine commissioning defaults
// ----------------------------------------------------------------------
// After each HOME switch contact the axis raw position is set to zero.
// These defaults permit the first commissioning attempt before the real
// physical zero, endpoints, and A2 gripper-clearance pose are saved.
//
// A1: away from switch = positive raw steps.
// A2: away from switch / positive joint side = negative raw steps.
constexpr long DEFAULT_A1_ZERO_RAW_STEPS = 25600L;
constexpr long DEFAULT_A2_ZERO_RAW_STEPS = -6400L;

constexpr long DEFAULT_A1_NEG_RAW_STEPS  = 2845L;
constexpr long DEFAULT_A1_POS_RAW_STEPS  = 49777L;

constexpr long DEFAULT_A2_NEG_RAW_STEPS  = -400L;
constexpr long DEFAULT_A2_POS_RAW_STEPS  = -12800L;
constexpr long DEFAULT_A2_PARK_RAW_STEPS = -12800L;

// ----------------------------------------------------------------------
// Primary calibration truth: raw machine positions from HOME switches
// ----------------------------------------------------------------------
// These values are saved in namespace "scara_raw_v2" by
// 02_CalibrationAndStorage.ino.
long a1ZeroSteps = DEFAULT_A1_ZERO_RAW_STEPS;
long a2ZeroSteps = DEFAULT_A2_ZERO_RAW_STEPS;

long a1NegativeLimitRawSteps = DEFAULT_A1_NEG_RAW_STEPS;
long a1PositiveLimitRawSteps = DEFAULT_A1_POS_RAW_STEPS;

long a2NegativeLimitRawSteps = DEFAULT_A2_NEG_RAW_STEPS;
long a2PositiveLimitRawSteps = DEFAULT_A2_POS_RAW_STEPS;

long a2HomeClearanceParkRawSteps = DEFAULT_A2_PARK_RAW_STEPS;

long zZeroSteps = DEFAULT_Z_ZERO_RAW_STEPS;
long zUpperLimitRawSteps = DEFAULT_Z_UPPER_RAW_STEPS;
long zLowerLimitRawSteps = DEFAULT_Z_LOWER_RAW_STEPS;

// Displayed/user coordinates are derived from the raw positions above.
// They are not the stored physical calibration truth.
float a1MinDeg = -160.0f;
float a1MaxDeg =  170.0f;
float a2MinDeg = -150.0f;
float a2MaxDeg =  160.0f;
float a2HomeClearanceParkDeg = 160.0f;
float zMinMm = -135.0f;
float zMaxMm =  135.0f;

// IK inputs from the web monitor are displayed in decimal millimeters.
// A point copied back from a safe endpoint can reconstruct a joint angle a few
// hundredths of a degree outside the exact software boundary.
// This tolerance is accepted only before clamping back to the safe boundary.
constexpr float IK_ROUNDING_JOINT_TOL_DEG = 0.50f;
constexpr float IK_MAX_CLAMP_POSITION_ERROR_MM = 1.00f;

// =====================================================
// WiFi: backup AP + shared robot-cell hotspot
// =====================================================
//
// Backup access stays available at 192.168.4.1.
// Main system access is the STA IP received from the ALSAADI hotspot.
// Use the STA IP in the MQTT coordinator GUI.

const char* AP_SSID = "SCARA-Robot";
const char* AP_PASS = "CHANGE_ME";

const char* STA_SSID = "YOUR_WIFI_SSID";
const char* STA_PASS = "YOUR_WIFI_PASSWORD";

constexpr unsigned long WIFI_STA_TIMEOUT_MS = 15000UL;

// HOME finish ready pose requested after returning rotary axes to calibrated zero.
// Z remains near the top HOME limit release position; it does NOT move down to Z=0.
constexpr float HOME_FINISH_A1_DEG = 130.0f;
constexpr float HOME_FINISH_A2_DEG = -136.0f;
constexpr bool HOME_FINISH_KEEP_Z_AT_TOP = true;

WebServer server(80);
Preferences calibrationStore;  // raw joint-machine calibration: namespace scara_raw_v2
Preferences motionStore;       // speed profile storage: namespace scara_motion
Preferences toolStore;         // tool TCP storage: namespace scara_tool
Preferences gripperStore;      // servo gripper positions: namespace scara_grip
Preferences collisionStore;    // Z-layered collision envelopes / startup park: namespace scara_env3
Preferences legacyCollisionStore; // v9.10 migration input: namespace scara_env2
Preferences packageRecipeStore;   // legacy only; recipes now stored in LittleFS /scara_recipes.dat
Preferences legacyPackageRecipeStore; // v10.5 migration source: namespace scara_pkg1

// =====================================================
// Motor objects
// =====================================================

AccelStepper arm1(AccelStepper::DRIVER, A1_STEP, A1_DIR);
AccelStepper arm2(AccelStepper::DRIVER, A2_STEP, A2_DIR);
AccelStepper zAxis(AccelStepper::DRIVER, Z_STEP, Z_DIR);

// =====================================================
// Motion settings - preserved from your working code
// =====================================================

// Faster normal FK/IK/real-time movement.
// Homing felt faster because it uses constant-speed runSpeed().
// Normal movement uses acceleration; increasing acceleration removes the long slow ramp.
constexpr float A1_MAX_SPEED = 9500.0f;   // about 66.8 deg/s
constexpr float A1_ACCEL     = 12000.0f;

constexpr float A2_MAX_SPEED = 7500.0f;   // about 187.5 deg/s
constexpr float A2_ACCEL     = 10000.0f;

constexpr float Z_MAX_SPEED  = 10000.0f;
constexpr float Z_ACCEL      = 5000.0f;

// User motion profiles. Percent scales the tested normal-movement ceilings above.
// Homing does not use these values.
enum MotionProfileMode {
  PROFILE_MANUAL_FK,
  PROFILE_REALTIME,
  PROFILE_IK,
  PROFILE_SETUP
};

// Percentage of the programmed NORMAL-MOTION ceiling, not motor capability.
// These are conservative default operating values; raise them after motion tests.
uint8_t manualFkSpeedPct = 60;   // Manual / FK point-to-point movement
uint8_t realtimeSpeedPct = 35;   // Real-Time slider dragging
uint8_t ikSpeedPct       = 60;   // IK point-to-point movement

MotionProfileMode activeMotionProfile = PROFILE_MANUAL_FK;
uint8_t activeMotionSpeedPct = 60;

// Automatic collision-safe routed moves. If a target is permitted but
// simultaneous A1/A2 movement can cross the box/base zone, travel through A2=0.
enum CollisionSafeRoutePhase {
  COLLISION_ROUTE_IDLE,
  COLLISION_ROUTE_CLEAR_A2_TO_ZERO,
  COLLISION_ROUTE_MOVE_A1_AT_ZERO,
  COLLISION_ROUTE_FINAL_TARGET
};

CollisionSafeRoutePhase collisionSafeRoutePhase = COLLISION_ROUTE_IDLE;
bool collisionSafeRouteActive = false;
float collisionRouteTargetA1Deg = 0.0f;
float collisionRouteTargetA2Deg = 0.0f;
float collisionRouteTargetZMm = 0.0f;
float collisionRouteTransitZMm = 0.0f;
MotionProfileMode collisionRouteProfile = PROFILE_MANUAL_FK;
String lastMotionPlan = "Direct motion";

constexpr uint8_t MIN_USER_SPEED_PCT = 10;
constexpr uint8_t MAX_USER_SPEED_PCT = 100;
constexpr uint8_t SETUP_JOG_SPEED_PCT = 20;

// Homing speeds. Homing is controlled separately from normal movement.
constexpr float A1_HOME_SPEED = 2600.0f;
constexpr float A2_HOME_SPEED = 2200.0f;
constexpr float Z_HOME_SPEED  = 5000.0f;

// Raw release distances used immediately after a switch is touched.
// These are intentionally independent from saved FK/IK calibration.
// A1 away from its switch = positive raw steps.
// A2 away from its switch = negative raw steps.
// Z  away from its top switch = positive raw steps.
constexpr long A1_HOME_RELEASE_STEPS = 1500L;  // about 10.5 deg of raw clearance
constexpr long A2_HOME_RELEASE_STEPS = 400L;   // about 10 deg of raw clearance
constexpr long Z_HOME_RELEASE_STEPS  = 1000L;  // 2.5 mm below top switch

// Minimum validation clearance for stored operating endpoints.
constexpr long A1_MIN_SAFE_RAW_CLEARANCE = 250L;
constexpr long A2_MIN_SAFE_RAW_CLEARANCE = 100L;
constexpr long Z_MIN_SAFE_RAW_CLEARANCE  = 800L;  // retain clearance below top switch

constexpr long A1_HOME_MAX_TRAVEL_STEPS = 65000L;
constexpr long A2_HOME_MAX_TRAVEL_STEPS = 20000L;
constexpr long Z_HOME_MAX_TRAVEL_STEPS  = 125000L;

constexpr unsigned long HOME_TIMEOUT_MS = 60000UL;
constexpr unsigned long MOVE_TIMEOUT_MS = 60000UL;

// DRV8825 idle policy:
// Release all three drivers after a completed movement so they can cool.
// Since steppers have no encoder, do not manually move the robot while motors are released.
// If the robot is moved by hand, press HOME again before using FK/IK.
constexpr unsigned long MOTOR_IDLE_OFF_MS = 1200UL;

// Active motion must call AccelStepper::run() frequently. The browser server
// is serviced between short motor-priority slices instead of before every
// possible step. This affects normal motion only; HOME remains unchanged.
constexpr unsigned long POINT_TO_POINT_MOTION_SLICE_US = 5000UL;  // FK and IK
constexpr unsigned long REALTIME_MOTION_SLICE_US       = 1500UL;  // keep target updates responsive

// DRV8825 STEP timing margin. TI requires minimum STEP high/low durations
// of 1.9 microseconds; 3 us is used for reliable ESP32 output.
constexpr unsigned int DRV8825_MIN_STEP_PULSE_US = 3;

// =====================================================
// State
// =====================================================

bool isHomed = false;          // true only when normal FK/IK operation is permitted
bool isHoming = false;         // true while the automatic HOME sequence is running
bool setupRequired = false;    // true after safe referencing when calibration must be repaired

// Each axis becomes referenced only after its HOME switch is touched and safely released.
// These states let Setup repair calibration without allowing unsafe FK/IK movement.
bool zReferenced = false;
bool a1Referenced = false;
bool a2Referenced = false;

bool motorsEnabled = false;
bool faultActive = false;
String faultMessage = "";
bool homeRequested = false;
unsigned long homeRequestedAt = 0;

unsigned long motionStartTime = 0;
unsigned long lastMotionActivityTime = 0;

// Peak commanded velocity captured for the current/most recent normal move.
// This verifies whether a motion profile is really being applied.
float movePeakA1DegS = 0.0f;
float movePeakA2DegS = 0.0f;
float movePeakZMmS   = 0.0f;

// =====================================================
// Structures
// =====================================================

struct Position {
  float x;
  float y;
  float z;
};

struct IKSolution {
  bool ok;
  float a1;
  float a2;
  String error;
};

// =====================================================
// Function declarations
// =====================================================

void enableMotors();
void disableMotors();
void emergencyStop(const String &reason);
void enterSetupRequired(const String &reason);
bool setupMovementAllowedForAxis(const String &axis);

void loadCalibration();
void saveCalibration();
void resetCalibration();
void updateDerivedJointCoordinates();

void loadMotionProfiles();
void saveMotionProfiles();
void loadToolSettings();
void saveToolSettings();

void loadGripperSettings();
void saveGripperSettings();
void resetGripperSettings();
bool validateGripperCalibration(uint8_t openDeg, uint8_t holdDeg, uint8_t maxCloseDeg, String &error);
bool gripperBegin(String &error);
bool commandGripperAngle(int angleDeg, String &error);
bool commandGripperOpen(String &error);
bool commandGripperHold(String &error);
uint32_t gripperAngleToDuty(uint8_t angleDeg);

void loadCollisionSettings();
void saveCollisionSettings();
void resetCollisionSettings();
void sortCollisionEnvelopePoints(CollisionEnvelopePoint points[], uint8_t count);
bool createCollisionEnvelopeLayer(uint8_t a1Region, uint8_t a2Bound,
                                  float zFromMm, float zToMm,
                                  uint8_t &createdIndex, String &error);
bool deleteCollisionEnvelopeLayer(uint8_t layerIndex, String &error);
bool addCollisionEnvelopePoint(uint8_t layerIndex, float a1Deg, float a2LimitDeg, String &error);
bool captureCurrentCollisionEnvelopePoint(uint8_t layerIndex, String &error);
bool deleteCollisionEnvelopePoint(uint8_t layerIndex, uint8_t pointIndex, String &error);
bool clearCollisionEnvelopeLayer(uint8_t layerIndex, String &error);
bool setCollisionEnvelopeMargin(float marginDeg, String &error);
bool setCollisionSafeCorridor(float corridorDeg, String &error);
bool setCollisionProtection(bool enabled, String &error);
bool saveCurrentStartupPark(String &error);
bool envelopeLimitAtPose(uint8_t layerIndex, float a1Deg, float zMm, float &limitDeg);
bool collisionPoseIsSafe(float a1Deg, float a2Deg, float zMm, int gripperDeg, String &error);
bool collisionPathIsSafe(float startA1Deg, float startA2Deg, float startZMm,
                         float targetA1Deg, float targetA2Deg, float targetZMm,
                         int gripperDeg, String &error);

const char* recipeStateName(RecipePlaybackState state);
const char* recipeCommandName(uint8_t command);
const char* recipeCommandDisplayName(uint8_t command);
uint8_t parseRecipeCommand(const String &value);
String activePackageRecipeName();
void loadPackageRecipes();
bool savePackageRecipes();
void clearAllPackageRecipes(bool clearLegacyStorage);
bool reloadPackageRecipesFromEsp32(String &statusMessage);

bool recipeFileSystemReady();
String recipeFileSystemError();
uint32_t recipeFsTotalBytes();
uint32_t recipeFsUsedBytes();
bool recipeFileExists();
uint32_t storedRecipeTotalBytes();
bool getStoredRecipeInfo(uint8_t &storedCount, uint32_t &storedCrc, uint32_t &storedFileBytes);
void handleRecipeCommitVerify();
bool createPackageRecipe(const String &name, uint8_t &createdIndex, String &error);
bool renamePackageRecipe(uint8_t recipeIndex, const String &name, String &error);
bool deletePackageRecipe(uint8_t recipeIndex, String &error);
bool addRecipeStep(uint8_t recipeIndex, const RecipeStep &step, uint8_t &createdIndex, String &error);
bool updateRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, const RecipeStep &step, String &error);
bool deleteRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, String &error);
bool reorderRecipeStep(uint8_t recipeIndex, uint8_t stepIndex, bool moveUp, String &error);
bool startPackageRecipe(uint8_t recipeIndex, uint16_t repeats, String &error);
bool stopPackageRecipe(const String &reason);
bool resetRecipeStatus(String &error);
void resetRecipeAfterHome();
void servicePackageRecipe();

const char* armJobStateName(ArmJobState state);
String activeArmJobRecipeName();
void initializeArmJobController();
void serviceArmJobController();
bool armJobLocksManualControl();
bool armJobHasAssignedJob();
bool loadArmJob(const String &jobId, const String &packageClass, uint8_t recipeIndex, String &error);
bool setArmJobWaitingForAmr(String &error);
bool confirmArmJobAmrArrival(String &error);
bool abortArmJob(String &error);
bool clearArmJobResult(String &error);
void armJobOnHomeComplete();
void armJobOnEmergencyStop(const String &reason);
void armJobOnPowerOff();


uint8_t getMotionProfilePercent(MotionProfileMode profile);
const char* getMotionProfileName(MotionProfileMode profile);
void applyMotionProfile(MotionProfileMode profile);
void applyFixedHomingMotionSettings();
bool zeroTargetsAreSafe(String &reason);
bool a2ParkingMappingIsSafe(String &reason);
bool operatingRawLimitsAreSafe(String &reason);
bool queueCalibrationJog(const String &axis, float amountToMove, String &error);
bool switchPressed(int pin);
bool anyMotion();
bool queueDirectJointMove(float a1Deg, float a2Deg, float zMM, MotionProfileMode profile, String &message);
bool startCollisionSafeRoute(float a1Deg, float a2Deg, float zMM, MotionProfileMode profile, String &message);
void serviceCollisionSafeRoute();
const char* collisionRoutePhaseName();
void runMotionTask();
void runMotionServiceSlice();
void resetMoveVelocityMonitor();
void updateMoveVelocityMonitor();
void checkLimitSafety();

long a1AngleToSteps(float angleDeg);
long a2AngleToSteps(float angleDeg);
long zMmToSteps(float zMM);
float a1StepsToAngle(long steps);
float a2StepsToAngle(long steps);
float zStepsToMm(long steps);
float actualA1();
float actualA2();
float actualZ();

Position forwardKinematics(float a1Deg, float a2Deg, float zMM);
float normalizeAngle(float angleDeg);
IKSolution solveIK(float x, float y, bool elbowDownConfiguration);
IKSolution selectNearestIKConfiguration(float x, float y);
bool queueJointMove(float a1Deg, float a2Deg, float zMM, MotionProfileMode profile, String &error);

bool runSingleBlocking(AccelStepper &motor, const char* name, unsigned long timeoutMS);
bool homeA1();
bool homeA2();
bool homeZ();
bool homeAll();

void handleRoot();
void handleNetworkStatus();
void handleMove();
void handleMoveRT();
void handleHome();
void handleIK();
void handlePosition();
void handleStop();
void handleOff();
void handleConfig();
void handleCalibrationJog();
void handleCalibrationSetZero();
void handleCalibrationSetLimit();
void handleCalibrationReset();
void handleMotionProfile();
void handleMotionProfileSave();
void handleToolTCPSet();
void handleA2HomeParkSet();
void handleGripperMove();
void handleGripperOpen();
void handleGripperHold();
void handleGripperSetCalibration();
void handleGripperResetCalibration();
void handleStartupConfirm();
void handleStartupParkSave();
void handleStartupParkGo();
void handlePackageRecipeCreate();
void handlePackageRecipeRename();
void handlePackageRecipeDelete();
void handleRecipeStepAdd();
void handleRecipeStepUpdate();
void handleRecipeStepDelete();
void handleRecipeStepReorder();
void handlePackageRecipePlay();
void handlePackageRecipeReset();
void handlePackageRecipeExport();
void handlePackageRecipeImport();
void handleRecipeSaveNow();
void handleRecipeCommitVerify();
void handleRecipeStorageStatus();
void handleRecipeReloadStorage();
void handleRecipeLibrary();
void handleRecipeClearAll();
void handleRecipeClearLegacy();
void handleJobLoad();
void handleJobWaitForAmr();
void handleJobAmrArrived();
void handleJobAbort();
void handleJobClear();
void handleJobStatus();
void handleCollisionLayerCreate();
void handleCollisionLayerDelete();
void handleCollisionEnvelopeAdd();
void handleCollisionEnvelopeCapture();
void handleCollisionEnvelopeDelete();
void handleCollisionEnvelopeClear();
void handleCollisionEnvelopeMargin();
void handleCollisionEnvelopeCorridor();
void handleCollisionSetProtection();
void handleCollisionLayerExport();
void handleCollisionLayerImport();
void handleCalibrationExport();
void handleCalibrationImport();

void printStatus();
void parseCommand(String cmd);

// =====================================================
// Main execution
// =====================================================
//
// setup(): runs once at power-up. It initializes stored configuration,
// motor drivers, the WiFi access point, and web routes.
//
// loop(): runs continuously. It services the web controller, starts a
// requested HOME sequence, runs motion updates, and checks serial commands.
//
// The implementations called here are located in the subsystem tabs.
// =====================================================


void setup() {
  Serial.begin(115200);

  // SAFETY FIRST: force all DRV8825 drivers disabled before Preferences,
  // WiFi, webserver, servo startup or any other slow boot work.
  // DRV8825 ENABLE is active LOW, so HIGH = OFF.
  digitalWrite(A1_EN, HIGH);
  digitalWrite(A2_EN, HIGH);
  digitalWrite(Z_EN, HIGH);
  pinMode(A1_EN, OUTPUT);
  pinMode(A2_EN, OUTPUT);
  pinMode(Z_EN, OUTPUT);
  disableMotors();

  loadCalibration();       // preserves saved geometric zero and safe endpoints
  loadMotionProfiles();    // speed settings are stored separately
  loadToolSettings();      // TCP and A2 HOME-clearance park pose
  loadGripperSettings();   // OPEN / HOLD / maximum-close servo positions
  loadCollisionSettings(); // Safe Startup Park and Z-layered collision envelopes
  loadPackageRecipes();    // taught box-type to shelf movement recipes
  initializeArmJobController(); // active jobs are RAM-only and never restart after power loss

  pinMode(A1_LIMIT, INPUT_PULLUP);
  pinMode(A2_LIMIT, INPUT_PULLUP);
  pinMode(Z_LIMIT, INPUT_PULLUP);

  disableMotors();

  // Direction settings kept from your working code.
  arm1.setPinsInverted(true, false, false);
  arm2.setPinsInverted(false, false, false);
  zAxis.setPinsInverted(false, false, false);

  arm1.setMaxSpeed(A1_MAX_SPEED);
  arm1.setAcceleration(A1_ACCEL);

  arm2.setMaxSpeed(A2_MAX_SPEED);
  arm2.setAcceleration(A2_ACCEL);

  zAxis.setMaxSpeed(Z_MAX_SPEED);
  zAxis.setAcceleration(Z_ACCEL);

  // DRV8825 requires wider STEP pulses than an A4988. This timing margin
  // prevents missed driver commands when normal motion is accelerated.
  arm1.setMinPulseWidth(DRV8825_MIN_STEP_PULSE_US);
  arm2.setMinPulseWidth(DRV8825_MIN_STEP_PULSE_US);
  zAxis.setMinPulseWidth(DRV8825_MIN_STEP_PULSE_US);

  applyMotionProfile(PROFILE_MANUAL_FK);

  // The servo is commanded OPEN at boot. Keep the gripper physically clear
  // during the first power-up after this firmware is installed.
  String gripperStartupError;
  if (!gripperBegin(gripperStartupError)) {
    Serial.print("Gripper startup fault: ");
    Serial.println(gripperStartupError);
  }

  WiFi.persistent(false);
  WiFi.setSleep(false);

  // Force a clean WiFi restart. This fixes many cases where the AP does not appear
  // after changing from old AP-only firmware to AP+STA firmware.
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(700);

  WiFi.mode(WIFI_AP_STA);
  delay(300);

  // Backup setup network. Fixed channel 6, visible SSID, max 4 stations.
  bool apStarted = WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);
  delay(500);

  Serial.print("Backup AP start result: ");
  Serial.println(apStarted ? "OK" : "FAILED");
  Serial.print("Backup AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Backup AP URL: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("Backup AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  // Main robot-cell network.
  WiFi.begin(STA_SSID, STA_PASS);

  Serial.print("Connecting to shared WiFi SSID: ");
  Serial.println(STA_SSID);

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_STA_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Shared WiFi STA IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Use in MQTT GUI Arm IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Shared WiFi failed. Backup AP should still be visible:");
    Serial.println("SSID: SCARA-Robot");
    Serial.println("URL:  http://192.168.4.1");
  }

  server.on("/", handleRoot);
  server.on("/net", HTTP_GET, handleNetworkStatus);
  server.on("/home", handleHome);
  server.on("/move", handleMove);
  server.on("/move_rt", handleMoveRT);
  server.on("/ik", handleIK);
  server.on("/position", handlePosition);
  server.on("/stop", handleStop);
  server.on("/off", handleOff);
  server.on("/config", handleConfig);
  server.on("/cal/jog", handleCalibrationJog);
  server.on("/cal/set_zero", handleCalibrationSetZero);
  server.on("/cal/set_limit", handleCalibrationSetLimit);
  server.on("/cal/reset", handleCalibrationReset);
  server.on("/speed/set", handleMotionProfile);
  server.on("/speed/save", handleMotionProfileSave);
  server.on("/tool/set_tcp", handleToolTCPSet);
  server.on("/tool/set_a2_home_park", handleA2HomeParkSet);
  server.on("/gripper/set", handleGripperMove);
  server.on("/gripper/open", handleGripperOpen);
  server.on("/gripper/hold", handleGripperHold);
  server.on("/gripper/cal/set", handleGripperSetCalibration);
  server.on("/gripper/cal/reset", handleGripperResetCalibration);
  server.on("/startup/confirm", handleStartupConfirm);
  server.on("/park/save", handleStartupParkSave);
  server.on("/park/go", handleStartupParkGo);
  server.on("/recipe/create", handlePackageRecipeCreate);
  server.on("/recipe/rename", handlePackageRecipeRename);
  server.on("/recipe/delete", handlePackageRecipeDelete);
  server.on("/recipe/step/add", handleRecipeStepAdd);
  server.on("/recipe/step/update", handleRecipeStepUpdate);
  server.on("/recipe/step/delete", handleRecipeStepDelete);
  server.on("/recipe/step/reorder", handleRecipeStepReorder);
  server.on("/recipe/play", handlePackageRecipePlay);
  server.on("/recipe/reset", handlePackageRecipeReset);
  server.on("/recipe/export", HTTP_GET, handlePackageRecipeExport);
  server.on("/recipe/import", HTTP_POST, handlePackageRecipeImport);
  server.on("/recipe/save-now", handleRecipeSaveNow);
  server.on("/recipe/commit-verify", handleRecipeCommitVerify);
  server.on("/recipe/storage-status", HTTP_GET, handleRecipeStorageStatus);
  server.on("/recipe/reload-storage", HTTP_GET, handleRecipeReloadStorage);
  server.on("/recipe/library", HTTP_GET, handleRecipeLibrary);
  server.on("/recipe/clear-all", handleRecipeClearAll);
  server.on("/recipe/clear-legacy", handleRecipeClearLegacy);
  server.on("/job/load", handleJobLoad);
  server.on("/job/wait-amr", handleJobWaitForAmr);
  server.on("/job/amr-arrived", handleJobAmrArrived);
  server.on("/job/abort", handleJobAbort);
  server.on("/job/clear", handleJobClear);
  server.on("/job/status", HTTP_GET, handleJobStatus);
  server.on("/collision/layer/create", handleCollisionLayerCreate);
  server.on("/collision/layer/delete", handleCollisionLayerDelete);
  server.on("/collision/envelope/add", handleCollisionEnvelopeAdd);
  server.on("/collision/envelope/capture", handleCollisionEnvelopeCapture);
  server.on("/collision/envelope/delete", handleCollisionEnvelopeDelete);
  server.on("/collision/envelope/clear", handleCollisionEnvelopeClear);
  server.on("/collision/envelope/margin", handleCollisionEnvelopeMargin);
  server.on("/collision/envelope/corridor", handleCollisionEnvelopeCorridor);
  server.on("/collision/protection", handleCollisionSetProtection);
  server.on("/collision/export", HTTP_GET, handleCollisionLayerExport);
  server.on("/collision/import", HTTP_POST, handleCollisionLayerImport);
  server.on("/cal/export", HTTP_GET, handleCalibrationExport);
  server.on("/cal/import", HTTP_POST, handleCalibrationImport);
  server.begin();

  Serial.println("SCARA v10.21 ready: compact persistent recipe store.");
}

void loop() {
  server.handleClient();

  if (homeRequested && !isHoming && (millis() - homeRequestedAt > 150UL)) {
    homeRequested = false;
    bool homeOk = homeAll();
    homeRequested = false;
    if (!homeOk) isHoming = false;
  }

  // Motor-service slicing lets AccelStepper reach the commanded normal
  // speeds while the web controller remains responsive between slices.
  runMotionServiceSlice();
  servicePackageRecipe();
  serviceArmJobController();

  if (Serial.available()) {
    parseCommand(Serial.readStringUntil('\n'));
  }
}
