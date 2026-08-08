# Troubleshooting — SCARA 

---

## HOME Won't Start

**"Confirm Safe Startup Pose before HOME"**

The startup pose confirmation is required before every HOME. Physically place the arm in the saved park position with the gripper open, then press **Confirm Safe Startup Pose** on the web page. HOME will remain blocked until this is done.

**"Abort the loaded job before HOME"**

A job is loaded or waiting for AMR. Use the Jobs panel to abort it, then HOME.

**"A recipe is playing. Press STOP before HOME"**

Stop recipe playback before homing.

---

## HOME Fails Mid-Sequence

**"A1/A2/Z homing timeout" or "homing max travel exceeded"**

The motor ran for too long without finding the limit switch. Check that the switch is wired correctly and triggers when the axis reaches it. Check that the motor is moving in the right direction for homing.

**"A1/A2/Z switch did not release after raw HOME backoff"**

The switch triggered successfully but did not release after the motor backed off. The switch may be stuck or the backoff distance is too small. Increase `A1_HOME_RELEASE_STEPS` / `A2_HOME_RELEASE_STEPS` / `Z_HOME_RELEASE_STEPS` in the main config.

**"A2 geometric zero raw position is unsafe"** or **"A2 HOME-clearance park raw position is unsafe"**

The stored A2 zero or park position is too close to the home switch to be used safely. Run Setup and recalibrate A2 zero and the clearance park position.

**Setup Required mode after HOME**

The arm homed successfully but calibration validation failed. The axes are referenced and you can use Setup jog to repair the affected values. Common causes: geometric zero too close to the switch, endpoints not set, or endpoints crossed. The serial monitor will print the specific reason.

---

## Motor and Motion Problems

**Motors won't enable / arm doesn't move**

Check the DRV8825 enable pin wiring. The firmware sets EN LOW to enable and HIGH to disable. Verify the stepper drivers have power and the fault/reset pins are held correctly for your driver.

**Motors disable after sitting idle**

This is expected. The firmware disables motors after the idle timeout (`MOTOR_IDLE_OFF_MS`) to reduce heat. Motion automatically re-enables them. If you moved the arm by hand while motors were off, HOME again before running any motion.

**"A1/A2/Z outside safe range"**

The requested angle or position is outside the calibrated limits. Check that joint limits are set correctly. If you are editing config constants (link lengths, steps per revolution), recalibrate after changing them.

**"Movement timeout"**

A move was queued but did not complete within the allowed time (`MOVE_TIMEOUT_MS`). The arm triggers an emergency stop. Possible causes: motor stall, obstruction, or the target position required more steps than the timeout allows at the configured speed. Check for mechanical binding or reduce speed.

**"A1/A2/Z limit touched outside HOME"**

A limit switch triggered during normal motion — not during homing. This is an emergency stop condition. The arm requires HOME before further motion. Possible causes: lost steps moving the arm beyond the calibrated endpoint, or the arm was moved by hand while motors were off.

---

## Collision Safety

**"Target is forbidden"**

The requested pose violates a collision envelope layer. The A2 angle at that A1 position and Z height is outside the permitted boundary. Either move to a different target or adjust the envelope in the Collision panel.

**"Cannot route safely to saved A2 corridor"**

The auto-safe routing system tried to plan a detour through the saved A2 corridor angle but the initial A2 move itself would violate the envelope. Set the corridor to an A2 angle that is clear at all A1 positions, or adjust the envelope boundaries.

**Collision protection won't enable**

At least one boundary point must be saved before enabling protection. If the current pose already violates a layer you are trying to enable, move to a permitted pose first.

---

## Recipe Problems

**"HOME the arm before playback"**

Recipes require the arm to be homed.

**"Enable collision protection before playback"**

Recipe execution requires active collision protection. Define and enable the collision envelope before running recipes.

**Recipe faults during playback**

The recipe state machine prints the failed step name and reason to the web status panel and to the serial monitor. Common causes: a step pose is now outside calibrated limits (calibration changed since the recipe was recorded), a step target violates the active collision envelope, or the gripper command failed.

**Recipes missing after restart**

Recipes are stored in LittleFS. If the filesystem failed to mount (wrong partition scheme), recipes cannot be saved or loaded. Use a partition scheme that includes a LittleFS filesystem region. The web interface's Storage Status panel shows filesystem state. Also use **Permanent Save + Verify** after any recipe changes to confirm they are written to flash.

---

## Web Interface

**Web page won't load**

The ESP32 runs an HTTP server on both AP and STA interfaces. Connect to the ESP32's access point or the local network it joined. The default AP SSID is set in `AP_SSID` in the main config. Check that you are on the correct network and using the correct IP address. The IP is printed to the serial monitor on startup.

**Status stops updating**

The web interface polls `/position` on a timer. If the ESP32 is busy with a blocking operation (such as LittleFS file write), polling may be briefly delayed. If it stops permanently, check the serial monitor for fault messages or restart the ESP32.

---

## Calibration

**Calibration resets unexpectedly**

If the NVS namespace changes between firmware versions, the firmware ignores the old data and uses defaults. This is intentional — the `scara_raw_v2` namespace was introduced specifically to avoid inheriting angle-based limits from older versions. After any firmware update, verify calibration and re-save if needed.

**Limits appear wrong after recalibrating zero**

Degree limits are derived at runtime from the raw endpoints and the current zero. If you move the zero, the displayed degree limits will change even though the raw limits (physical positions) are unchanged. This is correct behavior. Verify the physical range of motion matches the displayed limits.

**Export / Import calibration**

Always export a calibration backup before making significant changes. The exported JSON includes all axis calibration, gripper settings, motion profiles, collision envelope, and the startup park pose. Import restores all of these and triggers an emergency stop requiring HOME.
