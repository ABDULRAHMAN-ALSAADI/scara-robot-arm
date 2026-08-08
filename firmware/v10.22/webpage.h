#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SCARA Robot - Safe Motion Foundation</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,Segoe UI,sans-serif;background:#101114;color:#eee;min-height:100vh}
header{background:#17191e;border-bottom:1px solid #2c3038;padding:14px 22px;display:flex;align-items:center;gap:12px}
header h1{font-size:1.1rem;letter-spacing:.7px}
.dot{width:10px;height:10px;border-radius:50%;background:#777}
.container{display:grid;grid-template-columns:440px 1fr;gap:16px;padding:18px;max-width:1120px;margin:0 auto}
@media(max-width:900px){.container{grid-template-columns:1fr}}
.card{background:#17191e;border:1px solid #2c3038;border-radius:12px;padding:18px}
.card h2{font-size:.78rem;letter-spacing:1.4px;text-transform:uppercase;color:#87909e;margin-bottom:14px}
.warn{padding:10px;border-radius:8px;background:#28200e;border:1px solid #624917;color:#e1b75c;font-size:.82rem;margin-bottom:14px}
.warn.hidden{display:none}
.mode-tabs{display:flex;flex-wrap:wrap;border:1px solid #303641;border-radius:8px;overflow:hidden;margin-bottom:16px}
.mode-tab{flex:1 1 88px;padding:9px 5px;border:0;background:#111318;color:#87909e;font-weight:700;cursor:pointer;font-size:.72rem}
.mode-tab.active{background:#e7ebf0;color:#101114}
.ik-panel{display:none;padding-bottom:4px}
.ik-row{display:flex;align-items:center;gap:8px;margin-bottom:10px}
.ik-row label{width:70px;color:#aab1bd;font-size:.83rem}
.ik-row input{flex:1;padding:7px;border-radius:6px;background:#101217;border:1px solid #303641;color:#fff}
.branch{display:flex;gap:8px;flex:1}
.branch button{flex:1;border:1px solid #303641;border-radius:6px;padding:7px;background:#101217;color:#aab1bd;cursor:pointer}
.branch button.active{background:#e7ebf0;color:#101114;font-weight:700}
.ik-result{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;margin-bottom:12px;font-size:.82rem;min-height:38px;color:#aab1bd}
.ik-result.ok{color:#77cd8b}
.ik-result.err{color:#ee7777}
.solution-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:10px}
.solution-card{display:block;width:100%;min-height:72px;text-align:left;padding:9px;border:1px solid #303641;border-radius:8px;background:#101217;color:#aab1bd;cursor:pointer}
.solution-card.active{border-color:#e7ebf0;background:#20242b;color:#fff}
.solution-card.invalid{opacity:.48;cursor:not-allowed}
.solution-title{display:block;font-size:.77rem;font-weight:700;color:#e7ebf0;margin-bottom:5px}
.solution-values{font-size:.73rem;line-height:1.35}
.small-btn{width:100%;padding:8px;border:1px solid #303641;border-radius:7px;background:#101217;color:#e7ebf0;font-weight:700;cursor:pointer;margin-bottom:10px}
.target-fk{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:8px 10px;color:#aab1bd;font-size:.78rem;margin-bottom:12px}
.speed-control{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:11px;margin-bottom:14px}
.speed-head{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;font-size:.82rem;color:#aab1bd}
.speed-head strong{color:#fff}
.speed-rates{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px;font-size:.68rem;color:#87909e;text-align:center}
.speed-save{margin-top:10px;width:100%;padding:8px;border:1px solid #303641;border-radius:7px;background:#1c2027;color:#e7ebf0;font-weight:700;cursor:pointer}
.profile-badge{display:inline-block;padding:3px 7px;border-radius:10px;background:#303641;color:#e7ebf0;font-size:.68rem}
.gripper-control{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:11px;margin-bottom:14px}
.gripper-buttons{display:flex;gap:8px;margin-top:10px}
.gripper-buttons button{flex:1;padding:10px;border:1px solid #303641;border-radius:7px;background:#1c2027;color:#e7ebf0;font-weight:700;cursor:pointer}
.gripper-buttons .hold{background:#e7ebf0;color:#101114}
.startup-control{background:#16202a;border:1px solid #394d62;border-radius:8px;padding:11px;margin-bottom:14px}
.startup-control strong{font-size:.82rem;color:#e7ebf0}
.startup-buttons{display:flex;gap:7px;margin-top:9px}
.startup-buttons button{flex:1;padding:9px;border:1px solid #394d62;border-radius:7px;background:#202d39;color:#e7ebf0;font-size:.72rem;font-weight:700;cursor:pointer}
.startup-buttons .confirm{background:#e7ebf0;color:#101114}
.collision-status{margin-top:8px;padding:8px;border-radius:7px;background:#141a21;border:1px solid #303641;font-size:.73rem;color:#aab1bd}
.collision-status.enabled{color:#77cd8b;border-color:#385a40}
.collision-status.disabled{color:#e1b75c;border-color:#624917}
.point-list{margin-top:8px;font-size:.73rem;color:#aab1bd;line-height:1.55;background:#101217;padding:8px;border-radius:7px;border:1px solid #303641}
.collision-panel{display:none}
.zone-editor{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;margin:10px 0}
.zone-axis{margin:10px 0;padding-bottom:8px;border-bottom:1px solid #232833}.zone-axis:last-child{border-bottom:0}
.zone-title{font-size:.77rem;font-weight:700;color:#e7ebf0;margin-bottom:7px}
.zone-values{display:grid;grid-template-columns:1fr 76px 20px 1fr 76px;gap:6px;align-items:center}
.zone-values input[type=number]{width:100%;padding:6px;border:1px solid #303641;background:#111318;color:#fff;border-radius:5px;text-align:right}
.zone-values input[type=range]{width:100%}
.zone-table-row{background:#101217;border:1px solid #303641;border-radius:7px;padding:8px;margin-top:7px;font-size:.74rem;line-height:1.5}
.rule-box{background:#0d151b;border:1px solid #315063;border-radius:8px;padding:9px;margin:8px 0;font-size:.76rem;line-height:1.42;color:#d8e0eb}
.rule-box strong{color:#fff}
.local-status{margin:8px 0;padding:9px;border-radius:7px;border:1px solid #303641;font-size:.76rem;color:#aab1bd;background:#101217}
.local-status.ok{color:#77cd8b;border-color:#385a40}
.local-status.err{color:#ee7777;border-color:#633940}
.local-status.busy{color:#e1b75c;border-color:#624917}
.rule-row{border:1px solid #303641;background:#101217;border-radius:8px;padding:9px;margin-top:7px;font-size:.75rem;line-height:1.5}
.rule-row strong{color:#fff}
.rule-row button{float:right;border:1px solid #4a515e;background:#1c2027;color:#fff;border-radius:5px;padding:4px 8px;cursor:pointer}
.z-toggle{display:flex;align-items:center;gap:8px;font-size:.76rem;color:#d8e0eb;margin:9px 0}
.z-toggle input{width:auto}
.z-disabled{opacity:.45;pointer-events:none}
.storage-panel{display:none}
.storage-card{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:10px;margin-bottom:8px}
.storage-card h3{font-size:.78rem;color:#fff;margin-bottom:6px}
.storage-card pre{white-space:pre-wrap;word-break:break-word;font-size:.73rem;line-height:1.48;color:#aab1bd;font-family:Consolas,monospace}
.storage-actions{display:flex;gap:7px;margin-bottom:10px}
.storage-actions button{flex:1;padding:9px;border:1px solid #303641;background:#1c2027;color:#eee;border-radius:6px;cursor:pointer;font-weight:700}
.env-select{display:flex;gap:7px;margin:9px 0}.env-select button{flex:1;padding:9px;border:1px solid #303641;border-radius:6px;background:#1c2027;color:#eee;font-weight:700;cursor:pointer}.env-select button.active{background:#e7ebf0;color:#101114}
.env-current{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin:8px 0}.env-current div{background:#101217;border:1px solid #303641;border-radius:6px;padding:8px;text-align:center;font-size:.72rem}.env-current strong{display:block;color:#fff;font-size:.94rem}
.env-point{background:#101217;border:1px solid #303641;border-radius:7px;padding:8px;margin-top:6px;font-size:.75rem}.env-point button{float:right;background:#1c2027;color:#fff;border:1px solid #4a515e;border-radius:5px;padding:4px 8px}
.env-limit{background:#0d151b;border:1px solid #315063;border-radius:7px;padding:8px;margin:8px 0;font-size:.76rem;color:#d8e0eb}
.layer-row{background:#101217;border:1px solid #303641;border-radius:8px;padding:8px;margin:7px 0;font-size:.75rem;line-height:1.45}
.layer-row.selected{border-color:#dfe5ed;background:#171b22}
.layer-row button{margin:5px 5px 0 0;background:#1c2027;color:#fff;border:1px solid #4a515e;border-radius:5px;padding:5px 9px;cursor:pointer}
.layer-create{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;margin-top:8px}
.recipe-panel{display:none}
.recipe-note{background:#101217;border:1px solid #303641;border-radius:8px;padding:9px;color:#c7ced8;font-size:.76rem;line-height:1.5;margin-bottom:9px}
.recipe-row{display:flex;gap:6px;align-items:center;margin:8px 0}
.recipe-row input,.recipe-row select{flex:1;min-width:0;padding:9px;background:#111318;color:#fff;border:1px solid #303641;border-radius:6px}
.motion-method{background:#111318;border:1px solid #303641;border-radius:8px;padding:9px;margin:9px 0;font-size:.75rem;line-height:1.5;color:#c7ced8}
.motion-method select{width:100%;margin-top:7px;padding:9px;background:#0e1115;color:#fff;border:1px solid #303641;border-radius:6px}
.move-tag{display:inline-block;margin-top:4px;padding:2px 7px;border-radius:10px;background:#25323e;color:#8fb7e8;font-size:.67rem;font-weight:700}
.transition-path{margin-top:8px;padding:8px;background:#0d1117;border:1px solid #303641;border-radius:7px}
.transition-path label{display:block;color:#bfc7d1;font-size:.69rem;font-weight:700;margin-bottom:5px}
.transition-path select{width:100%;padding:7px;background:#111318;color:#fff;border:1px solid #303641;border-radius:6px;margin-bottom:6px}
.transition-path button{width:100%;margin:0;padding:7px 8px;background:#233247;border-color:#48627d}
.path-warning{background:#241c10;border:1px solid #624917;color:#e1b75c;border-radius:8px;padding:9px;margin-top:9px;font-size:.73rem;line-height:1.5}
.recipe-row button,.recipe-buttons button,.recipe-play button,.recipe-backup button{padding:9px;border:1px solid #303641;background:#1c2027;color:#fff;border-radius:6px;cursor:pointer;font-size:.73rem;font-weight:700}
.recipe-buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:6px;margin:8px 0}
.wide-task-button{width:100%;padding:10px;border:1px solid #48627d;background:#233247;color:#fff;border-radius:6px;cursor:pointer;font-size:.73rem;font-weight:700;margin-top:7px}
.recipe-live{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin:9px 0}
.recipe-live div{background:#101217;border:1px solid #303641;border-radius:7px;padding:8px;text-align:center;color:#87909e;font-size:.68rem}
.recipe-live strong{display:block;color:#fff;font-size:.9rem;margin-top:4px}
.recipe-pose-card{background:#101217;border:1px solid #303641;border-radius:8px;padding:9px;margin-top:7px;font-size:.73rem;line-height:1.55}
.recipe-pose-card.selected{border-color:#e7ebf0;background:#171b22}
.recipe-pose-card button{padding:5px 8px;margin:6px 5px 0 0;border:1px solid #4a515e;border-radius:5px;background:#1c2027;color:#fff;cursor:pointer}
.recipe-play{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;margin:9px 0}
.recipe-play input{width:72px;padding:8px;background:#111318;color:#fff;border:1px solid #303641;border-radius:6px;margin:0 7px}
.recipe-monitor{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;font-size:.77rem;line-height:1.7}
.recipe-log{background:#0e1115;border:1px solid #303641;border-radius:8px;padding:9px;min-height:88px;font-family:Consolas,monospace;font-size:.69rem;line-height:1.55;color:#aab1bd;white-space:pre-wrap;margin-top:8px}
.recipe-backup{display:flex;flex-wrap:wrap;gap:6px;margin-top:8px}
.recipe-backup input{width:100%;margin-top:6px}
.library-card{background:#101217;border:1px solid #303641;border-radius:8px;padding:9px;margin-top:7px;font-size:.73rem;line-height:1.5}.library-card.selected{border-color:#e7ebf0;background:#171b22}.library-card strong{color:#fff}.library-card button{padding:5px 8px;margin:6px 5px 0 0;border:1px solid #4a515e;border-radius:5px;background:#1c2027;color:#fff;cursor:pointer}.library-warning{background:#241010;border:1px solid #633940;color:#ee7777;border-radius:8px;padding:9px;margin-top:8px;font-size:.73rem;line-height:1.5}
.step-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin:8px 0}
.step-grid label{font-size:.67rem;color:#87909e;display:block}
.step-grid input{width:100%;box-sizing:border-box;padding:8px;margin-top:4px;background:#111318;color:#fff;border:1px solid #303641;border-radius:6px}
.step-card{background:#101217;border:1px solid #303641;border-radius:8px;padding:9px;margin-top:7px;font-size:.73rem;line-height:1.5}
.step-card.selected{border-color:#e7ebf0;background:#171b22}
.step-card .cmd{display:inline-block;padding:2px 7px;border-radius:10px;background:#25323e;color:#8fb7e8;font-size:.67rem;font-weight:700;margin:4px 0}
.step-card button{padding:5px 8px;margin:6px 5px 0 0;border:1px solid #4a515e;border-radius:5px;background:#1c2027;color:#fff;cursor:pointer}
.step-builder-actions{display:grid;grid-template-columns:repeat(3,1fr);gap:6px;margin-top:8px}
.step-builder-actions button,.small-button{padding:9px;border:1px solid #303641;background:#1c2027;color:#fff;border-radius:6px;cursor:pointer;font-size:.73rem;font-weight:700}
.small-button{width:100%;margin-top:7px}
.job-panel{display:none}
.job-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:7px;margin:8px 0}
.job-grid label{font-size:.68rem;color:#87909e;display:block}
.job-grid input,.job-grid select{width:100%;box-sizing:border-box;padding:9px;margin-top:4px;background:#111318;color:#fff;border:1px solid #303641;border-radius:6px}
.job-actions{display:grid;grid-template-columns:repeat(2,1fr);gap:7px;margin:10px 0}
.job-actions button{padding:10px;border:1px solid #303641;background:#1c2027;color:#fff;border-radius:6px;cursor:pointer;font-size:.73rem;font-weight:700}
.job-actions .run{background:#213324;border-color:#3c6745}
.job-actions .danger{background:#342122;border-color:#674044}
.job-state{background:#101217;border:1px solid #303641;border-radius:8px;padding:10px;line-height:1.7;font-size:.76rem}
.job-state .value{color:#cdd4dd}
.job-log{background:#0e1115;border:1px solid #303641;border-radius:8px;padding:9px;min-height:80px;font-family:Consolas,monospace;font-size:.69rem;line-height:1.55;color:#aab1bd;white-space:pre-wrap;margin-top:8px}
@media(max-width:540px){.job-grid,.job-actions{grid-template-columns:1fr}}

@media(max-width:540px){.step-grid{grid-template-columns:repeat(2,1fr)}.step-builder-actions{grid-template-columns:1fr}}

@media(max-width:540px){.recipe-live{grid-template-columns:repeat(2,1fr)}}

.cal-panel{display:none}
.cal-step{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:11px;margin-bottom:10px}
.cal-step h3{font-size:.78rem;color:#e7ebf0;margin-bottom:7px}
.cal-step p{font-size:.74rem;line-height:1.45;color:#aab1bd;margin-bottom:8px}
.cal-buttons{display:grid;grid-template-columns:repeat(2,1fr);gap:6px}
.cal-buttons button{padding:8px;border:1px solid #303641;background:#1c2027;color:#eee;border-radius:6px;cursor:pointer;font-size:.75rem;font-weight:700}
.cal-buttons button:hover{background:#303641}
.cal-readout{font-size:.74rem;color:#77cd8b;margin-top:7px}
.cal-input-row{display:flex;gap:8px;align-items:center;margin-bottom:8px}
.cal-input-row label{font-size:.75rem;color:#aab1bd;flex:1}
.cal-input-row input{width:95px;padding:7px;border:1px solid #303641;background:#101217;color:#fff;border-radius:6px;text-align:right}
.cal-action{width:100%;padding:8px;border:1px solid #303641;background:#1c2027;color:#eee;border-radius:6px;cursor:pointer;font-size:.75rem;font-weight:700;margin-top:5px}
.axis-row{margin-bottom:16px}
.axis-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:7px;color:#aab1bd;font-size:.84rem}
.axis-input{width:72px;padding:5px 7px;border-radius:6px;border:1px solid #303641;background:#101217;color:#fff;text-align:right}
input[type=range]{width:100%;height:5px;-webkit-appearance:none;appearance:none;background:#2c3038;border-radius:3px;outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:17px;height:17px;border-radius:50%;background:#fff;border:0;cursor:pointer}
input[type=range]::-moz-range-thumb{width:17px;height:17px;border-radius:50%;background:#fff;border:0;cursor:pointer}
input[type=range]::-moz-range-track{height:5px;background:#2c3038;border-radius:3px}
.safe-note{font-size:.73rem;color:#8d98a8;margin-top:5px}
.btn-row{display:flex;gap:8px;margin-top:14px}
.btn{border:0;border-radius:8px;padding:11px 12px;font-weight:700;cursor:pointer}
.send{flex:1;background:#e7ebf0;color:#101114}
.home{background:#3c4656;color:#fff}
.stop{background:#922d32;color:#fff;flex:1}
.off{background:#303641;color:#fff;flex:1}
.btn:disabled{opacity:.45;cursor:not-allowed}
.status{margin-top:12px;font-size:.8rem;color:#87909e;min-height:20px}
.status.ok{color:#77cd8b}
.status.err{color:#ee7777}
.status.busy{color:#e1b75c}
.value-grid{display:grid;grid-template-columns:repeat(5,1fr);gap:8px;margin-bottom:16px}
@media(max-width:600px){.value-grid{grid-template-columns:repeat(3,1fr)}}
.value-box{background:#101217;border:1px solid #2c3038;border-radius:8px;padding:9px;text-align:center}
.value-box .name{font-size:.67rem;color:#87909e;margin-bottom:4px}
.value-box .value{font-size:1.1rem;font-weight:700}
.value-box .unit{font-size:.65rem;color:#697384}
.state-line{font-size:.8rem;color:#aab1bd;margin-bottom:12px;padding:8px;background:#101217;border-radius:8px;border:1px solid #2c3038}
canvas{width:100%;height:340px;border-radius:9px;background:#101217;border:1px solid #2c3038;display:block}
</style>
</head>
<body>
<header><div class="dot" id="dot"></div><h1>SCARA Robot — v10.9 / Arm Job Controller</h1></header>
<div class="container">
  <div class="card">
    <h2>Joint Control</h2>
    <div class="warn" id="warn">HOME is blocked until the physical Safe Startup Pose is confirmed.</div>
    <div class="startup-control">
      <strong>Startup Safety</strong>
      <div class="safe-note" id="startupStatus">Place the arm in the safe startup posture, then confirm to unlock HOME.</div>
      <div class="startup-buttons">
        <button class="confirm" onclick="confirmStartupPose()">Confirm Safe Startup Pose — Enable HOME</button>
        <button onclick="sendParkForPowerOff()">PARK FOR POWER-OFF</button>
      </div>
      <div class="safe-note">Visual confirmation only; the robot has no absolute joint feedback.</div>
    </div>
    <div class="mode-tabs">
      <button class="mode-tab active" id="tabManual" onclick="setMode('manual')">Manual</button>
      <button class="mode-tab" id="tabRT" onclick="setMode('realtime')">Real-Time</button>
      <button class="mode-tab" id="tabIK" onclick="setMode('ik')">IK</button>
      <button class="mode-tab" id="tabCal" onclick="setMode('calibration')">Setup</button>
      <button class="mode-tab" id="tabCollision" onclick="setMode('collision')">Collision</button>
      <button class="mode-tab" id="tabRecipes" onclick="setMode('recipes')">Recipes</button>
      <button class="mode-tab" id="tabRecipeRun" onclick="setMode('recipeRun')">Recipe Run</button>
      <button class="mode-tab" id="tabJobs" onclick="setMode('jobs')">Jobs</button>
      <button class="mode-tab" id="tabStorage" onclick="setMode('storage')">Stored Data</button>
    </div>

    <div id="speedPanel" class="speed-control">
      <div class="speed-head"><span id="speedProfileLabel">Manual / FK Motion Scale</span><strong id="speedPctLabel">60%</strong></div>
      <input id="motionSpeed" type="range" min="10" max="100" step="1" value="60" oninput="speedChanged()" onchange="commitSpeedNow(true)">
      <div class="speed-rates">
        <div>A1 max<br><span id="a1MaxRate">--</span> °/s</div>
        <div>A2 max<br><span id="a2MaxRate">--</span> °/s</div>
        <div>Z max<br><span id="zMaxRate">--</span> mm/s</div>
      </div>
      <div class="safe-note" id="speedSendStatus" style="margin-top:8px;color:#77cd8b">Motion scale loaded from controller.</div>
      <button class="speed-save" onclick="saveSpeedProfiles()">Save Speed Profiles</button>
      <div class="safe-note" id="speedLockNote" style="margin-top:8px">Normal motion only. HOME speed is fixed.</div>
    </div>

    <div id="ikPanel" class="ik-panel">
      <div class="ik-row"><label>X (mm)</label><input type="number" id="ikX" value="180" step="0.01" oninput="clearIkRoundTripReference();previewIK()"></div>
      <div class="ik-row"><label>Y (mm)</label><input type="number" id="ikY" value="0" step="0.01" oninput="clearIkRoundTripReference();previewIK()"></div>
      <div class="ik-row"><label>Z (mm)</label><input type="number" id="ikZ" value="0" min="-135" max="135" step="1"></div>
      <button class="small-btn" onclick="loadActualPoseIntoIK()">Load Actual Pose into IK — Preserve Configuration</button>
      <button class="small-btn" onclick="loadFkTargetIntoIK()">Load FK Slider Target into IK — Round-Trip Test</button>
      <div class="ik-row"><label>Selection</label>
        <div class="branch">
          <button id="nearestBtn" class="active" onclick="selectConfiguration('nearest')">Closest Posture</button>
        </div>
      </div>
      <div class="solution-grid">
        <button id="elbowDownCard" class="solution-card" onclick="selectConfiguration('elbow_down')">
          <span class="solution-title">Elbow-Down Configuration</span>
          <span class="solution-values" id="elbowDownValues">--</span>
        </button>
        <button id="elbowUpCard" class="solution-card" onclick="selectConfiguration('elbow_up')">
          <span class="solution-title">Elbow-Up Configuration</span>
          <span class="solution-values" id="elbowUpValues">--</span>
        </button>
      </div>
      <div id="ikResult" class="ik-result">Enter an XY target.</div>
      <div class="safe-note" style="margin-bottom:12px">Choose the posture that matches the required approach.</div>
    </div>

    <div id="calPanel" class="cal-panel">
      <div class="cal-step">
        <h3>Setup State</h3>
        <div class="cal-readout" id="setupStateReadout">Run HOME to establish raw switch references.</div>
      </div>

      <div class="cal-step">
        <h3>Step 1 — Geometric Zero</h3>
        <p>HOME first. Set A1 and A2 geometric zero positions.</p>
        <div class="cal-buttons">
          <button onclick="calJog('a1',-2)">A1 −2°</button><button onclick="calJog('a1',2)">A1 +2°</button>
          <button onclick="calJog('a1',-0.5)">A1 −0.5°</button><button onclick="calJog('a1',0.5)">A1 +0.5°</button>
          <button onclick="setZero('a1')">Set Current A1 = 0°</button><button onclick="setZero('a2')">Set Current A2 = 0°</button>
          <button onclick="calJog('a2',-2)">A2 −2°</button><button onclick="calJog('a2',2)">A2 +2°</button>
          <button onclick="calJog('a2',-0.5)">A2 −0.5°</button><button onclick="calJog('a2',0.5)">A2 +0.5°</button>
        </div>
      </div>
      <div class="cal-step">
        <h3>Step 2 — Safe Endpoints</h3>
        <p>Save physical travel limits with clearance from switches and collisions.</p>
        <div class="cal-buttons">
          <button onclick="setLimit('a1','min')">Set A1 Negative Limit</button><button onclick="setLimit('a1','max')">Set A1 Positive Limit</button>
          <button onclick="setLimit('a2','min')">Set A2 Negative Limit</button><button onclick="setLimit('a2','max')">Set A2 Positive Limit</button>
        </div>
        <div class="cal-readout" id="calReadout">Loading saved limits...</div>
      </div>
      <div class="cal-step">
        <h3>Step 3 — Z Axis Safety / Ground Protection</h3>
        <p>Set Z upper and lower limits. Z− goes up; Z+ goes down.</p>
        <div class="cal-buttons">
          <button onclick="calJog('z',-10)">Z Up −10 mm</button><button onclick="calJog('z',10)">Z Down +10 mm</button>
          <button onclick="calJog('z',-1)">Z Up −1 mm</button><button onclick="calJog('z',1)">Z Down +1 mm</button>
          <button onclick="setZero('z')">Set Current Z = 0 mm</button><button onclick="setLimit('z','top')">Set Z Upper Safe Limit</button>
          <button onclick="setLimit('z','bottom')">Set Z Lower / Ground Limit</button>
        </div>
        <div class="cal-readout" id="zCalReadout">Loading Z safety limits...</div>
      </div>
      <div class="cal-step">
        <h3>Step 4 — Gripper TCP and HOME Clearance</h3>
        <p>Set tool-center distance and the A2 clearance pose used during HOME.</p>
        <div class="cal-input-row"><label>L2 / Tool Center Point (mm)</label><input id="tcpLengthInput" type="number" min="20" max="250" step="0.01" value="120.00"></div>
        <button class="cal-action" onclick="saveTcpLength()">Save TCP Length for FK / IK</button>

        <button class="cal-action" onclick="saveA2HomePark()">Set Current A2 as A1-HOME Clearance Pose</button>
        <div class="cal-readout" id="toolReadout">Loading gripper settings...</div>
      </div>
      <div class="cal-step">
        <h3>Step 5 — Servo Gripper Calibration</h3>
        <p>GPIO 13 signal. Save OPEN, BOX HOLD and MAX CLOSE positions.</p>
        <div class="cal-buttons">
          <button onclick="saveGripperPose('open')">Set Current as OPEN</button><button onclick="saveGripperPose('hold')">Set Current as BOX HOLD</button>
          <button onclick="saveGripperPose('maxclose')">Set Current as MAX CLOSE</button><button onclick="resetGripperCalibration()">Reset Gripper Defaults</button>
        </div>
        <div class="cal-readout" id="gripperCalReadout">Loading servo gripper calibration...</div>
      </div>
      <div class="cal-step">
        <h3>Step 6 — Safe Startup Park</h3>
        <p>Save the pose used before power-off and before HOME.</p>
        <button class="cal-action" onclick="saveStartupPark()">Set Current Pose as Safe Startup / Power-Off Park</button>
        <div class="cal-readout" id="parkReadout">No startup park loaded.</div>

      </div>
      <div class="cal-step">
        <h3>Step 7 — Calibration Backup / Restore</h3>
        <p>Export or restore all saved setup values and collision rules.</p>
        <button class="cal-action" onclick="downloadCalibrationBackup()">Download Calibration Backup (.json)</button>
        <div class="cal-input-row" style="margin-top:10px"><label>Restore backup file</label><input id="calBackupFile" type="file" accept=".json,application/json" style="width:210px"></div>
        <button class="cal-action" onclick="restoreCalibrationBackup()">Restore Calibration Backup</button>
        <div class="safe-note" style="margin-top:8px">Restore requires HOME afterward.</div>
      </div>
      <div class="cal-step">
        <h3>Reset</h3>
        <p>Restore default setup values.</p>
        <div class="cal-buttons"><button onclick="resetCalibration()">Reset Calibration Defaults</button></div>
      </div>
    </div>

    <div id="collisionPanel" class="collision-panel">
      <div class="cal-step">
        <h3>Z-Layered Collision Envelope</h3>
        <div class="rule-box">Each layer defines one A1→A2 boundary active only inside its Z range. Your existing tables migrate into <strong>Z −135 to 0 mm</strong>. Add new layers for <strong>Z 0 to 32 mm</strong>.</div>
        <div id="collisionActionStatus" class="local-status">No action yet.</div>
        <div id="currentEnvelopeLimit" class="env-limit">Current pose: no active envelope restriction.</div>
      </div>

      <div class="cal-step">
        <h3>1. Create Z Layer</h3>
        <div class="layer-create">
          <div class="env-select">
            <button id="layerPositiveBtn" class="active" onclick="selectLayerRegion('positive')">POSITIVE A1 SIDE</button>
            <button id="layerNegativeBtn" onclick="selectLayerRegion('negative')">NEGATIVE A1 SIDE</button>
          </div>
          <div class="env-select" style="margin-top:7px">
            <button id="layerMaxBtn" class="active" onclick="selectLayerBound('max')">BLOCK A2 ABOVE LIMIT</button>
            <button id="layerMinBtn" onclick="selectLayerBound('min')">BLOCK A2 BELOW LIMIT</button>
          </div>
          <div class="zone-axis" style="margin-top:10px"><div class="zone-title">Layer active when Z is between</div><div class="zone-values">
            <input id="layerZFromS" type="range" step="0.01" oninput="layerZSliderChanged('From')"><input id="layerZFromN" type="number" step="0.01" onchange="layerZNumberChanged('From')"><span>and</span>
            <input id="layerZToS" type="range" step="0.01" oninput="layerZSliderChanged('To')"><input id="layerZToN" type="number" step="0.01" onchange="layerZNumberChanged('To')">
          </div></div>
          <button class="cal-action" onclick="createEnvelopeLayer()">CREATE Z LAYER</button>
        </div>
        <div id="envelopeLayerList">Loading layers...</div>
      </div>

      <div class="cal-step">
        <h3>2. Add Boundary Points to Selected Layer</h3>
        <div id="selectedLayerReadout" class="rule-box">Select a layer first.</div>
        <div class="env-current">
          <div>Actual A1<strong id="envActualA1">--</strong>°</div>
          <div>Actual A2<strong id="envActualA2">--</strong>°</div>
          <div>Actual Z<strong id="envActualZ">--</strong> mm</div>
        </div>
        <div class="zone-editor">
          <div class="zone-axis"><div class="zone-title">A1 boundary position</div><div class="zone-values">
            <input id="envA1S" type="range" step="0.01" oninput="envSliderChanged('A1')"><input id="envA1N" type="number" step="0.01" onchange="envNumberChanged('A1')"><span></span><span></span><span></span>
          </div></div>
          <div class="zone-axis"><div class="zone-title" id="envA2Label">A2 boundary at this A1</div><div class="zone-values">
            <input id="envA2S" type="range" step="0.01" oninput="envSliderChanged('A2')"><input id="envA2N" type="number" step="0.01" onchange="envNumberChanged('A2')"><span></span><span></span><span></span>
          </div></div>
        </div>
        <div class="cal-buttons">
          <button onclick="saveEnteredEnvelopePoint()">Save Entered Point</button>
          <button onclick="captureActualEnvelopePoint()">Capture Actual Robot Pose</button>
          <button onclick="clearSelectedLayerPoints()">Clear Layer Points</button>
          <button onclick="setMode('manual')">Go to Manual / FK</button>
        </div>
        <div id="selectedLayerPoints" class="point-list">No layer selected.</div>
      </div>

      <div class="cal-step">
        <h3>3. Collision Layer Backup</h3>
        <button class="cal-action" onclick="downloadCollisionLayerBackup()">Download Collision Layers (.json)</button>
        <input id="collisionLayerBackupFile" class="restore-file" type="file" accept=".json,application/json">
        <button class="cal-action" onclick="restoreCollisionLayerBackup()">Restore Collision Layers</button>
        <div class="safe-note">Imports only Z-layer collision maps, margin and safe-route corridor. On a replacement ESP32, restore or calibrate A1/A2/Z first.</div>
      </div>

      <div class="cal-step">
        <h3>4. Protection and Safe Route</h3>
        <div class="cal-input-row"><label>Extra A2 margin (deg)</label><input id="envMargin" type="number" min="0" max="20" step="0.1" value="0"></div>
        <button class="cal-action" onclick="saveEnvelopeMargin()">Save Extra Margin</button>
        <div class="cal-input-row" style="margin-top:8px"><label>Safe-route A2 corridor</label><input id="envCorridor" type="number" step="0.1" value="0"></div>
        <button class="cal-action" onclick="saveEnvelopeCorridor()">Save Safe Corridor</button>
        <div class="cal-buttons">
          <button onclick="setCollisionProtection(true)">Enable Protection</button>
          <button onclick="setCollisionProtection(false)">Disable Protection</button>
        </div>
        <div id="collisionState" class="collision-status disabled">Protection disabled.</div>
        <div class="safe-note">When a move changes Z, every layer crossed by that Z movement is checked.</div>
        <div class="cal-readout" id="routeReadout">Motion plan: idle.</div>
      </div>
      <div class="safe-note">A1 endpoint correction: if FK shows −134.49° but the real safe negative limit is −133.49°, disable protection, HOME, move A1 to the safe endpoint and press <strong>Setup → Set A1 Negative Limit</strong>.</div>
    </div>
    <div id="recipesPanel" class="recipe-panel">
      <div class="cal-step">
        <h3>Recipes</h3>
        <div id="recipeLocalStatus" class="local-status">Create or select a recipe.</div>
        <div class="recipe-row">
          <input id="recipeNameInput" maxlength="37" value="" placeholder="Recipe name exactly as system will call it">
          <button onclick="createRecipe()">New</button>
        </div>
        <div class="recipe-row">
          <select id="recipeSelect" onchange="selectRecipe()"></select>
          <button onclick="renameRecipe()">Rename</button>
          <button onclick="deleteRecipe()">Delete</button>
        </div>
      </div>
      <div class="cal-step">
        <h3>Step Builder</h3>
        <div class="recipe-live">
          <div>A1 °<strong id="recipeLiveA1">--</strong></div>
          <div>A2 °<strong id="recipeLiveA2">--</strong></div>
          <div>Z mm<strong id="recipeLiveZ">--</strong></div>
          <div>Grip °<strong id="recipeLiveGrip">--</strong></div>
        </div>
        <button class="small-button" onclick="copyLiveStepTargets()">Copy Current Robot Values</button>
        <div class="recipe-row"><input id="recipeStepNameInput" maxlength="31" value="LIFT_BOX" placeholder="Step name"></div>
        <div class="recipe-row">
          <select id="recipeCommandInput" onchange="updateStepEditor()">
            <option value="MOVE_SAFE_XYZ">Move Safe A1 / A2 / Z — same as FK</option>
            <option value="MOVE_A1">Move A1 only</option>
            <option value="MOVE_A2">Move A2 only</option>
            <option value="MOVE_Z">Move Z only</option>
            <option value="MOVE_A1_A2">Move A1 + A2 together</option>
            <option value="MOVE_XYZ_DIRECT">Move A1 + A2 + Z together — direct</option>
            <option value="GRIPPER">Set gripper</option>
            <option value="WAIT">Wait only</option>
          </select>
        </div>
        <div class="step-grid">
          <label>A1 target °<input id="stepA1" type="number" step="0.01" value="0"></label>
          <label>A2 target °<input id="stepA2" type="number" step="0.01" value="0"></label>
          <label>Z target mm<input id="stepZ" type="number" step="0.01" value="0"></label>
          <label>Gripper after step °<input id="stepGrip" type="number" step="1" value="0"></label>
        </div>
        <div class="recipe-row">
          <label style="font-size:.73rem;color:#aab1bd">Pause after (s)</label>
          <input id="stepWaitSeconds" type="number" min="0" max="30" step="0.1" value="0">
        </div>
        <div class="step-builder-actions">
          <button onclick="addRecipeStep()">Add Step</button>
          <button onclick="updateSelectedRecipeStep()">Update Selected</button>
          <button onclick="clearStepEditor()">Clear</button>
        </div>
      </div>
      <div class="cal-step">
        <h3>Timeline</h3>
        <div id="recipeStepList">No recipe selected.</div>
      </div>

      <div class="cal-step">
        <h3>Saved Recipe Library in Arm ESP32</h3>
        <div class="recipe-note">This is the real recipe library stored in the arm ESP32 compact per-recipe store. Nothing is created automatically.</div>
        <div id="recipeLibraryList" class="point-list">Press Refresh Saved Library.</div>
        <div class="recipe-row">
          <input id="recipeLibraryRenameInput" maxlength="37" placeholder="New name for selected saved recipe">
          <button onclick="renameSavedLibraryRecipe()">Rename Saved</button>
          <button onclick="deleteSavedLibraryRecipe()">Delete Saved</button>
        </div>
        <div class="recipe-backup">
          <button onclick="refreshSavedRecipeLibrary()">Refresh Saved Library</button>
          <button onclick="commitRecipesToEsp32()">Permanent Save + Verify</button>
          <button onclick="clearSavedRecipeLibrary()">Clear All Saved Recipes</button>
          <button onclick="clearLegacyRecipesOnly()">Clear Old Ghost Storage</button>
        </div>
        <div id="recipeLibraryStatus" class="local-status">Saved library not checked.</div>
      </div>
      <div class="cal-step">
        <h3>Backup</h3>
        <div class="recipe-backup">
          <button onclick="commitRecipesToEsp32()">Permanent Save + Verify</button>
          <button onclick="saveRecipesToEsp32()">Save Only</button>
          <button onclick="reloadRecipesFromEsp32()">Reload from ESP32</button>
          <button onclick="checkRecipeStorage()">Check ESP32 Storage</button>
          <button onclick="downloadRecipesJson()">Download JSON</button>
          <button onclick="restoreRecipesJson()">Restore JSON</button>
          <input id="recipesJsonFile" type="file" accept=".json,application/json">
          <div id="recipeStorageStatus" class="local-status" style="width:100%;margin-top:6px">ESP32 storage not checked.</div>
        </div>
      </div>
    </div>
    <div id="recipeRunPanel" class="recipe-panel">
      <div class="cal-step">
        <h3>Recipe Run</h3>
        <div class="recipe-row"><select id="recipeRunSelect" onchange="selectRunRecipe()"></select></div>
        <div class="recipe-play">
          Repeat <input id="recipeRepeatInput" type="number" min="1" max="100" value="1"> time(s)
          <div class="recipe-buttons" style="margin-top:10px">
            <button onclick="playSelectedRecipe()">Play</button>
            <button onclick="sendStop()">STOP</button>
            <button onclick="resetRecipePlayback()">Reset</button>
            <button onclick="setMode('recipes')">Edit</button>
          </div>
        </div>
        <div class="recipe-monitor">
          Recipe: <strong id="recipeMonitorName">NONE</strong><br>
          State: <strong id="recipeMonitorState">IDLE</strong><br>
          Step: <strong id="recipeMonitorStep">--</strong><br>
          Action: <strong id="recipeMonitorMotion">--</strong><br>
          Status: <span id="recipeMonitorStatus">Idle.</span>
        </div>
        <div id="recipeLog" class="recipe-log">No recipe events.</div>
      </div>
    </div>

    <div id="jobsPanel" class="job-panel">
      <div class="cal-step">
        <h3>Arm Job Control</h3>
        <div id="jobLocalStatus" class="local-status">HOME the arm to enter READY.</div>
        <div class="job-grid">
          <label>Job ID<input id="jobIdInput" maxlength="29" value="JOB_001"></label>
          <label>Package class<input id="jobClassInput" maxlength="33" value="RED_SQUARE"></label>
        </div>
        <div class="job-grid" style="grid-template-columns:1fr">
          <label>Assigned recipe<select id="jobRecipeSelect"></select></label>
        </div>
        <div class="job-actions">
          <button onclick="loadArmJob()">Load Job</button>
          <button onclick="waitForAmr()">Wait for AMR</button>
          <button class="run" onclick="simulateAmrArrived()">Simulate AMR Arrived</button>
          <button class="danger" onclick="abortArmJob()">Abort Job</button>
          <button onclick="clearArmJob()">Clear Result</button>
        </div>
      </div>
      <div class="cal-step">
        <h3>Status</h3>
        <div class="job-state">
          State: <strong id="jobState">NOT_HOMED</strong><br>
          Job: <span class="value" id="jobMonitorId">--</span><br>
          Package: <span class="value" id="jobMonitorClass">--</span><br>
          Recipe: <span class="value" id="jobMonitorRecipe">--</span><br>
          AMR confirmed: <span class="value" id="jobMonitorAmr">No</span><br>
          Recipe step: <span class="value" id="jobMonitorStep">--</span><br>
          Result: <strong id="jobMonitorResult">--</strong><br>
          Message: <span class="value" id="jobMonitorMessage">--</span>
        </div>
        <div id="jobLog" class="job-log">No autonomous job events.</div>
      </div>
    </div>

    <div id="storagePanel" class="storage-panel">
      <div class="cal-step">
        <h3>Preferences Storage</h3>
        <div class="storage-actions">
          <button onclick="loadConfiguration()">Refresh Stored Data</button>
          <button onclick="downloadCalibrationBackup()">Download Full Backup</button>
          <button onclick="downloadCollisionLayerBackup()">Download Collision Layers</button>
        </div>
        <div class="safe-note">Stored by firmware using &lt;Preferences.h&gt;.</div>
      </div>
      <div id="storageContent">Loading...</div>
    </div>
    <div class="axis-row joint-row">
      <div class="axis-header"><label>Arm 1 / Shoulder</label><input id="a1Num" class="axis-input" type="number" min="-160" max="170" value="0" onchange="numberChanged()"></div>
      <input id="a1" type="range" min="-160" max="170" value="0" step="1" oninput="sliderChanged()">
      <div class="safe-note" id="a1RangeNote">Calibrated safe range loaded from raw endpoints.</div>
    </div>

    <div class="axis-row joint-row">
      <div class="axis-header"><label>Arm 2 / Elbow</label><input id="a2Num" class="axis-input" type="number" min="-150" max="160" value="0" onchange="numberChanged()"></div>
      <input id="a2" type="range" min="-150" max="160" value="0" step="1" oninput="sliderChanged()">
      <div class="safe-note" id="a2RangeNote">Calibrated safe range loaded from raw endpoints.</div>
    </div>

    <div class="axis-row joint-row">
      <div class="axis-header"><label>Z Axis</label><input id="zNum" class="axis-input" type="number" min="-135" max="135" value="0" onchange="numberChanged()"></div>
      <input id="z" type="range" min="-135" max="135" value="0" step="1" oninput="sliderChanged()">
      <div class="safe-note" id="zRangeNote">Z calibrated limits load after connection.</div>
    </div>
    <div class="target-fk">Slider target FK: X <span id="targetX">--</span> mm &nbsp; Y <span id="targetY">--</span> mm &nbsp; Z <span id="targetZ">--</span> mm</div>

    <div class="gripper-control" id="gripperPanel">
      <div class="speed-head"><span>Servo Gripper</span><strong><span id="gripperAngleLabel">0</span>° command</strong></div>
      <input id="gripperSlider" type="range" min="0" max="90" step="1" value="0" oninput="gripperSliderChanged()" onchange="sendGripperSliderNow()">
      <div class="gripper-buttons">
        <button onclick="sendGripperOpen()">OPEN</button>
        <button class="hold" onclick="sendGripperHold()">CLOSE / HOLD BOX</button>
      </div>
      <div class="safe-note" id="gripperStatus">Saved defaults: OPEN 0° | BOX HOLD 70° | MAX CLOSE 90°. 180° is blocked.</div>
    </div>

    <div class="btn-row">
      <button id="sendBtn" class="btn send" onclick="sendManual()">Send Target</button>
      <button id="homeBtn" class="btn home" onclick="sendHome()">HOME</button>
    </div>
    <div class="btn-row">
      <button class="btn stop" onclick="sendStop()">STOP</button>
      <button class="btn off" onclick="sendOff()">MOTORS OFF</button>
    </div>
    <div class="status" id="msg">Connecting...</div>
    <div class="safe-note" style="margin-top:10px">Drivers rest when idle. Re-home after manual movement.</div>
  </div>

  <div class="card">
    <h2>Actual Position Monitor</h2>
    <div class="value-grid">
      <div class="value-box"><div class="name">A1</div><div class="value" id="actA1">--</div><div class="unit">deg</div></div>
      <div class="value-box"><div class="name">A2</div><div class="value" id="actA2">--</div><div class="unit">deg</div></div>
      <div class="value-box"><div class="name">X</div><div class="value" id="actX">--</div><div class="unit">mm</div></div>
      <div class="value-box"><div class="name">Y</div><div class="value" id="actY">--</div><div class="unit">mm</div></div>
      <div class="value-box"><div class="name">Z</div><div class="value" id="actZ">--</div><div class="unit">mm</div></div>
      <div class="value-box"><div class="name">GRIPPER</div><div class="value" id="actGrip">--</div><div class="unit">cmd deg</div></div>
    </div>
    <div class="target-fk">Active motion profile: <span class="profile-badge" id="activeProfile">--</span> &nbsp; Motion scale: <strong id="activePct">--%</strong><br>
      Live commanded velocity: A1 <span id="liveA1Speed">0.00</span> °/s &nbsp; | &nbsp; A2 <span id="liveA2Speed">0.00</span> °/s &nbsp; | &nbsp; Z <span id="liveZSpeed">0.00</span> mm/s<br>
      Peak of last move: A1 <span id="peakA1Speed">0.00</span> °/s &nbsp; | &nbsp; A2 <span id="peakA2Speed">0.00</span> °/s &nbsp; | &nbsp; Z <span id="peakZSpeed">0.00</span> mm/s
    </div>
    <div class="state-line" id="stateLine">State: unknown</div><div class="safe-note" style="margin-bottom:12px">Operating limits: A1 −160° to +170° | A2 −150° to +160°. HOME switches are never normal target positions.</div>
    <canvas id="cv" height="340"></canvas>
  </div>
</div>

<script>
const L1 = 136.5;
let L2 = 120.0; // TCP length, loaded from Setup / ESP32
let A1_MIN = -160, A1_MAX = 170;
let A2_MIN = -150, A2_MAX = 160;
let Z_MIN = -135, Z_MAX = 135;
const IK_JOINT_TOL = 0.50;
const IK_MAX_POSITION_ERROR = 1.00;

let mode = 'manual';
let homed = false;
let setupRequired = false;
let references = {z:false, a1:false, a2:false};
let homingPending = false;
let busy = false;
let ikConfiguration = 'nearest';
let ikRoundTripReference = null;
let lastActual = {a1:0, a2:0, x:0, y:0, z:0};
let speeds = {manual:60, realtime:35, ik:60};
let gripper = {open:0, hold:70, maxClose:90, command:0, attached:false};
let gripperUpdateTimer = null;
let gripperEditActiveUntil = 0;
let gripperRequestInFlight = false;
let safety = {startupConfirmed:false, parkSaved:false, collisionEnabled:false, margin:0};
let layerDraftRegion = 'positive';
let layerDraftBound = 'max';
let selectedEnvelopeLayer = -1;
let storedConfig = null;
let layerEditorReady = false;
let packageRecipes = [];
let selectedRecipeIndex = -1;
let selectedRecipeStep = -1;
let selectedLibraryRecipe = -1;
let savedRecipeLibrary = [];
let recipeLiveValues = {a1:0,a2:0,z:0,gripper:0};
let recipeLogLines = [];
let lastRecipeEvent = '';
let jobLogLines = [];
let lastJobEvent = '';
let speedUpdateTimer = null;

// Prevent the periodic /position poll from writing an older value over the
// slider while a new value is being dragged or confirmed by the controller.
let speedEditActiveUntil = 0;
let speedRequestInFlight = false;
let lastSpeedRequestId = 0;

let rtTimer = null;

const A1_RATE_AT_100 = 9500 / 142.22;
const A2_RATE_AT_100 = 7500 / 40.0;
const Z_RATE_AT_100 = 10000 / 400.0;

function clamp(v, lo, hi){ return Math.max(lo, Math.min(hi, v)); }

function speedModeKey() {
  if (mode === 'realtime') return 'realtime';
  if (mode === 'ik') return 'ik';
  return 'manual';
}

function updateSpeedPanel() {
  const panel = document.getElementById('speedPanel');
  const slider = document.getElementById('motionSpeed');
  if (mode === 'calibration' || mode === 'collision' || mode === 'recipes' || mode === 'recipeRun' || mode === 'storage') {
    panel.style.display = 'none';
    return;
  }
  panel.style.display = 'block';
  const key = speedModeKey();
  const pct = speeds[key];
  slider.value = pct;
  document.getElementById('speedProfileLabel').textContent =
    key === 'manual' ? 'Manual / FK Motion Scale' : (key === 'realtime' ? 'Real-Time Motion Scale' : 'IK Motion Scale');
  document.getElementById('speedPctLabel').textContent = pct + '%';
  const factor = pct / 100;
  document.getElementById('a1MaxRate').textContent = (A1_RATE_AT_100 * factor).toFixed(1);
  document.getElementById('a2MaxRate').textContent = (A2_RATE_AT_100 * factor).toFixed(1);
  document.getElementById('zMaxRate').textContent = (Z_RATE_AT_100 * factor).toFixed(1);
}

function speedChanged() {
  if (mode === 'calibration') return;

  const key = speedModeKey();
  const pct = +document.getElementById('motionSpeed').value;

  speeds[key] = pct;
  speedEditActiveUntil = Date.now() + 1200;
  updateSpeedPanel();

  document.getElementById('speedSendStatus').textContent =
    'Sending ' + pct + '% ' + (key === 'manual' ? 'Manual / FK' : (key === 'realtime' ? 'Real-Time' : 'IK')) + ' motion scale...';

  // Capture this input value. Do not reread speeds[key] later: polling may
  // receive an older controller state while the user is still dragging.
  clearTimeout(speedUpdateTimer);
  speedUpdateTimer = setTimeout(() => commitSpeedNow(false, key, pct), 90);
}

async function commitSpeedNow(fromRelease = false, capturedKey = null, capturedPct = null) {
  if (mode === 'calibration') return;

  const key = capturedKey || speedModeKey();
  const pct = Number.isFinite(capturedPct) ? capturedPct : +document.getElementById('motionSpeed').value;
  const requestId = ++lastSpeedRequestId;

  speedRequestInFlight = true;
  speedEditActiveUntil = Date.now() + 1200;

  try {
    const r = await fetch('/speed/set?mode=' + key + '&pct=' + pct + '&_=' + Date.now(), {cache:'no-store'});
    const d = await r.json();

    if (requestId !== lastSpeedRequestId) return;

    if (!d.ok) {
      document.getElementById('speedSendStatus').textContent = 'Motion scale update rejected.';
      setMessage('err', d.err);
      return;
    }

    speeds[key] = d.pct;
    updateSpeedPanel();

    const profileName = key === 'manual' ? 'Manual / FK' : (key === 'realtime' ? 'Real-Time' : 'IK');
    document.getElementById('speedSendStatus').textContent =
      profileName + ' motion scale accepted: ' + d.pct + '%' +
      (d.appliedNow ? ' — active now.' : ' — used on the next move in this mode.');
  } catch(e) {
    document.getElementById('speedSendStatus').textContent = 'Motion scale connection error.';
    setMessage('err', 'Speed update connection error.');
  } finally {
    if (requestId === lastSpeedRequestId) {
      speedRequestInFlight = false;
      speedEditActiveUntil = Date.now() + 500;
    }
  }
}

async function saveSpeedProfiles() {
  try {
    const r = await fetch('/speed/save', {cache:'no-store'});
    const d = await r.json();
    if (d.ok) {
      document.getElementById('speedSendStatus').textContent = 'Motion scale profiles saved in ESP32 memory.';
      setMessage('ok', 'Motion speed profiles saved.');
    } else {
      setMessage('err', d.err || 'Unable to save speed profiles.');
    }
  } catch(e) {
    setMessage('err', 'Speed save connection error.');
  }
}

function applyGripperState(data, updateCommand=true) {
  if (!Number.isFinite(data.gripperOpenDeg)) return;

  gripper.open = data.gripperOpenDeg;
  gripper.hold = data.gripperHoldDeg;
  gripper.maxClose = data.gripperMaxCloseDeg;
  gripper.attached = data.gripperAttached !== false;

  const slider = document.getElementById('gripperSlider');
  slider.min = gripper.open;
  slider.max = gripper.maxClose;

  if (updateCommand && !gripperRequestInFlight && Date.now() > gripperEditActiveUntil && Number.isFinite(data.gripperCommandDeg)) {
    gripper.command = data.gripperCommandDeg;
    slider.value = gripper.command;
  }

  document.getElementById('gripperAngleLabel').textContent = slider.value;
  document.getElementById('gripperStatus').textContent =
    'OPEN ' + gripper.open + '° | BOX HOLD ' + gripper.hold + '° | MAX CLOSE ' + gripper.maxClose + '°. 180° is blocked.';

  const setupReadout = document.getElementById('gripperCalReadout');
  if (setupReadout) {
    setupReadout.textContent =
      'Saved servo poses: OPEN ' + gripper.open + '° | BOX HOLD ' + gripper.hold +
      '° | MAX CLOSE ' + gripper.maxClose + '° | Signal GPIO 13';
  }
}

function gripperSliderChanged() {
  const deg = +document.getElementById('gripperSlider').value;
  document.getElementById('gripperAngleLabel').textContent = deg;
  gripperEditActiveUntil = Date.now() + 1000;
  clearTimeout(gripperUpdateTimer);
  gripperUpdateTimer = setTimeout(() => sendGripperAngle(deg), 80);
}

function sendGripperSliderNow() {
  clearTimeout(gripperUpdateTimer);
  sendGripperAngle(+document.getElementById('gripperSlider').value);
}

async function sendGripperAngle(deg) {
  gripperRequestInFlight = true;
  gripperEditActiveUntil = Date.now() + 800;
  try {
    const r = await fetch('/gripper/set?deg=' + deg + '&_=' + Date.now(), {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    gripper.command = d.deg;
    document.getElementById('gripperAngleLabel').textContent = d.deg;
    setMessage('ok', 'Gripper command: ' + d.deg + '°.');
  } catch(e) {
    setMessage('err', 'Gripper connection error.');
  } finally {
    gripperRequestInFlight = false;
  }
}

async function sendGripperOpen() {
  try {
    const r = await fetch('/gripper/open', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    document.getElementById('gripperSlider').value = d.deg;
    document.getElementById('gripperAngleLabel').textContent = d.deg;
    setMessage('ok', 'Gripper OPEN: ' + d.deg + '°.');
  } catch(e) { setMessage('err', 'Gripper OPEN connection error.'); }
}

async function sendGripperHold() {
  try {
    const r = await fetch('/gripper/hold', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    document.getElementById('gripperSlider').value = d.deg;
    document.getElementById('gripperAngleLabel').textContent = d.deg;
    setMessage('ok', 'Gripper BOX HOLD: ' + d.deg + '°.');
  } catch(e) { setMessage('err', 'Gripper HOLD connection error.'); }
}

async function saveGripperPose(pose) {
  try {
    const r = await fetch('/gripper/cal/set?pose=' + pose, {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    await loadConfiguration();
    setMessage('ok', 'Gripper saved: OPEN ' + d.open + '°, HOLD ' + d.hold + '°, MAX CLOSE ' + d.maxclose + '°.');
  } catch(e) { setMessage('err', 'Cannot save gripper calibration.'); }
}

async function resetGripperCalibration() {
  try {
    const r = await fetch('/gripper/cal/reset', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    await loadConfiguration();
    setMessage('ok', d.msg);
  } catch(e) { setMessage('err', 'Cannot reset gripper calibration.'); }
}

function setCollisionLocalStatus(type,text) {
  const el=document.getElementById('collisionActionStatus'); if(!el)return;
  el.className='local-status'+(type?' '+type:''); el.textContent=text;
}
function limitValue(value,min,max){ return Math.max(min,Math.min(max,value)); }
function layerBoundSymbol(bound){ return bound==='max' ? '≤' : '≥'; }
function layerDescription(layer) {
  const side=layer.region==='positive'?'Positive A1':'Negative A1';
  const action=layer.bound==='max'?'Block A2 Above':'Block A2 Below';
  return side+' / '+action+' / Z '+layer.zFrom.toFixed(2)+' to '+layer.zTo.toFixed(2)+' mm';
}
function selectLayerRegion(region) {
  layerDraftRegion=region;
  document.getElementById('layerPositiveBtn').className=region==='positive'?'active':'';
  document.getElementById('layerNegativeBtn').className=region==='negative'?'active':'';
}
function selectLayerBound(bound) {
  layerDraftBound=bound;
  document.getElementById('layerMaxBtn').className=bound==='max'?'active':'';
  document.getElementById('layerMinBtn').className=bound==='min'?'active':'';
}
function setLayerZLimits(reset=false) {
  ['From','To'].forEach((key,index)=>{
    const s=document.getElementById('layerZ'+key+'S'), n=document.getElementById('layerZ'+key+'N');
    if(!s||!n)return;
    s.min=Z_MIN; s.max=Z_MAX; n.min=Z_MIN; n.max=Z_MAX;
    const defaultValue=index===0 ? limitValue(0,Z_MIN,Z_MAX) : limitValue(32,Z_MIN,Z_MAX);
    const value=reset || !layerEditorReady || !Number.isFinite(Number(n.value)) ? defaultValue : limitValue(Number(n.value),Z_MIN,Z_MAX);
    s.value=value; n.value=Number(value).toFixed(2);
  });
  layerEditorReady=true;
}
function layerZSliderChanged(key){ document.getElementById('layerZ'+key+'N').value=Number(document.getElementById('layerZ'+key+'S').value).toFixed(2); }
function layerZNumberChanged(key){ const s=document.getElementById('layerZ'+key+'S'),n=document.getElementById('layerZ'+key+'N'); const v=limitValue(Number(n.value),Number(s.min),Number(s.max));s.value=v;n.value=v.toFixed(2); }
function selectedLayer() {
  if(!storedConfig || !Array.isArray(storedConfig.envLayers)) return null;
  return storedConfig.envLayers.find(l=>l.index===selectedEnvelopeLayer) || null;
}
function selectEnvelopeLayer(index) {
  selectedEnvelopeLayer=index;
  renderEnvelopeLayers(storedConfig);
  setPointEditorLimits(true);
}
function setPointEditorLimits(reset=false) {
  const layer=selectedLayer();
  const a1=document.getElementById('envA1S'), a1n=document.getElementById('envA1N'), a2=document.getElementById('envA2S'), a2n=document.getElementById('envA2N');
  if(!a1||!a1n||!a2||!a2n)return;
  const minA1=layer && layer.region==='positive'?Math.max(0,A1_MIN):A1_MIN;
  const maxA1=layer && layer.region==='negative'?Math.min(0,A1_MAX):A1_MAX;
  a1.min=minA1; a1.max=maxA1; a1n.min=minA1; a1n.max=maxA1;
  a2.min=A2_MIN; a2.max=A2_MAX; a2n.min=A2_MIN; a2n.max=A2_MAX;
  const a1Value=reset ? limitValue(lastActual.a1,minA1,maxA1) : limitValue(Number(a1n.value||lastActual.a1),minA1,maxA1);
  const a2Value=reset ? limitValue(lastActual.a2,A2_MIN,A2_MAX) : limitValue(Number(a2n.value||lastActual.a2),A2_MIN,A2_MAX);
  a1.value=a1Value; a1n.value=Number(a1Value).toFixed(2); a2.value=a2Value; a2n.value=Number(a2Value).toFixed(2);
  const label=document.getElementById('envA2Label');
  if(label) label.textContent=layer ? ('A2 '+(layer.bound==='max'?'maximum':'minimum')+' permitted at this A1') : 'A2 boundary at this A1';
}
function envSliderChanged(key){ document.getElementById('env'+key+'N').value=Number(document.getElementById('env'+key+'S').value).toFixed(2); }
function envNumberChanged(key){ const s=document.getElementById('env'+key+'S'),n=document.getElementById('env'+key+'N'); const v=limitValue(Number(n.value),Number(s.min),Number(s.max));s.value=v;n.value=v.toFixed(2); }
function renderEnvelopeLayers(c) {
  const list=document.getElementById('envelopeLayerList');
  if(!list || !c) return;
  const layers=Array.isArray(c.envLayers)?c.envLayers:[];
  if(selectedEnvelopeLayer>=0 && !layers.some(l=>l.index===selectedEnvelopeLayer)) selectedEnvelopeLayer=-1;
  if(selectedEnvelopeLayer<0 && layers.length) selectedEnvelopeLayer=layers[0].index;
  list.innerHTML=layers.length ? layers.map(layer=>'<div class="layer-row '+(layer.index===selectedEnvelopeLayer?'selected':'')+'"><strong>Layer '+(layer.index+1)+':</strong> '+layerDescription(layer)+' | '+layer.points.length+' points<br><button onclick="selectEnvelopeLayer('+layer.index+')">Select</button><button onclick="deleteEnvelopeLayer('+layer.index+')">Delete Layer</button></div>').join('') : 'No layers saved.';
  const chosen=selectedLayer();
  const readout=document.getElementById('selectedLayerReadout');
  const points=document.getElementById('selectedLayerPoints');
  if(!chosen) {
    if(readout) readout.textContent='Select or create a Z layer first.';
    if(points) points.textContent='No layer selected.';
    return;
  }
  if(readout) readout.innerHTML='<strong>Selected Layer '+(chosen.index+1)+':</strong> '+layerDescription(chosen)+'<br>Allowed A2 must stay '+layerBoundSymbol(chosen.bound)+' interpolated boundary while Z is inside this range.';
  if(points) points.innerHTML=chosen.points.length ? chosen.points.map((p,i)=>'<div class="env-point"><button onclick="deleteEnvelopePoint('+chosen.index+','+i+')">Delete</button>A1 '+p.a1.toFixed(2)+'° → A2 '+layerBoundSymbol(chosen.bound)+' '+p.a2Limit.toFixed(2)+'°</div>').join('') : 'No boundary points in this layer.';
}
function updateCurrentEnvelopeLimit(d) {
  const el=document.getElementById('currentEnvelopeLimit'); if(!el)return;
  const limits=Array.isArray(d.activeEnvelopeLimits)?d.activeEnvelopeLimits:[];
  if(!limits.length){el.textContent='Current pose: no active envelope restriction at this A1/Z.';return;}
  el.textContent=limits.map(x=>'Layer '+(x.layer+1)+': A2 '+layerBoundSymbol(x.bound)+' '+Number(x.limit).toFixed(2)+'°').join(' | ');
}
function renderStorage(c) {
  const el=document.getElementById('storageContent'); if(!el||!c)return;
  const layers=Array.isArray(c.envLayers)?c.envLayers:[];
  const layerText=layers.length ? layers.map(l=>'layer'+(l.index+1)+': '+layerDescription(l)+'\n  '+(l.points.length?l.points.map(p=>'A1raw='+p.a1Raw+',A2raw='+p.a2Raw).join(' | '):'no points')).join('\n') : 'layers: none';
  el.innerHTML='<div class="storage-card"><h3>scara_raw_v2</h3><pre>a1zero='+c.a1ZeroSteps+'\na1neg='+c.a1NegativeLimitRawSteps+'\na1pos='+c.a1PositiveLimitRawSteps+'\na2zero='+c.a2ZeroSteps+'\na2neg='+c.a2NegativeLimitRawSteps+'\na2pos='+c.a2PositiveLimitRawSteps+'\na2park='+c.a2HomeParkRawSteps+'\nzzero='+c.zZeroSteps+'\nztop='+c.zUpperLimitRawSteps+'\nzbottom='+c.zLowerLimitRawSteps+'</pre></div>'+
    '<div class="storage-card"><h3>scara_motion / scara_tool / scara_grip</h3><pre>manual='+c.manualSpeedPct+'% realtime='+c.realtimeSpeedPct+'% ik='+c.ikSpeedPct+'%\ntcpMm='+Number(c.toolTcpLengthMm).toFixed(2)+'\nopen='+c.gripperOpenDeg+'° hold='+c.gripperHoldDeg+'° maxclose='+c.gripperMaxCloseDeg+'°</pre></div>'+
    '<div class="storage-card"><h3>scara_env3</h3><pre>enabled='+c.collisionEnabled+'\nmargin='+Number(c.collisionMarginDeg||0).toFixed(2)+'°\ncorridorA2='+Number(c.collisionCorridorA2||0).toFixed(2)+'°\n'+layerText+'</pre></div>'+
    '<div class="storage-card"><h3>scara_pkg2</h3><pre>Recipe command timelines. Old scara_pkg1 data migrates automatically.</pre></div>';
}
function applySafetyConfiguration(c) {
  safety.startupConfirmed=!!c.startupPoseConfirmed; safety.parkSaved=!!c.startupParkSaved; safety.collisionEnabled=!!c.collisionEnabled; safety.margin=Number(c.collisionMarginDeg||0);
  const layers=Array.isArray(c.envLayers)?c.envLayers:[]; const pointCount=layers.reduce((sum,l)=>sum+l.points.length,0);
  const park=document.getElementById('parkReadout'); if(park) park.textContent=safety.parkSaved?'Park: A1 '+c.startupParkA1.toFixed(2)+'° | A2 '+c.startupParkA2.toFixed(2)+'° | Z '+c.startupParkZ.toFixed(2)+' mm':'No startup park saved.';
  const state=document.getElementById('collisionState'); if(state){state.className='collision-status '+(safety.collisionEnabled?'enabled':'disabled'); state.textContent=safety.collisionEnabled?'Protection ENABLED — '+layers.length+' layers / '+pointCount+' points.':'Protection DISABLED — '+layers.length+' layers / '+pointCount+' points saved.';}
  const margin=document.getElementById('envMargin'); if(margin) margin.value=safety.margin.toFixed(2);
  const corridor=document.getElementById('envCorridor'); if(corridor) corridor.value=Number(c.collisionCorridorA2||0).toFixed(2);
  renderEnvelopeLayers(c); renderStorage(c); updateStartupStatus(); setPointEditorLimits(false);
}

function updateStartupStatus() {
  const status=document.getElementById('startupStatus'); if(!status)return;
  if(safety.startupConfirmed) status.textContent='Safe startup confirmed. HOME is unlocked for the next attempt.';
  else if(homed) status.textContent='Robot is homed. Confirm again only before a new HOME attempt.';
  else status.textContent='Place the arm in the safe startup posture, then confirm to unlock HOME.';
}

async function confirmStartupPose() {
  try {
    const r = await fetch('/startup/confirm', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    safety.startupConfirmed = true;
    updateStartupStatus();
    document.getElementById('homeBtn').disabled = false;
    setMessage('ok', d.msg);
  } catch(e) { setMessage('err', 'Startup confirmation connection error.'); }
}

async function saveStartupPark() {
  try {
    const r = await fetch('/park/save', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    await loadConfiguration();
    setMessage('ok', d.msg);
  } catch(e) { setMessage('err', 'Could not save startup park pose.'); }
}

async function sendParkForPowerOff() {
  try {
    const r = await fetch('/park/go', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    setMessage('busy', d.msg);
  } catch(e) { setMessage('err', 'Could not command power-off park.'); }
}

async function createEnvelopeLayer() {
  const q=new URLSearchParams({region:layerDraftRegion,bound:layerDraftBound,zfrom:document.getElementById('layerZFromN').value,zto:document.getElementById('layerZToN').value});
  setCollisionLocalStatus('busy','Creating Z layer...');
  try {
    const r=await fetch('/collision/layer/create?'+q.toString(),{cache:'no-store'}),d=await r.json();
    if(!d.ok){setCollisionLocalStatus('err',d.err);return;}
    selectedEnvelopeLayer=d.layer; await loadConfiguration(); setCollisionLocalStatus('ok','Z layer created. Add boundary points to it.');
  } catch(e){setCollisionLocalStatus('err','Layer creation connection error.');}
}
async function deleteEnvelopeLayer(layer) {
  try {
    const r=await fetch('/collision/layer/delete?layer='+layer,{cache:'no-store'}),d=await r.json();
    if(!d.ok){setCollisionLocalStatus('err',d.err);return;}
    selectedEnvelopeLayer=-1; await loadConfiguration(); setCollisionLocalStatus('ok',d.msg);
  } catch(e){setCollisionLocalStatus('err','Layer delete connection error.');}
}
async function saveEnteredEnvelopePoint() {
  if(selectedEnvelopeLayer<0){setCollisionLocalStatus('err','Select or create a Z layer first.');return;}
  const q=new URLSearchParams({layer:selectedEnvelopeLayer,a1:document.getElementById('envA1N').value,a2:document.getElementById('envA2N').value});
  setCollisionLocalStatus('busy','Saving boundary point...');
  try { const r=await fetch('/collision/envelope/add?'+q.toString(),{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Save connection error.');}
}
async function captureActualEnvelopePoint() {
  if(selectedEnvelopeLayer<0){setCollisionLocalStatus('err','Select or create a Z layer first.');return;}
  setCollisionLocalStatus('busy','Capturing current robot pose...');
  try { const r=await fetch('/collision/envelope/capture?layer='+selectedEnvelopeLayer,{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Capture connection error.');}
}
async function deleteEnvelopePoint(layer,index) {
  try { const r=await fetch('/collision/envelope/delete?layer='+layer+'&index='+index,{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Delete connection error.');}
}
async function clearSelectedLayerPoints() {
  if(selectedEnvelopeLayer<0){setCollisionLocalStatus('err','Select a Z layer first.');return;}
  try { const r=await fetch('/collision/envelope/clear?layer='+selectedEnvelopeLayer,{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Clear connection error.');}
}
async function saveEnvelopeMargin() {
  try { const r=await fetch('/collision/envelope/margin?deg='+document.getElementById('envMargin').value,{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Margin connection error.');}
}
async function saveEnvelopeCorridor() {
  try { const r=await fetch('/collision/envelope/corridor?a2='+document.getElementById('envCorridor').value,{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Corridor connection error.');}
}
async function setCollisionProtection(enabled) {
  setCollisionLocalStatus('busy',enabled?'Enabling protection...':'Disabling protection...');
  try { const r=await fetch('/collision/protection?enabled='+(enabled?'1':'0'),{cache:'no-store'}),d=await r.json(); if(!d.ok){setCollisionLocalStatus('err',d.err);return;} await loadConfiguration(); setCollisionLocalStatus('ok',d.msg); } catch(e){setCollisionLocalStatus('err','Protection connection error.');}
}

function showRecipeStatus(type,text) {
  const el=document.getElementById('recipeLocalStatus'); if(!el)return;
  el.className='local-status'+(type?' '+type:''); el.textContent=text;
}
function currentRecipe() { return packageRecipes.find(r=>r.index===selectedRecipeIndex) || null; }
function commandLabel(c) {
  const map={MOVE_SAFE_XYZ:'Move Safe A1/A2/Z',MOVE_A1:'Move A1 only',MOVE_A2:'Move A2 only',MOVE_Z:'Move Z only',MOVE_A1_A2:'Move A1+A2 together',MOVE_XYZ_DIRECT:'Move A1+A2+Z direct',GRIPPER:'Set gripper',WAIT:'Wait'};
  return map[c]||c;
}
function stepDetail(s) {
  const w=(Number(s.waitMs||0)/1000).toFixed(1)+' s';
  const g=' | Gripper '+Number(s.gripper).toFixed(0)+'°';
  if(s.command==='MOVE_A1') return 'A1 → '+Number(s.a1).toFixed(2)+'°'+g+' | pause '+w;
  if(s.command==='MOVE_A2') return 'A2 → '+Number(s.a2).toFixed(2)+'°'+g+' | pause '+w;
  if(s.command==='MOVE_Z') return 'Z → '+Number(s.z).toFixed(2)+' mm'+g+' | pause '+w;
  if(s.command==='MOVE_A1_A2') return 'A1 → '+Number(s.a1).toFixed(2)+'°, A2 → '+Number(s.a2).toFixed(2)+'°'+g+' | pause '+w;
  if(s.command==='MOVE_SAFE_XYZ'||s.command==='MOVE_XYZ_DIRECT') return 'A1 '+Number(s.a1).toFixed(2)+'°, A2 '+Number(s.a2).toFixed(2)+'°, Z '+Number(s.z).toFixed(2)+' mm'+g+' | pause '+w;
  if(s.command==='GRIPPER') return 'Gripper → '+Number(s.gripper).toFixed(0)+'° | pause '+w;
  return 'Wait '+w+' | Gripper unchanged';
}
function renderRecipeSelectors() {
  if(selectedRecipeIndex>=0 && !packageRecipes.some(r=>r.index===selectedRecipeIndex)){selectedRecipeIndex=-1;selectedRecipeStep=-1;}
  if(selectedRecipeIndex<0 && packageRecipes.length)selectedRecipeIndex=packageRecipes[0].index;
  const html=packageRecipes.length?packageRecipes.map(r=>'<option value="'+r.index+'" '+(r.index===selectedRecipeIndex?'selected':'')+'>'+r.name+' ('+r.steps.length+' steps)</option>').join(''):'<option value="-1">No recipe</option>';
  document.getElementById('recipeSelect').innerHTML=html;document.getElementById('recipeRunSelect').innerHTML=html;
  const jobSelect=document.getElementById('jobRecipeSelect');
  if(jobSelect){
    const previous=jobSelect.value;
    jobSelect.innerHTML=html;
    if(previous!=='' && packageRecipes.some(r=>String(r.index)===previous)) jobSelect.value=previous;
  }
}
function renderRecipeSteps() {
  const list=document.getElementById('recipeStepList'),recipe=currentRecipe();
  if(!recipe){list.textContent='Create or select a recipe.';return;}
  list.innerHTML=recipe.steps.length?recipe.steps.map(s=>'<div class="step-card '+(s.index===selectedRecipeStep?'selected':'')+'"><strong>'+(s.index+1)+'. '+s.name+'</strong><br><span class="cmd">'+commandLabel(s.command)+'</span><br>'+stepDetail(s)+'<br><button onclick="selectRecipeStep('+s.index+')">Edit</button><button onclick="moveRecipeStep('+s.index+',\'up\')">Up</button><button onclick="moveRecipeStep('+s.index+',\'down\')">Down</button><button onclick="deleteRecipeStep('+s.index+')">Delete</button></div>').join(''):'No steps saved.';
}
function applyPackageRecipes(c){packageRecipes=Array.isArray(c.packageRecipes)?c.packageRecipes:[];renderRecipeSelectors();const r=currentRecipe();if(r){const n=document.getElementById('recipeNameInput');if(n)n.value=r.name;}renderRecipeSteps();}
function selectRecipe(){selectedRecipeIndex=Number(document.getElementById('recipeSelect').value);selectedRecipeStep=-1;const r=currentRecipe();if(r)document.getElementById('recipeNameInput').value=r.name;renderRecipeSelectors();renderRecipeSteps();}
function selectRunRecipe(){selectedRecipeIndex=Number(document.getElementById('recipeRunSelect').value);selectedRecipeStep=-1;renderRecipeSelectors();renderRecipeSteps();}
function copyLiveStepTargets(){document.getElementById('stepA1').value=recipeLiveValues.a1.toFixed(2);document.getElementById('stepA2').value=recipeLiveValues.a2.toFixed(2);document.getElementById('stepZ').value=recipeLiveValues.z.toFixed(2);document.getElementById('stepGrip').value=recipeLiveValues.gripper.toFixed(0);}
function updateStepEditor(){const c=document.getElementById('recipeCommandInput').value,m={MOVE_SAFE_XYZ:['stepA1','stepA2','stepZ'],MOVE_A1:['stepA1'],MOVE_A2:['stepA2'],MOVE_Z:['stepZ'],MOVE_A1_A2:['stepA1','stepA2'],MOVE_XYZ_DIRECT:['stepA1','stepA2','stepZ'],GRIPPER:['stepGrip'],WAIT:[]};['stepA1','stepA2','stepZ','stepGrip'].forEach(id=>document.getElementById(id).disabled=!m[c].includes(id));if(c==='GRIPPER'&&Number(document.getElementById('stepWaitSeconds').value)===0)document.getElementById('stepWaitSeconds').value='0.8';}
function clearStepEditor(){selectedRecipeStep=-1;document.getElementById('recipeStepNameInput').value='';document.getElementById('recipeCommandInput').value='MOVE_SAFE_XYZ';document.getElementById('stepWaitSeconds').value='0';copyLiveStepTargets();updateStepEditor();renderRecipeSteps();}
function selectRecipeStep(index){selectedRecipeStep=index;const r=currentRecipe(),s=r&&r.steps.find(x=>x.index===index);if(!s)return;document.getElementById('recipeStepNameInput').value=s.name;document.getElementById('recipeCommandInput').value=s.command;document.getElementById('stepA1').value=Number(s.a1).toFixed(2);document.getElementById('stepA2').value=Number(s.a2).toFixed(2);document.getElementById('stepZ').value=Number(s.z).toFixed(2);document.getElementById('stepGrip').value=s.gripper;document.getElementById('stepWaitSeconds').value=(Number(s.waitMs||0)/1000).toFixed(1);updateStepEditor();renderRecipeSteps();}
function stepQuery(){const q=new URLSearchParams();q.set('recipe',selectedRecipeIndex);q.set('name',document.getElementById('recipeStepNameInput').value);q.set('command',document.getElementById('recipeCommandInput').value);q.set('a1',document.getElementById('stepA1').value);q.set('a2',document.getElementById('stepA2').value);q.set('z',document.getElementById('stepZ').value);q.set('gripper',document.getElementById('stepGrip').value);q.set('waitms',Math.round(Number(document.getElementById('stepWaitSeconds').value||0)*1000));return q;}
function appendRecipeEvent(text){if(!text||text===lastRecipeEvent)return;lastRecipeEvent=text;recipeLogLines.unshift('['+new Date().toLocaleTimeString()+'] '+text);if(recipeLogLines.length>12)recipeLogLines.pop();document.getElementById('recipeLog').textContent=recipeLogLines.join('\n');}
function applyRecipeRuntime(d){recipeLiveValues={a1:Number(d.a1),a2:Number(d.a2),z:Number(d.z),gripper:Number(d.gripperCommandDeg)};[['recipeLiveA1',d.a1,2],['recipeLiveA2',d.a2,2],['recipeLiveZ',d.z,2],['recipeLiveGrip',d.gripperCommandDeg,0]].forEach(x=>{const e=document.getElementById(x[0]);if(e&&Number.isFinite(Number(x[1])))e.textContent=Number(x[1]).toFixed(x[2]);});document.getElementById('recipeMonitorName').textContent=d.recipeActive||'NONE';document.getElementById('recipeMonitorState').textContent=d.recipeState||'IDLE';document.getElementById('recipeMonitorStep').textContent=d.recipeRunning?('Step '+d.recipeStepIndex+' | Cycle '+d.recipeCycle+' / '+d.recipeRepeats):'--';document.getElementById('recipeMonitorMotion').textContent=d.recipeMotion||'--';document.getElementById('recipeMonitorStatus').textContent=d.recipeStatus||'Idle.';appendRecipeEvent(d.recipeLastEvent);}
function showJobStatus(type,text){const e=document.getElementById('jobLocalStatus');if(!e)return;e.className='local-status'+(type?' '+type:'');e.textContent=text;}
function appendJobEvent(text){if(!text||text===lastJobEvent)return;lastJobEvent=text;jobLogLines.unshift('['+new Date().toLocaleTimeString()+'] '+text);if(jobLogLines.length>12)jobLogLines.pop();const e=document.getElementById('jobLog');if(e)e.textContent=jobLogLines.join('\n');}
function applyJobRuntime(d){
  const s=document.getElementById('jobState');if(!s)return;
  s.textContent=d.jobState||'NOT_HOMED';
  document.getElementById('jobMonitorId').textContent=d.jobId||'--';
  document.getElementById('jobMonitorClass').textContent=d.jobPackageClass||'--';
  document.getElementById('jobMonitorRecipe').textContent=d.jobRecipe||'--';
  document.getElementById('jobMonitorAmr').textContent=d.jobAmrConfirmed?'Yes':'No';
  document.getElementById('jobMonitorStep').textContent=d.jobState==='RUNNING'?('Step '+d.recipeStepIndex):'--';
  document.getElementById('jobMonitorResult').textContent=d.jobResult||'--';
  document.getElementById('jobMonitorMessage').textContent=d.jobMessage||'--';
  appendJobEvent(d.jobLastEvent);
}
async function loadArmJob(){const rcp=document.getElementById('jobRecipeSelect').value,id=document.getElementById('jobIdInput').value,cls=document.getElementById('jobClassInput').value;try{const r=await fetch('/job/load?recipe='+encodeURIComponent(rcp)+'&jobId='+encodeURIComponent(id)+'&packageClass='+encodeURIComponent(cls),{cache:'no-store'}),d=await r.json();if(!d.ok){showJobStatus('err',d.err);return;}showJobStatus('ok',d.msg);}catch(e){showJobStatus('err','Could not load job.');}}
async function waitForAmr(){try{const r=await fetch('/job/wait-amr',{cache:'no-store'}),d=await r.json();if(!d.ok){showJobStatus('err',d.err);return;}showJobStatus('ok',d.msg);}catch(e){showJobStatus('err','Could not arm job.');}}
async function simulateAmrArrived(){try{const r=await fetch('/job/amr-arrived',{cache:'no-store'}),d=await r.json();if(!d.ok){showJobStatus('err',d.err);return;}showJobStatus('ok',d.msg);}catch(e){showJobStatus('err','Could not start job.');}}
async function abortArmJob(){try{const r=await fetch('/job/abort',{cache:'no-store'}),d=await r.json();if(!d.ok){showJobStatus('err',d.err);return;}showJobStatus('ok',d.msg);}catch(e){showJobStatus('err','Could not abort job.');}}
async function clearArmJob(){try{const r=await fetch('/job/clear',{cache:'no-store'}),d=await r.json();if(!d.ok){showJobStatus('err',d.err);return;}showJobStatus('ok',d.msg);}catch(e){showJobStatus('err','Could not clear job result.');}}

async function createRecipe(){try{const r=await fetch('/recipe/create?name='+encodeURIComponent(document.getElementById('recipeNameInput').value),{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeIndex=d.recipe;selectedRecipeStep=-1;await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not create recipe.');}}
async function renameRecipe(){if(selectedRecipeIndex<0){showRecipeStatus('err','Select a recipe first.');return;}try{const r=await fetch('/recipe/rename?recipe='+selectedRecipeIndex+'&name='+encodeURIComponent(document.getElementById('recipeNameInput').value),{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not rename recipe.');}}
async function deleteRecipe(){if(selectedRecipeIndex<0){showRecipeStatus('err','Select a recipe first.');return;}try{const r=await fetch('/recipe/delete?recipe='+selectedRecipeIndex,{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeIndex=-1;selectedRecipeStep=-1;await loadConfiguration();await checkRecipeStorage();await refreshSavedRecipeLibrary();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not delete recipe.');}}
async function addRecipeStep(){if(selectedRecipeIndex<0){showRecipeStatus('err','Select a recipe first.');return;}try{const r=await fetch('/recipe/step/add?'+stepQuery().toString(),{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeStep=d.step;await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not add step.');}}
async function updateSelectedRecipeStep(){if(selectedRecipeIndex<0||selectedRecipeStep<0){showRecipeStatus('err','Select a step first.');return;}const q=stepQuery();q.set('step',selectedRecipeStep);try{const r=await fetch('/recipe/step/update?'+q.toString(),{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not update step.');}}
async function deleteRecipeStep(index){try{const r=await fetch('/recipe/step/delete?recipe='+selectedRecipeIndex+'&step='+index,{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeStep=-1;await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not delete step.');}}
async function moveRecipeStep(index,direction){try{const r=await fetch('/recipe/step/reorder?recipe='+selectedRecipeIndex+'&step='+index+'&direction='+direction,{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeStep=-1;await loadConfiguration();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not reorder step.');}}
async function playSelectedRecipe(){if(selectedRecipeIndex<0){showRecipeStatus('err','Select a recipe first.');return;}try{const r=await fetch('/recipe/play?recipe='+selectedRecipeIndex+'&repeat='+encodeURIComponent(document.getElementById('recipeRepeatInput').value),{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}appendRecipeEvent(d.msg);showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not run recipe.');}}
async function resetRecipePlayback(){try{const r=await fetch('/recipe/reset',{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}appendRecipeEvent(d.msg);showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not reset status.');}}
async function downloadRecipesJson(){try{const r=await fetch('/recipe/export',{cache:'no-store'}),text=await r.text(),blob=new Blob([text],{type:'application/json'}),url=URL.createObjectURL(blob),a=document.createElement('a');a.href=url;a.download='SCARA_command_recipes_'+new Date().toISOString().replace(/[:.]/g,'-')+'.json';a.click();URL.revokeObjectURL(url);showRecipeStatus('ok','JSON downloaded.');}catch(e){showRecipeStatus('err','Could not download JSON.');}}
async function saveRecipesToEsp32(){try{const r=await fetch('/recipe/save-now',{cache:'no-store'}),d=await r.json();const el=document.getElementById('recipeStorageStatus');if(el)el.textContent=(d.ok?'Saved: ':'Save failed: ')+(d.msg||'')+' | count '+(d.count??'?')+' | checksum '+(d.checksum??'?');showRecipeStatus(d.ok?'ok':'err',d.msg||'Save command finished.');await loadConfiguration();await refreshSavedRecipeLibrary();}catch(e){showRecipeStatus('err','Could not save recipes to ESP32.');}}
async function commitRecipesToEsp32(){try{const r=await fetch('/recipe/commit-verify',{cache:'no-store'}),d=await r.json();const el=document.getElementById('recipeStorageStatus');if(el)el.textContent=(d.ok?'COMMITTED: ':'COMMIT FAILED: ')+(d.msg||'')+' | count '+(d.count??'?')+' | logical '+(d.logicalAfter??'?');showRecipeStatus(d.ok?'ok':'err',d.msg||'Commit finished.');await loadConfiguration();await checkRecipeStorage();await refreshSavedRecipeLibrary();}catch(e){showRecipeStatus('err','Could not commit recipes to ESP32.');}}
async function reloadRecipesFromEsp32(){try{const r=await fetch('/recipe/reload-storage',{cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeIndex=-1;selectedRecipeStep=-1;await loadConfiguration();await checkRecipeStorage();await refreshSavedRecipeLibrary();showRecipeStatus('ok',d.msg||'Recipes reloaded from ESP32.');}catch(e){showRecipeStatus('err','Could not reload recipes from ESP32.');}}
async function checkRecipeStorage(){try{const r=await fetch('/recipe/storage-status',{cache:'no-store'}),d=await r.json();const names=(d.recipes||[]).map(x=>x.index+': '+x.name+' ('+x.steps+' steps)').join(' | ');const el=document.getElementById('recipeStorageStatus');if(el)el.textContent='Storage '+d.storage+' | format '+(d.format||'?')+' | version '+(d.versionOk?'OK':'BAD')+' | stored '+d.storedCount+' | active '+d.count+' | logical '+(d.logicalChecksum??'?')+' | '+(names||'No recipes saved');showRecipeStatus('ok','ESP32 recipe storage checked.');}catch(e){showRecipeStatus('err','Could not check ESP32 recipe storage.');}}
async function restoreRecipesJson(){const f=document.getElementById('recipesJsonFile');if(!f.files||!f.files[0]){showRecipeStatus('err','Select a JSON file first.');return;}try{const b=JSON.parse(await f.files[0].text());let rs=[];if(b.format==='SCARA_COMMAND_RECIPES_BACKUP'&&Array.isArray(b.recipes)){rs=b.recipes;}else if(b.format==='SCARA_PACKAGE_RECIPES_BACKUP'&&Array.isArray(b.recipes)){rs=b.recipes.map(r=>{const steps=[];(r.poses||[]).forEach(p=>{steps.push({name:(p.name||'POSE')+'_MOVE',command:'MOVE_SAFE_XYZ',a1Deg:p.a1Deg,a2Deg:p.a2Deg,zMm:p.zMm,gripperDeg:p.gripperDeg,waitMs:0});steps.push({name:(p.name||'POSE')+'_GRIP',command:'GRIPPER',a1Deg:p.a1Deg,a2Deg:p.a2Deg,zMm:p.zMm,gripperDeg:p.gripperDeg,waitMs:800});});return{name:r.name,steps};});}else{showRecipeStatus('err','Invalid recipe JSON.');return;}const q=new URLSearchParams();q.set('recipeCount',rs.length);rs.forEach((r,ri)=>{q.set('r'+ri+'n',r.name);q.set('r'+ri+'c',(r.steps||[]).length);(r.steps||[]).forEach((s,si)=>{const k='r'+ri+'s'+si;q.set(k+'n',s.name);q.set(k+'cmd',s.command);q.set(k+'a1',s.a1Deg||0);q.set(k+'a2',s.a2Deg||0);q.set(k+'z',s.zMm||0);q.set(k+'g',s.gripperDeg||0);q.set(k+'w',s.waitMs||0);});});const r=await fetch('/recipe/import',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:q.toString(),cache:'no-store'}),d=await r.json();if(!d.ok){showRecipeStatus('err',d.err);return;}selectedRecipeIndex=-1;selectedRecipeStep=-1;await loadConfiguration();await checkRecipeStorage();await refreshSavedRecipeLibrary();showRecipeStatus('ok',d.msg);}catch(e){showRecipeStatus('err','Could not restore JSON.');}}


function renderSavedRecipeLibrary() {
  const box = document.getElementById('recipeLibraryList');
  if (!box) return;
  if (!savedRecipeLibrary.length) {
    box.innerHTML = 'No saved recipes in ESP32.';
    return;
  }
  box.innerHTML = savedRecipeLibrary.map(r => {
    const steps = Array.isArray(r.steps) ? r.steps.length : (r.steps || 0);
    const selected = r.index === selectedLibraryRecipe ? ' selected' : '';
    return '<div class="library-card'+selected+'" onclick="selectSavedLibraryRecipe('+r.index+')">' +
      '<strong>'+r.index+': '+r.name+'</strong><br>' +
      steps + ' steps saved in ESP32' +
      '<br><button onclick="event.stopPropagation();selectSavedLibraryRecipe('+r.index+');renameSavedLibraryRecipe()">Rename</button>' +
      '<button onclick="event.stopPropagation();selectSavedLibraryRecipe('+r.index+');deleteSavedLibraryRecipe()">Delete</button>' +
      '</div>';
  }).join('');
}

function selectSavedLibraryRecipe(index) {
  selectedLibraryRecipe = index;
  selectedRecipeIndex = index;
  const recipe = savedRecipeLibrary.find(r => r.index === index);
  if (recipe) {
    const n1 = document.getElementById('recipeLibraryRenameInput');
    const n2 = document.getElementById('recipeNameInput');
    if (n1) n1.value = recipe.name;
    if (n2) n2.value = recipe.name;
  }
  renderSavedRecipeLibrary();
  renderRecipeSelectors();
  renderRecipeSteps();
}

async function refreshSavedRecipeLibrary() {
  try {
    const r = await fetch('/recipe/library', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { showRecipeStatus('err', d.err || 'Could not read recipe library.'); return; }
    savedRecipeLibrary = Array.isArray(d.recipes) ? d.recipes : [];
    const st = document.getElementById('recipeLibraryStatus');
    if (st) st.textContent = 'Saved library: '+d.count+' recipe(s) | stored '+d.storedCount+' | stored bytes '+(d.blobBytes??'?')+' | checksum '+(d.logicalChecksum??'?');
    renderSavedRecipeLibrary();
  } catch(e) {
    showRecipeStatus('err','Could not refresh saved recipe library.');
  }
}

async function renameSavedLibraryRecipe() {
  if (selectedLibraryRecipe < 0) { showRecipeStatus('err','Select a saved recipe first.'); return; }
  const newName = document.getElementById('recipeLibraryRenameInput').value;
  if (!newName.trim()) { showRecipeStatus('err','Enter a new recipe name.'); return; }
  try {
    const r = await fetch('/recipe/rename?recipe='+selectedLibraryRecipe+'&name='+encodeURIComponent(newName), {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { showRecipeStatus('err', d.err); return; }
    await commitRecipesToEsp32();
    await loadConfiguration();
    await refreshSavedRecipeLibrary();
    showRecipeStatus('ok','Saved recipe renamed and verified.');
  } catch(e) { showRecipeStatus('err','Could not rename saved recipe.'); }
}

async function deleteSavedLibraryRecipe() {
  if (selectedLibraryRecipe < 0) { showRecipeStatus('err','Select a saved recipe first.'); return; }
  if (!confirm('Delete this saved recipe from ESP32?')) return;
  try {
    const r = await fetch('/recipe/delete?recipe='+selectedLibraryRecipe, {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { showRecipeStatus('err', d.err); return; }
    selectedRecipeIndex = -1;
    selectedRecipeStep = -1;
    selectedLibraryRecipe = -1;
    await commitRecipesToEsp32();
    await loadConfiguration();
    await refreshSavedRecipeLibrary();
    showRecipeStatus('ok','Saved recipe deleted and verified.');
  } catch(e) { showRecipeStatus('err','Could not delete saved recipe.'); }
}

async function clearSavedRecipeLibrary() {
  if (!confirm('Clear ALL saved recipes from ESP32? This only clears recipes, not calibration or collision settings.')) return;
  try {
    const r = await fetch('/recipe/clear-all', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { showRecipeStatus('err', d.err); return; }
    selectedRecipeIndex = -1;
    selectedRecipeStep = -1;
    selectedLibraryRecipe = -1;
    savedRecipeLibrary = [];
    await loadConfiguration();
    await refreshSavedRecipeLibrary();
    showRecipeStatus('ok', d.msg);
  } catch(e) { showRecipeStatus('err','Could not clear saved recipes.'); }
}

async function clearLegacyRecipesOnly() {
  try {
    const r = await fetch('/recipe/clear-legacy', {cache:'no-store'});
    const d = await r.json();
    showRecipeStatus(d.ok?'ok':'err', d.msg || d.err || 'Legacy storage command finished.');
  } catch(e) { showRecipeStatus('err','Could not clear old ghost storage.'); }
}

function setMode(newMode) {
  mode = newMode;
  const activeMap = {Manual:'manual', RT:'realtime', IK:'ik', Cal:'calibration', Collision:'collision', Recipes:'recipes', RecipeRun:'recipeRun', Jobs:'jobs', Storage:'storage'};
  Object.keys(activeMap).forEach(key => {
    const el = document.getElementById('tab' + key);
    if (el) el.className = 'mode-tab' + (mode === activeMap[key] ? ' active' : '');
  });
  document.getElementById('ikPanel').style.display = mode==='ik' ? 'block' : 'none';
  document.getElementById('calPanel').style.display = mode==='calibration' ? 'block' : 'none';
  document.getElementById('collisionPanel').style.display = mode==='collision' ? 'block' : 'none';
  document.getElementById('recipesPanel').style.display = mode==='recipes' ? 'block' : 'none';
  document.getElementById('recipeRunPanel').style.display = mode==='recipeRun' ? 'block' : 'none';
  document.getElementById('jobsPanel').style.display = mode==='jobs' ? 'block' : 'none';
  document.getElementById('storagePanel').style.display = mode==='storage' ? 'block' : 'none';
  const hidden = ['ik','calibration','collision','recipes','recipeRun','jobs','storage'].includes(mode);
  document.querySelectorAll('.joint-row').forEach(e => e.style.display = hidden ? 'none' : '');
  document.querySelector('.target-fk').style.display = hidden ? 'none' : '';
  const send = document.getElementById('sendBtn');
  send.style.display = ['realtime','calibration','collision','recipes','recipeRun','jobs','storage'].includes(mode) ? 'none' : '';
  send.textContent = mode==='ik' ? 'Send IK Target' : 'Send Target';
  send.onclick = mode==='ik' ? sendIK : sendManual;
  if (mode==='ik') previewIK();
  if (['calibration','collision','recipes','recipeRun','jobs','storage'].includes(mode)) loadConfiguration();
  if (mode==='recipes') refreshSavedRecipeLibrary();
  updateSpeedPanel();
}
function targetValues() {
  return {
    a1: +document.getElementById('a1').value,
    a2: +document.getElementById('a2').value,
    z: +document.getElementById('z').value
  };
}

function syncTargetDisplay() {
  const t = targetValues();
  document.getElementById('a1Num').value = t.a1;
  document.getElementById('a2Num').value = t.a2;
  document.getElementById('zNum').value = t.z;
  const p = fk(t.a1, t.a2, t.z);
  document.getElementById('targetX').textContent = p.x.toFixed(2);
  document.getElementById('targetY').textContent = p.y.toFixed(2);
  document.getElementById('targetZ').textContent = p.z.toFixed(2);
  drawArm(t.a1, t.a2);
}

function numberChanged() {
  const a1 = clamp(+document.getElementById('a1Num').value, A1_MIN, A1_MAX);
  const a2 = clamp(+document.getElementById('a2Num').value, A2_MIN, A2_MAX);
  const z = clamp(+document.getElementById('zNum').value, Z_MIN, Z_MAX);
  document.getElementById('a1').value = a1;
  document.getElementById('a2').value = a2;
  document.getElementById('z').value = z;
  syncTargetDisplay();
  if (mode === 'realtime') scheduleRT();
}

function sliderChanged() {
  syncTargetDisplay();
  if (mode === 'realtime') scheduleRT();
}

function fk(a1, a2, z) {
  const r1 = a1 * Math.PI / 180;
  const r2 = a2 * Math.PI / 180;
  return {
    x: L1 * Math.cos(r1) + L2 * Math.cos(r1 + r2),
    y: L1 * Math.sin(r1) + L2 * Math.sin(r1 + r2),
    z: z
  };
}

function chooseSafeA1(base, reference) {
  while (base > 180) base -= 360;
  while (base < -180) base += 360;

  const candidates = [base - 360, base, base + 360]
    .filter(v => v >= A1_MIN - IK_JOINT_TOL && v <= A1_MAX + IK_JOINT_TOL);

  if (!candidates.length) return null;

  candidates.sort((a, b) => {
    const da = Math.abs(a - reference);
    const db = Math.abs(b - reference);
    if (Math.abs(da - db) < 0.001) return b - a;
    return da - db;
  });

  return clamp(candidates[0], A1_MIN, A1_MAX);
}

function clearIkRoundTripReference() {
  ikRoundTripReference = null;
}

function setIkConfigurationFromA2(a2) {
  ikConfiguration = (a2 < -0.001) ? 'elbow_up' : 'elbow_down';
  document.getElementById('nearestBtn').className = '';
}

function selectConfiguration(configuration) {
  clearIkRoundTripReference();
  ikConfiguration = configuration;
  document.getElementById('nearestBtn').className = configuration==='nearest' ? 'active' : '';
  previewIK();
}

function loadActualPoseIntoIK() {
  document.getElementById('ikX').value = Number(lastActual.x).toFixed(4);
  document.getElementById('ikY').value = Number(lastActual.y).toFixed(4);
  document.getElementById('ikZ').value = Number(lastActual.z).toFixed(3);
  ikRoundTripReference = {a1:lastActual.a1, a2:lastActual.a2, source:'Actual pose'};
  setIkConfigurationFromA2(lastActual.a2);
  previewIK();
}

function loadFkTargetIntoIK() {
  const t = targetValues();
  const p = fk(t.a1, t.a2, t.z);
  document.getElementById('ikX').value = p.x.toFixed(4);
  document.getElementById('ikY').value = p.y.toFixed(4);
  document.getElementById('ikZ').value = p.z.toFixed(3);
  ikRoundTripReference = {a1:t.a1, a2:t.a2, source:'FK slider target'};
  setIkConfigurationFromA2(t.a2);
  previewIK();
}

function solvePreviewConfiguration(x,y,configuration) {
  const d2=x*x+y*y;
  const c2raw=(d2-L1*L1-L2*L2)/(2*L1*L2);
  if(c2raw < -1.0001 || c2raw > 1.0001) return {ok:false, reason:'Outside XY reach'};
  const c2=clamp(c2raw,-1,1);
  let s2=Math.sqrt(Math.max(0,1-c2*c2));
  // Standard naming for this X-right / Y-up coordinate system:
  // positive theta2 bends the elbow below the base-to-tool line: Elbow-Down.
  if(configuration==='elbow_up') s2=-s2;
  let a2=Math.atan2(s2,c2)*180/Math.PI;
  const rawA1=(Math.atan2(y,x)-Math.atan2(L2*s2,L1+L2*c2))*180/Math.PI;
  const a1=chooseSafeA1(rawA1,lastActual.a1);
  if(a1===null) return {ok:false, reason:'A1 protected limit'};
  if(a2 < A2_MIN-IK_JOINT_TOL || a2 > A2_MAX+IK_JOINT_TOL) return {ok:false, reason:'A2 protected limit'};
  a2=clamp(a2,A2_MIN,A2_MAX);
  const check=fk(a1,a2,0);
  const error=Math.hypot(check.x-x,check.y-y);
  if(error>IK_MAX_POSITION_ERROR) return {ok:false, reason:'Safe-limit clamp error'};
  return {ok:true,a1,a2,error,configuration};
}

function fillSolutionCard(cardId,valueId,solution,selected) {
  const card=document.getElementById(cardId);
  const value=document.getElementById(valueId);
  if(!solution.ok) {
    card.className='solution-card invalid';
    card.disabled=true;
    value.textContent='Unavailable: '+solution.reason;
    return;
  }
  card.disabled=false;
  card.className='solution-card'+(selected?' active':'');
  value.textContent='A1 '+solution.a1.toFixed(2)+'°  |  A2 '+solution.a2.toFixed(2)+'°';
}

function previewIK() {
  const x=+document.getElementById('ikX').value;
  const y=+document.getElementById('ikY').value;
  const res=document.getElementById('ikResult');

  const elbowDown=solvePreviewConfiguration(x,y,'elbow_down');
  const elbowUp=solvePreviewConfiguration(x,y,'elbow_up');

  let selected=null;
  let selectedName='';

  if(ikConfiguration==='elbow_down') {
    selected=elbowDown;
    selectedName='Elbow-Down Configuration';
  } else if(ikConfiguration==='elbow_up') {
    selected=elbowUp;
    selectedName='Elbow-Up Configuration';
  } else {
    const options=[elbowDown,elbowUp].filter(s=>s.ok);
    if(options.length) {
      options.sort((a,b)=>
        (Math.abs(a.a1-lastActual.a1)+Math.abs(a.a2-lastActual.a2)) -
        (Math.abs(b.a1-lastActual.a1)+Math.abs(b.a2-lastActual.a2))
      );
      selected=options[0];
      selectedName=selected.configuration==='elbow_up'?'Elbow-Up Configuration':'Elbow-Down Configuration';
    } else {
      selected={ok:false,reason:'No safe IK configuration exists for this target.'};
    }
  }

  fillSolutionCard('elbowDownCard','elbowDownValues',elbowDown,ikConfiguration==='elbow_down');
  fillSolutionCard('elbowUpCard','elbowUpValues',elbowUp,ikConfiguration==='elbow_up');

  if(!selected || !selected.ok) {
    res.className='ik-result err';
    res.textContent=selected ? selected.reason : 'No safe IK solution.';
    return;
  }

  if (ikRoundTripReference) {
    const dA1 = Math.abs(selected.a1 - ikRoundTripReference.a1);
    const dA2 = Math.abs(selected.a2 - ikRoundTripReference.a2);
    if (dA1 > 0.15 || dA2 > 0.15) {
      res.className='ik-result err';
      res.textContent='Round-trip mismatch from '+ikRoundTripReference.source+': source A1 '+ikRoundTripReference.a1.toFixed(2)+'°, A2 '+ikRoundTripReference.a2.toFixed(2)+'°; IK returned A1 '+selected.a1.toFixed(2)+'°, A2 '+selected.a2.toFixed(2)+'°. The wrong posture configuration is selected.';
      drawArm(selected.a1,selected.a2);
      return;
    }
    res.className='ik-result ok';
    res.textContent='Round-trip PASS — '+selectedName+': A1 '+selected.a1.toFixed(2)+'°, A2 '+selected.a2.toFixed(2)+'° matches '+ikRoundTripReference.source+'.';
  } else {
    res.className='ik-result ok';
    res.textContent='Selected: '+selectedName+' — A1 '+selected.a1.toFixed(2)+'°, A2 '+selected.a2.toFixed(2)+'°';
  }
  drawArm(selected.a1,selected.a2);
}

async function loadConfiguration() {
  try {
    const r = await fetch('/config',{cache:'no-store'});
    const c = await r.json();
    if (!c.ok) return;
    A1_MIN = c.a1Min; A1_MAX = c.a1Max;
    A2_MIN = c.a2Min; A2_MAX = c.a2Max;
    Z_MIN = c.zMin; Z_MAX = c.zMax;
    if (Number.isFinite(c.manualSpeedPct)) {
      speeds.manual = c.manualSpeedPct;
      speeds.realtime = c.realtimeSpeedPct;
      speeds.ik = c.ikSpeedPct;
      updateSpeedPanel();
    }
    if (Number.isFinite(c.gripperOpenDeg)) {
      applyGripperState(c, true);
    }
    applySafetyConfiguration(c);
    applyPackageRecipes(c);
    if (Number.isFinite(c.toolTcpLengthMm)) {
      L2 = c.toolTcpLengthMm;
      document.getElementById('tcpLengthInput').value = L2.toFixed(2);
      document.getElementById('toolReadout').textContent =
        'Saved TCP L2: ' + L2.toFixed(2) + ' mm | A2 A1-HOME clearance pose: ' + c.a2HomeParkDeg.toFixed(2) + '°';
      syncTargetDisplay();
      if (mode === 'ik') previewIK();
    }
    document.getElementById('a1').min = A1_MIN;
    document.getElementById('a1').max = A1_MAX;
    document.getElementById('a1Num').min = A1_MIN;
    document.getElementById('a1Num').max = A1_MAX;
    document.getElementById('a2').min = A2_MIN;
    document.getElementById('a2').max = A2_MAX;
    document.getElementById('a2Num').min = A2_MIN;
    document.getElementById('a2Num').max = A2_MAX;
    document.getElementById('z').min = Z_MIN;
    document.getElementById('z').max = Z_MAX;
    document.getElementById('zNum').min = Z_MIN;
    document.getElementById('zNum').max = Z_MAX;
    document.getElementById('a1RangeNote').textContent = 'Calibrated safe range: ' + A1_MIN.toFixed(2) + '° to ' + A1_MAX.toFixed(2) + '°';
    document.getElementById('a2RangeNote').textContent = 'Calibrated safe range: ' + A2_MIN.toFixed(2) + '° to ' + A2_MAX.toFixed(2) + '°';
    document.getElementById('zRangeNote').textContent = 'Calibrated safe range: ' + Z_MIN.toFixed(2) + ' mm to ' + Z_MAX.toFixed(2) + ' mm';
    document.getElementById('calReadout').textContent =
      'Derived angle limits from raw endpoints: A1 ' + A1_MIN.toFixed(2) + '° to ' + A1_MAX.toFixed(2) +
      '° | A2 ' + A2_MIN.toFixed(2) + '° to ' + A2_MAX.toFixed(2) + '°';
    document.getElementById('zCalReadout').textContent =
      'Z limits: ' + Z_MIN.toFixed(2) + ' mm to ' + Z_MAX.toFixed(2) + ' mm';
    storedConfig = c;
    setLayerZLimits(false);
    renderEnvelopeLayers(c);
    setPointEditorLimits(false);
  } catch(e) {
    setMessage('err','Cannot load calibration.');
  }
}

async function calJog(axis, deg) {
  if (!homed && !setupRequired) { setMessage('err','Press HOME first.'); return; }
  if (setupRequired && !references[axis]) { setMessage('err',axis.toUpperCase()+' is not referenced yet.'); return; }
  try {
    const r = await fetch('/cal/jog?axis='+axis+'&deg='+deg);
    const d = await r.json();
    if (!d.ok) setMessage('err',d.err);
    else setMessage('busy','Setup jog: '+axis.toUpperCase()+' '+deg+'°');
  } catch(e) { setMessage('err','Setup jog connection error.'); }
}

async function setZero(axis) {
  try {
    const r = await fetch('/cal/set_zero?axis='+axis);
    const d = await r.json();
    if (!d.ok) { setMessage('err',d.err); return; }
    setMessage('ok',axis.toUpperCase()+' geometric zero saved.');
    await loadConfiguration();
  } catch(e) { setMessage('err','Cannot save joint zero.'); }
}

async function setLimit(axis, side) {
  try {
    const r = await fetch('/cal/set_limit?axis='+axis+'&side='+side);
    const d = await r.json();
    if (!d.ok) { setMessage('err',d.err); return; }
    await loadConfiguration();
    setMessage('ok',axis.toUpperCase()+' '+side+' safe limit saved.');
  } catch(e) { setMessage('err','Cannot save safe limit.'); }
}

async function saveTcpLength() {
  const mm = +document.getElementById('tcpLengthInput').value;
  if (!Number.isFinite(mm) || mm < 20 || mm > 250) {
    setMessage('err','TCP length must be between 20 and 250 mm.');
    return;
  }
  try {
    const r = await fetch('/tool/set_tcp?mm=' + mm, {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    L2 = mm;
    await loadConfiguration();
    setMessage('ok','TCP length saved. FK and IK now use ' + mm.toFixed(2) + ' mm.');
  } catch(e) { setMessage('err','Could not save TCP length.'); }
}

async function saveA2HomePark() {
  if (!homed && !setupRequired) { setMessage('err','HOME/reference the robot first.'); return; }
  if (!references.a2) { setMessage('err','A2 is not referenced yet.'); return; }
  try {
    const r = await fetch('/tool/set_a2_home_park', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) { setMessage('err', d.err); return; }
    await loadConfiguration();
    setMessage('ok','A2 HOME-clearance pose saved: ' + d.deg.toFixed(2) + '°.');
  } catch(e) { setMessage('err','Could not save A2 HOME-clearance pose.'); }
}

async function downloadCollisionLayerBackup() {
  try {
    const response = await fetch('/collision/export', {cache:'no-store'});
    const text = await response.text();
    const blob = new Blob([text], {type:'application/json'});
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    const stamp = new Date().toISOString().replace(/[:.]/g,'-');
    anchor.href = url;
    anchor.download = 'SCARA_collision_layers_' + stamp + '.json';
    anchor.click();
    URL.revokeObjectURL(url);
    setCollisionLocalStatus('ok', 'Collision layers JSON downloaded.');
    setMessage('ok', 'Collision layers JSON downloaded.');
  } catch(e) {
    setCollisionLocalStatus('err', 'Could not download collision layers JSON.');
    setMessage('err', 'Could not download collision layers JSON.');
  }
}

async function restoreCollisionLayerBackup() {
  const input = document.getElementById('collisionLayerBackupFile');
  if (!input.files || !input.files[0]) {
    setCollisionLocalStatus('err', 'Select a collision layers JSON file first.');
    return;
  }

  try {
    const backup = JSON.parse(await input.files[0].text());
    if (backup.format !== 'SCARA_COLLISION_LAYER_BACKUP' || !Array.isArray(backup.layers)) {
      setCollisionLocalStatus('err', 'Invalid collision layers JSON file.');
      return;
    }

    const params = new URLSearchParams();
    params.set('layerCount', backup.layers.length);
    params.set('marginDeg', backup.marginDeg ?? 0);
    params.set('corridorA2Deg', backup.corridorA2Deg ?? 0);

    backup.layers.forEach((layer, layerIndex) => {
      const base = 'l' + layerIndex;
      params.set(base + 'region', layer.region);
      params.set(base + 'bound', layer.bound);
      params.set(base + 'zFromMm', layer.zFromMm);
      params.set(base + 'zToMm', layer.zToMm);
      const points = Array.isArray(layer.points) ? layer.points : [];
      params.set(base + 'pointCount', points.length);
      points.forEach((point, pointIndex) => {
        const prefix = base + 'p' + pointIndex;
        params.set(prefix + 'a1Deg', point.a1Deg);
        params.set(prefix + 'a2LimitDeg', point.a2LimitDeg);
      });
    });

    const response = await fetch('/collision/import', {
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:params.toString(),
      cache:'no-store'
    });
    const data = await response.json();
    if (!data.ok) {
      setCollisionLocalStatus('err', data.err);
      return;
    }

    selectedEnvelopeLayer = -1;
    await loadConfiguration();
    setCollisionLocalStatus('ok', data.msg);
  } catch(e) {
    setCollisionLocalStatus('err', 'Could not read or restore collision layers JSON.');
  }
}

async function downloadCalibrationBackup() {
  try {
    const r = await fetch('/cal/export', {cache:'no-store'});
    const text = await r.text();
    const blob = new Blob([text], {type:'application/json'});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    const stamp = new Date().toISOString().replace(/[:.]/g,'-');
    a.href = url;
    a.download = 'SCARA_calibration_backup_' + stamp + '.json';
    a.click();
    URL.revokeObjectURL(url);
    setMessage('ok','Calibration backup downloaded.');
  } catch(e) {
    setMessage('err','Could not download calibration backup.');
  }
}

async function restoreCalibrationBackup() {
  const input = document.getElementById('calBackupFile');
  if (!input.files || !input.files[0]) {
    setMessage('err','Select a calibration backup JSON file first.');
    return;
  }

  try {
    const backup = JSON.parse(await input.files[0].text());
    if (backup.format !== 'SCARA_RAW_CAL_BACKUP') {
      setMessage('err','Invalid SCARA calibration backup file.');
      return;
    }

    const params = new URLSearchParams();
    ['a1zero','a1neg','a1pos','a2zero','a2neg','a2pos','a2park','zzero','ztop','zbottom','tcpMm','manualPct','realtimePct','ikPct']
      .forEach(k => params.set(k, backup[k]));
    ['gripOpen','gripHold','gripMax','envEnabled','envMargin','envCorridor','parkValid','parkA1','parkA2','parkZ','layerCount'].forEach(k => { if (backup[k] !== undefined) params.set(k, backup[k]); });
    if (Number.isFinite(backup.layerCount)) {
      for (let i=0; i<backup.layerCount; i++) {
        ['region','bound','zmin','zmax','count'].forEach(s => { const k='l'+i+s; if (backup[k] !== undefined) params.set(k,backup[k]); });
        const count=Number.isFinite(backup['l'+i+'count']) ? backup['l'+i+'count'] : 0;
        for (let j=0; j<count; j++) ['a','b'].forEach(s => { const k='l'+i+'p'+j+s; if (backup[k] !== undefined) params.set(k,backup[k]); });
      }
    } else {
      ['px','pn','nx','nn'].forEach(prefix => {
        const countKey=prefix+'Count'; if (backup[countKey] !== undefined) params.set(countKey,backup[countKey]);
        const count=Number.isFinite(backup[countKey]) ? backup[countKey] : 0;
        for (let i=0;i<count;i++) ['a','b'].forEach(s => { const k=prefix+i+s; if(backup[k]!==undefined) params.set(k,backup[k]); });
      });
    }

    const r = await fetch('/cal/import', {
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:params.toString(),
      cache:'no-store'
    });
    const d = await r.json();
    if (!d.ok) {
      setMessage('err', d.err);
      return;
    }

    homed = false;
    setupRequired = false;
    await loadConfiguration();
    setMessage('ok', d.msg);
  } catch(e) {
    setMessage('err','Could not read or restore calibration backup.');
  }
}

async function resetCalibration() {
  try {
    const r = await fetch('/cal/reset');
    const d = await r.json();
    homed = false;
    await loadConfiguration();
    setMessage('err',d.msg);
  } catch(e) { setMessage('err','Reset failed.'); }
}

function drawArm(a1,a2) {
  const c=document.getElementById('cv'), ctx=c.getContext('2d');
  c.width=c.clientWidth; c.height=340;
  const W=c.width,H=c.height,cx=W/2,cy=H/2;
  const scale=(Math.min(W,H)*0.40)/(L1+L2);
  ctx.clearRect(0,0,W,H);
  [L1+L2,Math.abs(L1-L2)].forEach(r=>{
    ctx.beginPath();ctx.arc(cx,cy,r*scale,0,Math.PI*2);
    ctx.setLineDash([4,4]);ctx.strokeStyle='#2c3038';ctx.stroke();ctx.setLineDash([]);
  });
  const r1=a1*Math.PI/180,r2=a2*Math.PI/180;
  const ex=cx+L1*Math.cos(r1)*scale, ey=cy-L1*Math.sin(r1)*scale;
  const px=ex+L2*Math.cos(r1+r2)*scale, py=ey-L2*Math.sin(r1+r2)*scale;
  ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(ex,ey);
  ctx.strokeStyle='#aab1bd';ctx.lineWidth=5;ctx.lineCap='round';ctx.stroke();
  ctx.beginPath();ctx.moveTo(ex,ey);ctx.lineTo(px,py);
  ctx.strokeStyle='#e7ebf0';ctx.lineWidth=4;ctx.stroke();
  [[cx,cy,6],[ex,ey,5],[px,py,6]].forEach(p=>{ctx.beginPath();ctx.arc(p[0],p[1],p[2],0,Math.PI*2);ctx.fillStyle='#fff';ctx.fill();});
}

function setMessage(type,text){
  const m=document.getElementById('msg');
  m.className='status '+type;
  m.textContent=text;
}

async function sendManual() {
  if (!homed){setMessage('err','Press HOME first.');return;}
  const t=targetValues();
  try{
    const r=await fetch('/move?a1='+t.a1+'&a2='+t.a2+'&z='+t.z);
    const d=await r.json();
    if(d.ok) setMessage('ok', d.msg || 'Target accepted.');
    else setMessage('err',d.err);
  }catch(e){setMessage('err','Connection error.');}
}

function scheduleRT() {
  clearTimeout(rtTimer);
  rtTimer=setTimeout(async()=>{
    if(!homed) return;
    const t=targetValues();
    try {
      const r=await fetch('/move_rt?a1='+t.a1+'&a2='+t.a2+'&z='+t.z);
      const d=await r.json();
      if(!d.ok) setMessage('err',d.err); else if(d.msg && d.msg.includes('AUTO SAFE ROUTE')) setMessage('busy', d.msg);
    } catch(e) { setMessage('err','Connection error.'); }
  },250);
}

async function sendIK() {
  if (!homed){setMessage('err','Press HOME first.');return;}
  const x=document.getElementById('ikX').value;
  const y=document.getElementById('ikY').value;
  const z=document.getElementById('ikZ').value;
  try{
    const r=await fetch('/ik?x='+x+'&y='+y+'&z='+z+'&config='+ikConfiguration);
    const d=await r.json();
    if(d.ok) setMessage(d.msg && d.msg.includes('AUTO SAFE ROUTE') ? 'busy' : 'ok', (d.msg ? d.msg + ' — ' : '') + d.configuration+': A1 '+d.a1+'°, A2 '+d.a2+'°');
    else setMessage('err',d.err);
  }catch(e){setMessage('err','Connection error.');}
}

async function sendHome() {
  if (!safety.startupConfirmed) {
    setMessage('err', 'HOME blocked. Put the arm in its Safe Startup Pose and confirm it first.');
    return;
  }
  const btn = document.getElementById('homeBtn');
  btn.disabled = true;
  homingPending = true;
  setMessage('busy', 'Homing started. Wait until the state becomes Ready.');
  try {
    const r = await fetch('/home', {cache:'no-store'});
    const d = await r.json();
    if (!d.ok) {
      homingPending = false;
      btn.disabled = false;
      setMessage('err', d.err || 'Homing could not start.');
    }
  } catch(e) {
    homingPending = false;
    btn.disabled = false;
    setMessage('err', 'Could not start HOME request.');
  }
}

async function sendStop() {
  try{
    const r=await fetch('/stop');
    const d=await r.json();
    homed=false;
    homingPending=false;
    safety.startupConfirmed=false;
    updateStartupStatus();
    document.getElementById('homeBtn').disabled=true;
    document.getElementById('warn').classList.remove('hidden');
    setMessage('err',d.msg);
  }catch(e){setMessage('err','STOP connection error. Cut motor power if needed.');}
}

async function sendOff() {
  try{
    const r=await fetch('/off');
    const d=await r.json();
    homed=false;
    homingPending=false;
    safety.startupConfirmed=false;
    updateStartupStatus();
    document.getElementById('homeBtn').disabled=true;
    document.getElementById('warn').classList.remove('hidden');
    setMessage('err',d.msg);
  }catch(e){setMessage('err','OFF connection error.');}
}

async function poll() {
  try {
    const r = await fetch('/position', {cache:'no-store'});
    const d = await r.json();
    homed = d.homed;
    setupRequired = !!d.setupRequired;
    safety.startupConfirmed = !!d.startupPoseConfirmed;
    safety.parkSaved = !!d.startupParkSaved;
    safety.collisionEnabled = !!d.collisionEnabled;
    updateStartupStatus();
    references = {z:!!d.zReferenced, a1:!!d.a1Referenced, a2:!!d.a2Referenced};
    lastActual = {a1:d.a1, a2:d.a2, x:d.x, y:d.y, z:d.z};
    if (Number.isFinite(d.a1Min)) { A1_MIN=d.a1Min; A1_MAX=d.a1Max; A2_MIN=d.a2Min; A2_MAX=d.a2Max; Z_MIN=d.zMin; Z_MAX=d.zMax; }

    document.getElementById('actA1').textContent = d.a1.toFixed(1);
    document.getElementById('actA2').textContent = d.a2.toFixed(1);
    document.getElementById('actX').textContent = d.x.toFixed(2);
    document.getElementById('actY').textContent = d.y.toFixed(2);
    document.getElementById('actZ').textContent = d.z.toFixed(1);
    document.getElementById('activeProfile').textContent = d.activeProfile || '--';
    document.getElementById('activePct').textContent = (d.activeSpeedPct ?? 0) + '%';
    document.getElementById('liveA1Speed').textContent = (d.a1VelocityDegS ?? 0).toFixed(2);
    document.getElementById('liveA2Speed').textContent = (d.a2VelocityDegS ?? 0).toFixed(2);
    document.getElementById('liveZSpeed').textContent = (d.zVelocityMmS ?? 0).toFixed(2);
    document.getElementById('peakA1Speed').textContent = (d.a1PeakDegS ?? 0).toFixed(2);
    document.getElementById('peakA2Speed').textContent = (d.a2PeakDegS ?? 0).toFixed(2);
    document.getElementById('peakZSpeed').textContent = (d.zPeakMmS ?? 0).toFixed(2);
    document.getElementById('actGrip').textContent = Number.isFinite(d.gripperCommandDeg) ? d.gripperCommandDeg : '--';
    document.getElementById('envActualA1').textContent = Number(d.a1).toFixed(2);
    document.getElementById('envActualA2').textContent = Number(d.a2).toFixed(2);
    document.getElementById('envActualZ').textContent = Number(d.z).toFixed(2);
    updateCurrentEnvelopeLimit(d);
    applyRecipeRuntime(d);
    applyJobRuntime(d);
    if (Number.isFinite(d.gripperOpenDeg)) applyGripperState(d, true);
    if (Number.isFinite(d.manualSpeedPct) && !speedRequestInFlight && Date.now() > speedEditActiveUntil) {
      speeds.manual = d.manualSpeedPct;
      speeds.realtime = d.realtimeSpeedPct;
      speeds.ik = d.ikSpeedPct;
      updateSpeedPanel();
    }
    if (Number.isFinite(d.toolTcpLengthMm)) L2 = d.toolTcpLengthMm;
    const route = document.getElementById('routeReadout');
    if (route) route.textContent = 'Motion plan: ' + (d.motionPlan || 'Direct / idle') +
      (d.collisionRouteActive ? ' | ' + d.collisionRoutePhase : '');
    if (mode === 'ik') previewIK();

    document.getElementById('dot').style.cssText = 'background:#66c27a;box-shadow:0 0 8px #66c27a';

    if (d.fault) {
      homingPending = false;
      document.getElementById('homeBtn').disabled = false;
      document.getElementById('stateLine').textContent = 'State: FAULT — ' + d.faultMessage;
      setMessage('err', d.faultMessage);
    } else if (d.homing) {
      document.getElementById('stateLine').textContent = 'State: Homing...';
      setMessage('busy', 'Homing in progress...');
    } else if (d.setupRequired) {
      homingPending = false;
      document.getElementById('homeBtn').disabled = false;
      document.getElementById('stateLine').textContent = 'State: REFERENCED SETUP REQUIRED';
      document.getElementById('setupStateReadout').textContent =
        'Referenced axes — Z: ' + (d.zReferenced?'YES':'NO') +
        ' | A1: ' + (d.a1Referenced?'YES':'NO') +
        ' | A2: ' + (d.a2Referenced?'YES':'NO') +
        '. Repair calibration below, then press HOME again.';
      setMessage('err', d.faultMessage || 'Stored calibration requires repair in Setup.');
    } else if (d.moving) {
      document.getElementById('stateLine').textContent = 'State: Moving';
    } else if (d.homed) {
      if (homingPending) {
        document.getElementById('a1').value = 0;
        document.getElementById('a2').value = 0;
        document.getElementById('z').value = 0;
        syncTargetDisplay();
      }
      homingPending = false;
      document.getElementById('homeBtn').disabled = !safety.startupConfirmed;
      document.getElementById('stateLine').textContent = d.motors ? 'State: Ready / motors powered' : 'State: Ready / motors resting';
      setMessage('ok', d.collisionEnabled ? 'Ready. Collision rules enabled.' : 'Ready. Collision rules disabled.');
    } else {
      document.getElementById('homeBtn').disabled = !safety.startupConfirmed;
      document.getElementById('stateLine').textContent = 'State: Not homed / startup confirmation required';
    }

    if (d.setupRequired && mode !== 'calibration') {
      document.getElementById('warn').classList.remove('hidden');
      document.getElementById('warn').textContent = 'Calibration repair required. Open Setup. Normal FK/IK control is blocked.';
    } else if (d.homed && !d.collisionEnabled) {
      document.getElementById('warn').classList.remove('hidden');
      document.getElementById('warn').textContent = 'Collision rules are disabled. Enable them before autonomous movement.';
    } else if (d.homed && d.collisionEnabled) {
      document.getElementById('warn').classList.add('hidden');
    } else {
      document.getElementById('warn').classList.remove('hidden');
      document.getElementById('warn').textContent = safety.startupConfirmed
        ? 'Safe Startup Pose confirmed. Press HOME.'
        : 'HOME blocked: physically place the arm in the Safe Startup Pose and confirm it first.';
    }

    drawArm(d.a1, d.a2);
  } catch(e) {
    if (!homingPending) {
      document.getElementById('dot').style.cssText = 'background:#ee7777';
      document.getElementById('stateLine').textContent = 'State: disconnected';
    }
  }
}

window.addEventListener('load',()=>{
  document.getElementById('homeBtn').disabled = true;
  syncTargetDisplay();
  loadConfiguration();
  updateSpeedPanel();
  poll();
  setInterval(poll,350);
});
</script>
</body>
</html>
)rawhtml";

#endif
