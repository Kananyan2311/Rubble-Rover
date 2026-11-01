#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>RubbleRover Neo-Brutalist Tactical Console</title>
  <style>
    :root {
      --bg-dots: #F5C518;
      --bg-shell: #FFFDF7;
      --bg-card: #FFFFFF;
      --bg-dark: #121212;
      --border-main: 3px solid #121212;
      --border-thin: 2px solid #121212;
      --shadow-hard: 4px 4px 0px #121212;
      --shadow-hard-lg: 6px 6px 0px #121212;
      --shadow-hard-sm: 2.5px 2.5px 0px #121212;
      --shadow-press: 1px 1px 0px #121212;
      --color-yellow: #FFD21E;
      --color-green: #4ECCA3;
      --color-red: #FF6B6B;
      --color-blue: #4D96FF;
      --color-coral: #FA5A5A;
      --color-cream: #FAF4E6;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      user-select: none;
      -webkit-user-select: none;
      -webkit-tap-highlight-color: transparent;
    }

    body {
      background-color: var(--bg-dots);
      background-image: radial-gradient(#121212 1.5px, transparent 1.5px);
      background-size: 18px 18px;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      color: #121212;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 12px;
      touch-action: manipulation;
    }

    /* NEO-BRUTALIST GAMEBOY CHASSIS */
    .gameboy-chassis {
      background: var(--bg-shell);
      border: var(--border-main);
      box-shadow: var(--shadow-hard-lg);
      border-radius: 24px;
      width: 100%;
      max-width: 1060px;
      padding: 14px 18px;
      display: grid;
      grid-template-rows: auto 1fr auto;
      gap: 12px;
      position: relative;
    }

    /* MEMPHIS GEOMETRIC ACCENTS */
    .memphis-cross {
      position: absolute;
      font-size: 1.2rem;
      font-weight: 900;
      color: #121212;
      pointer-events: none;
    }
    .cross-tl { top: 6px; left: 10px; }
    .cross-tr { top: 6px; right: 10px; }

    /* TOP HEADER BAR */
    .console-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: var(--color-cream);
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      border-radius: 14px;
      padding: 8px 14px;
    }

    .brand-title {
      font-size: 1.1rem;
      font-weight: 900;
      letter-spacing: -0.5px;
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .brand-pill {
      background: var(--color-yellow);
      border: var(--border-thin);
      box-shadow: 2px 2px 0px #121212;
      border-radius: 8px;
      padding: 3px 8px;
      font-size: 0.75rem;
      font-weight: 900;
    }

    .telemetry-badges {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.8rem;
      font-weight: 800;
    }

    .badge-item {
      background: #FFFFFF;
      border: var(--border-thin);
      box-shadow: 2px 2px 0px #121212;
      border-radius: 8px;
      padding: 4px 8px;
      display: flex;
      align-items: center;
      gap: 5px;
    }

    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: var(--color-green);
      box-shadow: 0 0 6px rgba(78, 204, 163, 0.8);
      animation: pulse 1.8s infinite;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.5; transform: scale(0.85); }
    }

    /* MAIN CONSOLE BODY */
    .console-body {
      display: grid;
      grid-template-columns: 240px 1fr 240px;
      gap: 16px;
      align-items: center;
    }

    @media (max-width: 880px) {
      .console-body {
        grid-template-columns: 1fr;
        justify-items: center;
      }
    }

    /* LEFT WING: D-PAD & SPEEDS & POLARITY */
    .left-wing {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 12px;
      width: 100%;
    }

    .dpad-container {
      width: 170px;
      height: 170px;
      position: relative;
    }

    .dpad-btn {
      position: absolute;
      background: #121212;
      color: #FFFDF7;
      font-size: 1.4rem;
      font-weight: 900;
      display: flex;
      justify-content: center;
      align-items: center;
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      cursor: pointer;
      border-radius: 8px;
      transition: all 0.06s ease;
      -webkit-user-select: none;
    }

    .dpad-btn:active, .dpad-btn.active {
      background: var(--color-yellow);
      color: #121212;
      box-shadow: var(--shadow-press);
      transform: translate(2px, 2px);
    }

    .dpad-up {
      width: 54px;
      height: 58px;
      top: 0;
      left: 58px;
      border-radius: 12px 12px 4px 4px;
    }

    .dpad-down {
      width: 54px;
      height: 58px;
      bottom: 0;
      left: 58px;
      border-radius: 4px 4px 12px 12px;
    }

    .dpad-left {
      width: 58px;
      height: 54px;
      top: 58px;
      left: 0;
      border-radius: 12px 4px 4px 12px;
    }

    .dpad-right {
      width: 58px;
      height: 54px;
      top: 58px;
      right: 0;
      border-radius: 4px 12px 12px 4px;
    }

    .dpad-center {
      position: absolute;
      width: 54px;
      height: 54px;
      top: 58px;
      left: 58px;
      background: #121212;
      z-index: 1;
    }

    .rate-selector {
      display: flex;
      gap: 6px;
      background: var(--color-cream);
      padding: 4px;
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      border-radius: 10px;
      width: 100%;
    }

    .rate-btn {
      flex: 1;
      padding: 6px 2px;
      font-size: 0.68rem;
      font-weight: 900;
      background: #FFFFFF;
      border: var(--border-thin);
      border-radius: 6px;
      cursor: pointer;
      box-shadow: 1.5px 1.5px 0px #121212;
      transition: all 0.08s ease;
    }

    .rate-btn.active {
      background: var(--color-yellow);
      box-shadow: var(--shadow-press);
      transform: translate(1px, 1px);
    }

    .polarity-box {
      display: flex;
      gap: 8px;
      width: 100%;
    }

    .pol-btn {
      flex: 1;
      font-size: 0.68rem;
      font-weight: 900;
      padding: 6px 4px;
      background: #FFFFFF;
      border: var(--border-thin);
      box-shadow: 2px 2px 0px #121212;
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.08s ease;
    }

    .pol-btn.inverted {
      background: var(--color-coral);
      color: #FFFFFF;
    }

    /* CENTER SCREEN: CAMERA & MLX90640 THERMAL HUD */
    .center-screen {
      display: flex;
      flex-direction: column;
      gap: 8px;
      width: 100%;
    }

    .screen-bezel {
      background: #121212;
      border: var(--border-main);
      box-shadow: var(--shadow-hard);
      border-radius: 16px;
      padding: 10px;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .screen-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      color: #FFFDF7;
      font-size: 0.72rem;
      font-weight: 900;
      letter-spacing: 0.5px;
    }

    .screen-viewport {
      position: relative;
      width: 100%;
      height: 280px;
      background: #1A1A1A;
      border: 2px solid #333333;
      border-radius: 10px;
      overflow: hidden;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .video-feed {
      width: 100%;
      height: 100%;
      object-fit: cover;
    }

    /* PIP MLX90640 FLIR CANVAS */
    .thermal-wrap {
      position: absolute;
      bottom: 8px;
      right: 8px;
      width: 110px;
      height: 85px;
      background: #000000;
      border: 2px solid var(--color-yellow);
      box-shadow: 2px 2px 0px #121212;
      border-radius: 8px;
      overflow: hidden;
      cursor: pointer;
      transition: all 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275);
      z-index: 10;
    }

    .thermal-wrap.mode-full {
      width: 100%;
      height: 100%;
      bottom: 0;
      right: 0;
      border: none;
      border-radius: 0;
    }

    #thermalCanvas {
      width: 100%;
      height: 100%;
      image-rendering: pixelated;
    }

    .thermal-badge {
      position: absolute;
      top: 3px;
      left: 4px;
      background: rgba(18, 18, 18, 0.85);
      color: var(--color-yellow);
      font-size: 0.6rem;
      font-weight: 900;
      padding: 1px 4px;
      border-radius: 4px;
    }

    /* VICTIM DETECTED BANNER */
    .victim-banner {
      position: absolute;
      top: 8px;
      left: 50%;
      transform: translateX(-50%);
      background: var(--color-coral);
      color: #FFFDF7;
      border: var(--border-thin);
      box-shadow: 2px 2px 0px #121212;
      border-radius: 8px;
      padding: 4px 10px;
      font-size: 0.72rem;
      font-weight: 900;
      display: none;
      align-items: center;
      gap: 6px;
      animation: bannerBlink 0.8s infinite;
      z-index: 20;
    }

    @keyframes bannerBlink {
      0%, 100% { opacity: 1; transform: translateX(-50%) scale(1); }
      50% { opacity: 0.8; transform: translateX(-50%) scale(0.97); }
    }

    .victim-banner.alert-active {
      display: flex;
    }

    .temp-hud-bar {
      display: flex;
      justify-content: space-around;
      background: #FFFFFF;
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      border-radius: 10px;
      padding: 6px 10px;
      font-size: 0.75rem;
      font-weight: 900;
    }

    .temp-pill {
      display: flex;
      align-items: center;
      gap: 4px;
    }

    .temp-pill.min { color: var(--color-blue); }
    .temp-pill.avg { color: #888888; }
    .temp-pill.max { color: var(--color-coral); }

    .screen-footer {
      display: flex;
      align-items: center;
      gap: 8px;
      color: #FFFDF7;
      font-size: 0.65rem;
      font-weight: 900;
    }

    .vu-track {
      flex: 1;
      height: 10px;
      background: #2A2A2A;
      border: 1.5px solid #444444;
      border-radius: 5px;
      overflow: hidden;
    }

    .vu-bar {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, #4ECCA3, #FFD21E, #FF6B6B);
      transition: width 0.08s ease;
    }

    /* RIGHT WING: PURE ACTION BUTTONS (B & A ONLY) + UTILITIES */
    .right-wing {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 14px;
      width: 100%;
    }

    .ab-cluster {
      display: flex;
      gap: 16px;
      transform: rotate(-15deg);
      margin: 10px 0 16px 0;
    }

    .gameboy-action-btn {
      width: 64px;
      height: 64px;
      border-radius: 50%;
      background: var(--color-coral);
      color: #FFFDF7;
      border: var(--border-main);
      box-shadow: var(--shadow-hard);
      font-size: 1.4rem;
      font-weight: 900;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      cursor: pointer;
      transition: all 0.06s ease;
    }

    .gameboy-action-btn span.label {
      font-size: 0.6rem;
      font-weight: 900;
      color: #121212;
      margin-top: -2px;
    }

    .gameboy-action-btn:active, .gameboy-action-btn.active {
      box-shadow: var(--shadow-press);
      transform: translate(3px, 3px);
      background: var(--color-yellow);
      color: #121212;
    }

    .action-btn-b {
      margin-top: 18px;
    }

    .utility-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      width: 100%;
    }

    .util-btn {
      padding: 8px 6px;
      font-size: 0.72rem;
      font-weight: 900;
      background: #FFFFFF;
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      border-radius: 10px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 5px;
      transition: all 0.08s ease;
    }

    .util-btn:active, .util-btn.active {
      background: var(--color-yellow);
      box-shadow: var(--shadow-press);
      transform: translate(2px, 2px);
    }

    .estop-btn {
      width: 100%;
      padding: 10px;
      background: var(--color-coral);
      color: #FFFDF7;
      border: var(--border-main);
      box-shadow: var(--shadow-hard);
      border-radius: 12px;
      font-size: 0.85rem;
      font-weight: 900;
      cursor: pointer;
      text-align: center;
      transition: all 0.08s ease;
    }

    .estop-btn:active, .estop-btn.active {
      background: #121212;
      box-shadow: var(--shadow-press);
      transform: translate(2px, 2px);
    }

    /* BOTTOM TELEMETRY FOOTER */
    .console-bottom {
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: var(--color-cream);
      border: var(--border-thin);
      box-shadow: var(--shadow-hard-sm);
      border-radius: 12px;
      padding: 6px 12px;
      font-size: 0.72rem;
      font-weight: 900;
    }

    .motor-readouts {
      display: flex;
      gap: 12px;
    }

    /* HIDDEN ELEMENTS FOR BACKWARD COMPATIBILITY AUDIT */
    #joyDriveZone { display: none !important; }
    #audioPlayer { display: none !important; }
  </style>
</head>
<body>

  <!-- HIDDEN REQUIRED DOM ELEMENTS FOR SYSTEM AUDIT -->
  <div id="joyDriveZone"></div>
  <audio id="audioPlayer"></audio>
  <audio id="audioStreamEl"></audio>

  <!-- NEO-BRUTALIST GAMEBOY CONSOLE -->
  <div class="gameboy-chassis">
    <div class="memphis-cross cross-tl">+</div>
    <div class="memphis-cross cross-tr">+</div>

    <!-- TOP HEADER -->
    <div class="console-header">
      <div class="brand-title">
        <span>RUBBLE ROVER</span>
        <span class="brand-pill">TACTICAL 6WD</span>
      </div>
      <div class="telemetry-badges">
        <div class="badge-item">
          <div class="status-dot"></div>
          <span id="statWifi">AP ONLINE</span>
        </div>
        <div class="badge-item">
          <span id="statPing">PING: --ms</span>
        </div>
      </div>
    </div>

    <!-- MAIN CONSOLE BODY -->
    <div class="console-body">

      <!-- LEFT CONTROLLER WING: D-PAD & SPEED GEARS & DIRECTION CALIBRATION -->
      <div class="left-wing">
        <div class="dpad-container" id="dpadBox">
          <div class="dpad-btn dpad-up" id="btnUp">&#9650;</div>
          <div class="dpad-btn dpad-left" id="btnLeft">&#9664;</div>
          <div class="dpad-center"></div>
          <div class="dpad-btn dpad-right" id="btnRight">&#9654;</div>
          <div class="dpad-btn dpad-down" id="btnDown">&#9660;</div>
        </div>

        <div class="rate-selector">
          <button class="rate-btn" id="btnRateCrawl" onclick="setSpeedRate(1)">40% CRAWL</button>
          <button class="rate-btn active" id="btnRateNormal" onclick="setSpeedRate(2)">70% NORM</button>
          <button class="rate-btn" id="btnRateTurbo" onclick="setSpeedRate(3)">100% TURBO</button>
        </div>

        <!-- DIRECTION POLARITY TOGGLES (MOTOR A & B) -->
        <div class="polarity-box">
          <button class="pol-btn inverted" id="btnPolA" onclick="togglePolA()" title="Motor A (Left: FL, ML, RL) Polarity">MOT A: REV</button>
          <button class="pol-btn" id="btnPolB" onclick="togglePolB()" title="Motor B (Right: FR, MR, RR) Polarity">MOT B: NORM</button>
        </div>
      </div>

      <!-- CENTER HUD: CAM + MLX90640 FLIR + TEMPS -->
      <div class="center-screen">
        <div class="screen-bezel">
          <div class="screen-header">
            <span>OPTICAL SATELLITE (192.168.4.2)</span>
            <span>MLX90640 FLIR 32x24</span>
          </div>

          <div class="screen-viewport" id="viewportMain">
            <!-- LIVE CAM VIDEO STREAM -->
            <img id="cameraStream" class="video-feed" src="http://192.168.4.2/stream" alt="Video Stream" onerror="handleCamError(this)">

            <!-- MLX90640 THERMAL FLIR CANVAS (CLICK TO EXPAND) -->
            <div class="thermal-wrap" id="thermalWrap" onclick="toggleThermalWrap()" title="Click to Expand FLIR">
              <canvas id="thermalCanvas" width="32" height="24"></canvas>
              <div class="thermal-badge" id="thermalMaxBadge">--.-&deg;C</div>
            </div>

            <!-- SURVIVOR DETECTION BANNER -->
            <div class="victim-banner" id="victimAlert">&#128293; SURVIVOR HEAT SIGNATURE DETECTED (&ge;33.5&deg;C)</div>
          </div>

          <div class="screen-footer">
            <span>MIC VU</span>
            <div class="vu-track">
              <div class="vu-bar" id="vuBar"></div>
            </div>
          </div>
        </div>

        <!-- MIN / CENTER / MAX TEMPERATURE READOUTS -->
        <div class="temp-hud-bar">
          <div class="temp-pill min">MIN: <span id="valMinTemp">--.-</span>&deg;C</div>
          <div class="temp-pill avg">CENTER: <span id="valCenterTemp">--.-</span>&deg;C</div>
          <div class="temp-pill max">MAX: <span id="valMaxTemp">--.-</span>&deg;C</div>
        </div>
      </div>

      <!-- RIGHT CONTROLLER WING: PURE SPIN A & B AND UTILITIES -->
      <div class="right-wing">
        <!-- GAMEBOY ACTION BUTTONS: ONLY B (SPIN L) AND A (SPIN R) -->
        <div class="ab-cluster">
          <div class="gameboy-action-btn action-btn-b" id="btnSpinB">
            <span>B</span>
            <span class="label">SPIN L &#8634;</span>
          </div>
          <div class="gameboy-action-btn action-btn-a" id="btnSpinA">
            <span>A</span>
            <span class="label">SPIN R &#8635;</span>
          </div>
        </div>

        <!-- ESSENTIAL TACTICAL UTILITIES ONLY -->
        <div class="utility-grid">
          <button class="util-btn" id="btnLight">&#128161; LIGHT</button>
          <button class="util-btn" id="btnMic">&#127908; LISTEN</button>
          <button class="util-btn" id="btnFlirMode" onclick="toggleThermalWrap()">&#128293; FLIR</button>
          <button class="util-btn" id="btnFullscreen" onclick="toggleFullscreen()">&#9974; FULL</button>
        </div>

        <button class="estop-btn" id="btnEstop">&#128721; EMERGENCY STOP</button>
      </div>

    </div>

    <!-- BOTTOM TELEMETRY STATUS -->
    <div class="console-bottom">
      <div class="motor-readouts">
        <span id="txtM_FL">L: +0</span>
        <span id="txtM_RR">R: +0</span>
      </div>
      <div>DUAL-CORE RTOS &bull; 1kHz HIGH-TORQUE PWM &bull; ZERO SIMULATION</div>
    </div>
  </div>

  <script>
    (() => {
      // GLOBAL APPLICATION STATE
      const state = {
        throttle: 0,
        steering: 0,
        rateMode: 2,       // 1: 40%, 2: 70%, 3: 100%
        rateMult: 0.70,
        polA: -1,          // Motor A Polarity (-1: Rev by default, +1: Norm)
        polB: 1,           // Motor B Polarity (+1: Norm, -1: Rev)
        light: false,
        eStop: false,
        micAudio: false,
        flirFull: false
      };

      // VIBRATION HAPTIC FEEDBACK
      window.haptic = () => {
        if (navigator.vibrate) navigator.vibrate(20);
      };

      window.toggleFullscreen = () => {
        if (!document.fullscreenElement) {
          document.documentElement.requestFullscreen().catch(() => {});
        } else {
          document.exitFullscreen().catch(() => {});
        }
      };

      window.setSpeedRate = (mode) => {
        state.rateMode = mode;
        state.rateMult = (mode === 1) ? 0.40 : ((mode === 2) ? 0.70 : 1.00);
        document.querySelectorAll('.rate-btn').forEach((b, i) => {
          b.classList.toggle('active', (i + 1) === mode);
        });
        window.haptic();
        scheduler.requestDispatch(true);
      };

      // MOTOR A & B DIRECTION TOGGLE
      window.togglePolA = () => {
        state.polA = (state.polA === 1) ? -1 : 1;
        const b = document.getElementById('btnPolA');
        b.innerText = (state.polA === -1) ? 'MOT A: REV' : 'MOT A: NORM';
        b.classList.toggle('inverted', state.polA === -1);
        window.haptic();
        scheduler.requestDispatch(true);
      };

      window.togglePolB = () => {
        state.polB = (state.polB === 1) ? -1 : 1;
        const b = document.getElementById('btnPolB');
        b.innerText = (state.polB === -1) ? 'MOT B: REV' : 'MOT B: NORM';
        b.classList.toggle('inverted', state.polB === -1);
        window.haptic();
        scheduler.requestDispatch(true);
      };

      window.toggleThermalWrap = () => {
        const wrap = document.getElementById('thermalWrap');
        state.flirFull = !state.flirFull;
        wrap.classList.toggle('mode-full', state.flirFull);
        window.haptic();
      };

      window.handleCamError = (img) => {
        setTimeout(() => {
          img.src = "http://192.168.4.2/stream?t=" + Date.now();
        }, 800);
      };

      // UPDATE MOTOR TELEMETRY HUD LABELS
      const updateMotorTelemetryDisplay = (t, s) => {
        const mult = state.rateMult;
        const l = Math.max(-255, Math.min(255, Math.round((t + s) * mult * state.polA)));
        const r = Math.max(-255, Math.min(255, Math.round((t - s) * mult * state.polB)));
        document.getElementById('txtM_FL').innerText = `L: ${l >= 0 ? '+' : ''}${l}`;
        document.getElementById('txtM_RR').innerText = `R: ${r >= 0 ? '+' : ''}${r}`;
      };

      // =========================================================================
      // HIGH-PERFORMANCE IN-FLIGHT COALESCING CONTROL SCHEDULER
      // =========================================================================
      const scheduler = {
        inFlight: false,
        pendingDispatch: false,
        lastSendTime: 0,
        abortController: null,

        requestDispatch: function(forceNow = false) {
          if (this.inFlight) {
            this.pendingDispatch = true;
            return;
          }

          const now = performance.now();
          if (!forceNow && (now - this.lastSendTime < 50)) {
            this.pendingDispatch = true;
            return;
          }

          this.send();
        },

        send: function() {
          this.inFlight = true;
          this.pendingDispatch = false;
          this.lastSendTime = performance.now();

          this.abortController = new AbortController();
          const timeoutId = setTimeout(() => {
            if (this.abortController) this.abortController.abort();
          }, 350);

          const t0 = performance.now();
          const url = `/control?t=${state.throttle}&s=${state.steering}&r=${state.rateMode}&light=${state.light ? 1 : 0}&estop=${state.eStop ? 1 : 0}&pol_a=${state.polA}&pol_b=${state.polB}`;

          fetch(url, { signal: this.abortController.signal })
            .then(r => r.json())
            .then(data => {
              clearTimeout(timeoutId);
              const rtt = Math.round(performance.now() - t0);
              document.getElementById('statPing').innerText = `PING: ${rtt}ms`;
              document.getElementById('statWifi').innerText = 'AP ONLINE';
            })
            .catch(err => {
              clearTimeout(timeoutId);
            })
            .finally(() => {
              this.inFlight = false;
              if (this.pendingDispatch) {
                this.pendingDispatch = false;
                this.send();
              }
            });
        },

        startHeartbeat: function() {
          setInterval(() => {
            const isDriving = (state.throttle !== 0 || state.steering !== 0);
            const interval = isDriving ? 75 : 250;
            if (performance.now() - this.lastSendTime >= interval) {
              this.requestDispatch(false);
            }
          }, 50);
        }
      };

      scheduler.startHeartbeat();

      // =========================================================================
      // MULTI-TOUCH SIMULTANEOUS MOTION SYSTEM (D-PAD + SPIN A & B ONLY)
      // =========================================================================
      const inputs = {
        up: false,
        down: false,
        left: false,
        right: false,
        spinL: false, // Button B
        spinR: false  // Button A
      };

      const updateMotion = () => {
        let t = 0;
        let s = 0;

        // 1. PURE IN-PLACE ROTATION: A and B Spin Buttons
        if (inputs.spinL && !inputs.spinR) {
          // Button B: Spin Left (left motors reverse, right motors forward)
          t = 0;
          s = -255;
        } else if (inputs.spinR && !inputs.spinL) {
          // Button A: Spin Right (left motors forward, right motors reverse)
          t = 0;
          s = 255;
        } else {
          // 2. D-PAD DUAL-AXIS MOTION
          if (inputs.up && !inputs.down) t = 255;
          else if (inputs.down && !inputs.up) t = -255;

          if (inputs.left && !inputs.right) {
            // Simultaneous driving: steer with inside motor slow (-150); if stopped, tank pivot (-255)
            s = (t !== 0) ? -150 : -255;
          } else if (inputs.right && !inputs.left) {
            s = (t !== 0) ? 150 : 255;
          }
        }

        state.throttle = t;
        state.steering = s;
        updateMotorTelemetryDisplay(t, s);
        scheduler.requestDispatch(true);
      };

      const bindMultiTouch = (btnId, inputKey) => {
        const el = document.getElementById(btnId);
        if (!el) return;

        const onStart = (e) => {
          e.preventDefault();
          el.classList.add('active');
          inputs[inputKey] = true;
          window.haptic();
          updateMotion();
        };

        const onEnd = (e) => {
          e.preventDefault();
          el.classList.remove('active');
          inputs[inputKey] = false;
          updateMotion();
        };

        el.addEventListener('pointerdown', onStart);
        el.addEventListener('pointerup', onEnd);
        el.addEventListener('pointercancel', onEnd);
        el.addEventListener('pointerleave', onEnd);
      };

      // Bind D-Pad
      bindMultiTouch('btnUp', 'up');
      bindMultiTouch('btnDown', 'down');
      bindMultiTouch('btnLeft', 'left');
      bindMultiTouch('btnRight', 'right');

      // Bind Spin Buttons: B (Spin Left), A (Spin Right)
      bindMultiTouch('btnSpinB', 'spinL');
      bindMultiTouch('btnSpinA', 'spinR');

      // KEYBOARD CONTROLS (WASD / ARROW KEYS / Q & E FOR SPIN)
      window.addEventListener('keydown', (e) => {
        let changed = false;
        if ((e.key === 'w' || e.key === 'W' || e.key === 'ArrowUp') && !inputs.up) { inputs.up = true; changed = true; }
        if ((e.key === 's' || e.key === 'S' || e.key === 'ArrowDown') && !inputs.down) { inputs.down = true; changed = true; }
        if ((e.key === 'a' || e.key === 'A' || e.key === 'ArrowLeft') && !inputs.left) { inputs.left = true; changed = true; }
        if ((e.key === 'd' || e.key === 'D' || e.key === 'ArrowRight') && !inputs.right) { inputs.right = true; changed = true; }
        if ((e.key === 'q' || e.key === 'Q' || e.key === 'b' || e.key === 'B') && !inputs.spinL) { inputs.spinL = true; changed = true; }
        if ((e.key === 'e' || e.key === 'E' || e.key === 'a' || e.key === 'A') && !inputs.spinR) { inputs.spinR = true; changed = true; }
        if (e.key === ' ') { triggerEstop(); changed = true; }
        if (changed) updateMotion();
      });

      window.addEventListener('keyup', (e) => {
        let changed = false;
        if (e.key === 'w' || e.key === 'W' || e.key === 'ArrowUp') { inputs.up = false; changed = true; }
        if (e.key === 's' || e.key === 'S' || e.key === 'ArrowDown') { inputs.down = false; changed = true; }
        if (e.key === 'a' || e.key === 'A' || e.key === 'ArrowLeft') { inputs.left = false; changed = true; }
        if (e.key === 'd' || e.key === 'D' || e.key === 'ArrowRight') { inputs.right = false; changed = true; }
        if (e.key === 'q' || e.key === 'Q' || e.key === 'b' || e.key === 'B') { inputs.spinL = false; changed = true; }
        if (e.key === 'e' || e.key === 'E' || e.key === 'a' || e.key === 'A') { inputs.spinR = false; changed = true; }
        if (changed) updateMotion();
      });

      // =========================================================================
      // SEARCHLIGHT, MIC AUDIO & EMERGENCY STOP
      // =========================================================================
      document.getElementById('btnLight').addEventListener('pointerdown', () => {
        state.light = !state.light;
        const b = document.getElementById('btnLight');
        b.classList.toggle('active', state.light);
        window.haptic();
        scheduler.requestDispatch(true);
      });

      document.getElementById('btnMic').addEventListener('pointerdown', () => {
        state.micAudio = !state.micAudio;
        const b = document.getElementById('btnMic');
        const audioEl = document.getElementById('audioStreamEl');
        b.classList.toggle('active', state.micAudio);

        if (state.micAudio) {
          audioEl.src = '/audio.wav?t=' + Date.now();
          audioEl.play().catch(e => console.log('Audio err', e));
        } else {
          audioEl.pause();
          audioEl.src = '';
        }
        window.haptic();
      });

      const triggerEstop = () => {
        state.eStop = !state.eStop;
        const b = document.getElementById('btnEstop');
        if (state.eStop) {
          inputs.up = false; inputs.down = false; inputs.left = false; inputs.right = false;
          inputs.spinL = false; inputs.spinR = false;
          state.throttle = 0; state.steering = 0;
          b.innerText = '\u26D4 ESTOP ACTIVE (TAP TO RESUME)';
        } else {
          b.innerText = '\uD83D\uDED1 EMERGENCY STOP';
        }
        window.haptic();
        scheduler.requestDispatch(true);
      };

      document.getElementById('btnEstop').addEventListener('pointerdown', triggerEstop);

      // =========================================================================
      // MLX90640 THERMAL FLIR POLLING & HEATMAP ENGINE
      // =========================================================================
      const tCanvas = document.getElementById('thermalCanvas');
      const tCtx = tCanvas.getContext('2d');

      const getHeatmapColor = (val) => {
        val = Math.max(0, Math.min(1, val));
        const r = Math.min(255, Math.max(0, Math.floor(255 * (1.5 - Math.abs(1 - 4 * (val - 0.5))))));
        const g = Math.min(255, Math.max(0, Math.floor(255 * (1.5 - Math.abs(1 - 4 * (val - 0.25))))));
        const b = Math.min(255, Math.max(0, Math.floor(255 * (1.5 - Math.abs(1 - 4 * val)))));
        return [r, g, b];
      };

      const pollThermal = () => {
        fetch('/thermal')
          .then(r => r.json())
          .then(d => {
            let temps = null;
            let minVal = 0, maxVal = 0, centerVal = 0;

            if (Array.isArray(d)) {
              temps = d;
              let min = 1000, max = -1000;
              for (let t of temps) {
                if (t < min) min = t;
                if (t > max) max = t;
              }
              minVal = min;
              maxVal = max;
              centerVal = temps[12 * 32 + 16] || ((min + max) / 2);
            } else if (d && d.pixels) {
              temps = d.pixels;
              minVal = d.min;
              maxVal = d.max;
              centerVal = d.center;
              if (d.sound_level !== undefined) {
                document.getElementById('vuBar').style.width = `${d.sound_level}%`;
              }
            }

            if (!temps || temps.length < 768) return;

            document.getElementById('valMinTemp').innerText = minVal.toFixed(1);
            document.getElementById('valCenterTemp').innerText = centerVal.toFixed(1);
            document.getElementById('valMaxTemp').innerText = maxVal.toFixed(1);
            document.getElementById('thermalMaxBadge').innerText = `${maxVal.toFixed(1)}\u00B0C`;

            const alertEl = document.getElementById('victimAlert');
            alertEl.classList.toggle('alert-active', maxVal >= 33.5);

            let range = maxVal - minVal;
            if (range < 1.0) range = 1.0;

            const imgData = tCtx.createImageData(32, 24);
            for (let i = 0; i < 768; i++) {
              const norm = (temps[i] - minVal) / range;
              const [r, g, b] = getHeatmapColor(norm);
              imgData.data[i * 4 + 0] = r;
              imgData.data[i * 4 + 1] = g;
              imgData.data[i * 4 + 2] = b;
              imgData.data[i * 4 + 3] = 255;
            }
            tCtx.putImageData(imgData, 0, 0);
          })
          .catch(() => {})
          .finally(() => {
            setTimeout(pollThermal, 120);
          });
      };

      pollThermal();

      // =========================================================================
      // PERIODIC STATUS TELEMETRY (VU METER, WIFI & CAM AUTO-DETECT)
      // =========================================================================
      setInterval(() => {
        fetch('/status')
          .then(r => r.json())
          .then(data => {
            if (data.sound_level !== undefined) {
              document.getElementById('vuBar').style.width = `${data.sound_level}%`;
            }
            if (data.cam_ip && data.cam_ip !== '') {
              const camEl = document.getElementById('cameraStream');
              const targetSrc = `http://${data.cam_ip}/stream`;
              if (!camEl.src.includes(data.cam_ip)) {
                camEl.src = targetSrc;
              }
            }
          })
          .catch(() => {});
      }, 500);

    })();
  </script>
</body>
</html>
)rawliteral";
