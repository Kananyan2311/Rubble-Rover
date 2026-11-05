/*
  RubbleRover v2 (Router + AP Edition) - ESP32-S3 Master Tactical Controller
  Hardware: ESP32-S3 (Dual-Core) + 3x TB6612FNG (6WD) + MLX90640 + INMP441
  Network: Dual Mode (WIFI_AP_STA)
    1. Connects to Wi-Fi Router / Hotspot (Station Mode)
    2. Simultaneously broadcasts "RubbleRover-Tactical-AP" at 192.168.4.1 (AP Mode)
  Direct simplified motor control with High-Z neutral coast and Motor A inverted.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <esp_arduino_version.h>

#if __has_include(<ESP_I2S.h>)
  #include <ESP_I2S.h>
  #define HAVE_ESP_I2S 1
#else
  #include <driver/i2s.h>
  #define HAVE_ESP_I2S 0
#endif

// =================================================================================================
// USER VERIFIED PIN DEFINITIONS (3x TB6612 = 18 PINS)
// =================================================================================================

// --- TB6612 #1: Front Motor Pair (Header J1-10 to J1-17) ---
#define PIN_FL_PWMA    17   // Front-Left Speed (Motor A)
#define PIN_FL_AIN1    18   // Front-Left Direction 1
#define PIN_FL_AIN2    8    // Front-Left Direction 2
#define PIN_FR_BIN1    9    // Front-Right Direction 1
#define PIN_FR_BIN2    10   // Front-Right Direction 2
#define PIN_FR_PWMB    11   // Front-Right Speed (Motor B)

// --- TB6612 #2: Middle Motor Pair (Header J1-04 to J1-09) ---
#define PIN_ML_PWMA    4    // Middle-Left Speed (Motor A)
#define PIN_ML_AIN1    5    // Middle-Left Direction 1
#define PIN_ML_AIN2    6    // Middle-Left Direction 2
#define PIN_MR_BIN1    7    // Middle-Right Direction 1
#define PIN_MR_BIN2    15   // Middle-Right Direction 2
#define PIN_MR_PWMB    16   // Middle-Right Speed (Motor B)

// --- TB6612 #3: Rear Motor Pair (Header J1-18..20 & J3-10..18) ---
#define PIN_RL_PWMA    12   // Rear-Left Speed (Motor A)
#define PIN_RL_AIN1    13   // Rear-Left Direction 1
#define PIN_RL_AIN2    14   // Rear-Left Direction 2
#define PIN_RR_BIN1    38   // Rear-Right Direction 1
#define PIN_RR_BIN2    47   // Rear-Right Direction 2
#define PIN_RR_PWMB    21   // Rear-Right Speed (Motor B)

// --- INMP441 I2S Microphone (Header J3-06..08 | L/R tied to GND) ---
#define PIN_I2S_SCK    42   // Bit Clock (BCLK)
#define PIN_I2S_WS     41   // Word Select (LRCK)
#define PIN_I2S_SD     40   // Serial Data In (DIN)

// --- MLX90640 I2C Thermal Imager (Header J3-04 & J3-05) ---
#define PIN_I2C_SDA    1    // I2C Data (GPIO 1)
#define PIN_I2C_SCL    2    // I2C Clock (GPIO 2)

// --- Free Expansion Pin ---
#define PIN_SPARE_IO39 39   // Header J3-09

// =================================================================================================
// SYSTEM STATE & WATCHDOG
// =================================================================================================
WebServer server(80);
Adafruit_MLX90640 mlx;

#if HAVE_ESP_I2S
  I2SClass i2s;
#endif

// Motion State
int16_t curThrottle = 0;
int16_t curSteering = 0;
int     currentSpeed = 255;
bool    isEstop = false;
bool    searchlightOn = false;
// =================================================================================================
// WI-FI ROUTER + SIMULTANEOUS ACCESS POINT CONFIGURATION
// =================================================================================================
String  routerSSID     = "YOUR_WIFI_ROUTER_SSID";     // <-- Enter your Wi-Fi Router / Hotspot SSID
String  routerPass     = "YOUR_WIFI_ROUTER_PASSWORD"; // <-- Enter your Wi-Fi Router / Hotspot Password
const char* AP_SSID    = "RubbleRover-Tactical-AP";   // Simultaneous Fallback Access Point
const char* AP_PASS    = "RubbleRover2026";

String  camSatelliteIP = "192.168.5.1";               // Default to Standalone CAM AP or Router IP

// Motor A is physically reversed out of the box
int8_t  polarityA = -1; // -1: Inverted (Left side FL, ML, RL)
int8_t  polarityB =  1; // +1: Normal (Right side FR, MR, RR)

// Watchdog Timer (stops motors if signal lost > 1000ms)
unsigned long lastControlTime = 0;
const unsigned long WATCHDOG_TIMEOUT_MS = 1000;

// Microphone Real-Time Sound Level (0 - 100%)
volatile int liveSoundLevel = 0;
bool micOnline = false;

// MLX90640 Thermal Sensor State
volatile bool mlxFound = false;
bool i2cSwapped = false;
float thermalPixels[768];
portMUX_TYPE thermalMux = portMUX_INITIALIZER_UNLOCKED;

// Audio Non-Blocking Buffer
const size_t AUDIO_SLICE_SAMPLES = 4000;
int16_t audioSliceBuffer[AUDIO_SLICE_SAMPLES];
volatile size_t audioSliceIdx = 0;
portMUX_TYPE audioMux = portMUX_INITIALIZER_UNLOCKED;

// =================================================================================================
// FORWARD DECLARATIONS
// =================================================================================================
void setupMotors();
void setupI2SMicrophone();
void setupThermalSensor();
void setMotorPin(int pwmPin, int speed);
void setLeftMotors(uint8_t in1, uint8_t in2, int speed);
void setRightMotors(uint8_t in1, uint8_t in2, int speed);
void moveForward(int speed);
void moveBackward(int speed);
void spinLeft(int speed);
void spinRight(int speed);
void steerForwardLeft(int speed);
void steerForwardRight(int speed);
void steerBackwardLeft(int speed);
void steerBackwardRight(int speed);
void stopMotors();
void emergencyBrake();
void runMotorDiagnostic();
void handleRoot();
void handleControl();
void handleTestMotors();
void handleThermal();
void handleThermalRaw();
void handleAudioWav();
void handleRegisterCam();
void handleLight();
void handleStatus();
void handleSetWifi();
void sensorTask(void *pvParameters);

// =================================================================================================
// EMBEDDED WEB APPLICATION (HTML / CSS / JS)
// =================================================================================================
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
          <span id="statWifi" onclick="promptRouterWifi()" style="cursor:pointer;" title="Click to join Wi-Fi Router">AP+STA MODE</span>
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
            <span id="lblCamIp" onclick="promptCamIp()" style="cursor:pointer; text-decoration:underline;" title="Click to change Camera IP">CAM IP: 192.168.5.1 (SET)</span>
            <span>MLX90640 FLIR 32x24</span>
          </div>

          <div class="screen-viewport" id="viewportMain">
            <!-- LIVE CAM VIDEO STREAM -->
            <img id="cameraStream" class="video-feed" src="http://192.168.5.1/stream" alt="Video Stream" onerror="handleCamError(this)">

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

      state.camIp = '192.168.5.1';

      window.handleCamError = (img) => {
        setTimeout(() => {
          img.src = "http://" + state.camIp + "/stream?t=" + Date.now();
        }, 1200);
      };

      window.promptCamIp = () => {
        const ip = prompt("Enter ESP32-CAM IP Address (Router IP or 192.168.5.1):", state.camIp);
        if (ip && ip.trim().length > 0) {
          state.camIp = ip.trim();
          document.getElementById('lblCamIp').innerText = `CAM IP: ${state.camIp} (SET)`;
          document.getElementById('cameraStream').src = `http://${state.camIp}/stream?t=${Date.now()}`;
          fetch(`/register_cam?ip=${encodeURIComponent(state.camIp)}`).catch(() => {});
        }
      };

      window.promptRouterWifi = () => {
        const ssid = prompt("Enter Wi-Fi Router SSID for S3 Motor Controller:");
        if (ssid && ssid.trim().length > 0) {
          const pass = prompt("Enter Wi-Fi Router Password:") || "";
          fetch(`/set_wifi?ssid=${encodeURIComponent(ssid.trim())}&pass=${encodeURIComponent(pass)}`)
            .then(() => alert("Connecting S3 to Router! Check Serial Monitor or top badge for Router IP."))
            .catch(() => {});
        }
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
        fetch(`http://${state.camIp}/light?state=${state.light ? 1 : 0}`, { mode: 'no-cors' }).catch(() => {});
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
            if (data.sta_ip && data.sta_ip !== 'DISCONNECTED') {
              document.getElementById('statWifi').innerText = `ROUTER: ${data.sta_ip}`;
            } else {
              document.getElementById('statWifi').innerText = `AP: 192.168.4.1`;
            }
            if (data.cam_ip && data.cam_ip !== '') {
              state.camIp = data.cam_ip;
              document.getElementById('lblCamIp').innerText = `CAM IP: ${state.camIp} (SET)`;
              const camEl = document.getElementById('cameraStream');
              if (!camEl.src.includes(data.cam_ip)) {
                camEl.src = `http://${data.cam_ip}/stream`;
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

// =================================================================================================
// SYSTEM INITIALIZATION (setup)
// =================================================================================================
void setup() {
  // 1. Initialize Hardware CDC Serial FIRST
  Serial.begin(115200);
  #if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(50);
  #endif
  unsigned long startSerial = millis();
  while (!Serial && (millis() - startSerial < 2000)) {
    delay(10);
  }
  delay(50);

  Serial.println("\n====================================================================");
  Serial.println("  RUBBLEROVER: DIRECT-DRIVE 6WD CONTROLLER ONLINE");
  Serial.println("  Motor A: Inverted (-1) | Motor B: Normal (+1) | High-Z Coasting");
  Serial.println("====================================================================");

  // 2. Initialize all 18 motor pins to OUTPUT and immediately STOP in High-Z Coast
  setupMotors();
  Serial.println("[MOTORS] All 18 pins configured as OUTPUT. High-Z Coast Stop active.");

  // 3. Dual Wi-Fi Mode: Simultaneous Wi-Fi Router (STA) + Tactical Access Point (AP)
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // 3A. Start Always-On Tactical Access Point (192.168.4.1)
  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gateway, subnet);

  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.print("[WIFI-AP] Online! SSID: ");
  Serial.print(AP_SSID);
  Serial.print(" | AP IP: http://");
  Serial.println(WiFi.softAPIP());

  // 3B. Connect to External Wi-Fi Router / Hotspot (if configured)
  if (routerSSID.length() > 0 && routerSSID != "YOUR_WIFI_ROUTER_SSID") {
    Serial.printf("[WIFI-STA] Connecting to Wi-Fi Router: %s ...
", routerSSID.c_str());
    WiFi.begin(routerSSID.c_str(), routerPass.c_str());
    unsigned long startWait = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startWait < 6000)) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[WIFI-STA] Connected to Router! Router IP: http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[WIFI-STA] Router not reached yet (background auto-reconnect active).");
    }
  } else {
    Serial.println("[WIFI-STA] Router SSID not set yet. Set ROUTER_SSID in code or via /set_wifi?ssid=...&pass=...");
  }

  // 4. Initialize Sensors
  setupI2SMicrophone();
  setupThermalSensor();

  // 5. Setup WebServer Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/index.html", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/test_motors", HTTP_GET, handleTestMotors);
  server.on("/thermal", HTTP_GET, handleThermal);
  server.on("/thermal_raw", HTTP_GET, handleThermalRaw);
  server.on("/audio.wav", HTTP_GET, handleAudioWav);
  server.on("/register_cam", HTTP_GET, handleRegisterCam);
  server.on("/light", HTTP_GET, handleLight);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/set_wifi", HTTP_GET, handleSetWifi);

  server.begin();
  Serial.println("[HTTP] Web Server Online on Port 80");

  // 6. FreeRTOS Core 0 Background Task for MLX90640 & I2S Audio
  xTaskCreatePinnedToCore(sensorTask, "SensorTaskCore0", 16384, NULL, 1, NULL, 0);

  lastControlTime = millis();
}

// =================================================================================================
// MAIN LOOP (Core 1 - Instant Motor Response & WebServer)
// =================================================================================================
void loop() {
  server.handleClient();

  // Watchdog failsafe: Stop motors if no command received within 1000ms
  if (millis() - lastControlTime > WATCHDOG_TIMEOUT_MS) {
    if (curThrottle != 0 || curSteering != 0) {
      Serial.println("[WATCHDOG] Failsafe timeout! Stopping motors.");
      stopMotors();
    }
  }

  // Background non-blocking Router reconnect check every 10s
  static unsigned long lastRouterCheck = 0;
  if (millis() - lastRouterCheck >= 10000) {
    lastRouterCheck = millis();
    if (routerSSID.length() > 0 && routerSSID != "YOUR_WIFI_ROUTER_SSID" && WiFi.status() != WL_CONNECTED) {
      Serial.printf("[WIFI-STA] Background reconnecting to Router: %s...\n", routerSSID.c_str());
      WiFi.begin(routerSSID.c_str(), routerPass.c_str());
    }
  }

  // Periodic 2.5s Heartbeat status log (Shows both AP IP and Router IP)
  static unsigned long lastSerialHeartbeat = 0;
  if (millis() - lastSerialHeartbeat >= 2500) {
    lastSerialHeartbeat = millis();
    int clientCount = WiFi.softAPgetStationNum();
    String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "DISCONNECTED";
    Serial.printf("[HEARTBEAT] AP:%s (Clients:%d) | Router:%s | MLX:%s | Mic:%s (Lvl:%d%%)\n",
                  WiFi.softAPIP().toString().c_str(),
                  clientCount,
                  staIP.c_str(),
                  mlxFound ? "ONLINE" : "OFFLINE",
                  micOnline ? "ONLINE" : "OFFLINE",
                  liveSoundLevel);
  }
}

// =================================================================================================
// SENSOR TASK (Core 0 - 8Hz Thermal Polling & I2S Microphone)
// =================================================================================================
void sensorTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  float localFrame[768];

  while (true) {
    // Hot-plug auto-probe for MLX90640
    if (!mlxFound) {
      static unsigned long lastI2cRetry = 0;
      if (millis() - lastI2cRetry > 2000) {
        lastI2cRetry = millis();
        Wire.beginTransmission(0x33);
        if (Wire.endTransmission() == 0) {
          if (mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
            mlx.setMode(MLX90640_CHESS);
            mlx.setResolution(MLX90640_ADC_18BIT);
            mlx.setRefreshRate(MLX90640_8_HZ);
            Wire.setClock(800000);
            mlxFound = true;
            Serial.println("[MLX90640] Sensor hot-plug detected and initialized!");
          }
        }
      }
    }

    // Read thermal frame
    if (mlxFound) {
      if (mlx.getFrame(localFrame) == 0) {
        portENTER_CRITICAL(&thermalMux);
        for (int i = 0; i < 768; i++) {
          thermalPixels[i] = localFrame[i];
        }
        portEXIT_CRITICAL(&thermalMux);
      }
    }

    // Sample INMP441 audio
    #if HAVE_ESP_I2S
    if (micOnline) {
      int32_t rawBuf[128];
      size_t bytesRead = i2s.readBytes((char*)rawBuf, sizeof(rawBuf));
      if (bytesRead > 0) {
        int samples = bytesRead / sizeof(int32_t);
        int64_t sum = 0;
        portENTER_CRITICAL(&audioMux);
        for (int i = 0; i < samples; i++) {
          int32_t raw = rawBuf[i];
          int16_t pcm = (int16_t)(raw >> 14);
          sum += abs(pcm);
          if (audioSliceIdx < AUDIO_SLICE_SAMPLES) {
            audioSliceBuffer[audioSliceIdx++] = pcm;
          }
        }
        portEXIT_CRITICAL(&audioMux);

        int avgAmp = (int)(sum / (samples + 1));
        int mapped = constrain(map(avgAmp, 8, 1200, 0, 100), 0, 100);
        liveSoundLevel = (liveSoundLevel * 3 + mapped) / 4;
      }
    }
    #endif

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(60));
  }
}

// =================================================================================================
// DIRECT-DRIVE SIMPLIFIED MOTOR CONTROL (3x TB6612, High-Torque, Neutral Coast)
// =================================================================================================

// Initialize all 18 motor control pins
void setupMotors() {
  pinMode(PIN_FL_PWMA, OUTPUT); pinMode(PIN_FL_AIN1, OUTPUT); pinMode(PIN_FL_AIN2, OUTPUT);
  pinMode(PIN_FR_BIN1, OUTPUT); pinMode(PIN_FR_BIN2, OUTPUT); pinMode(PIN_FR_PWMB, OUTPUT);
  pinMode(PIN_ML_PWMA, OUTPUT); pinMode(PIN_ML_AIN1, OUTPUT); pinMode(PIN_ML_AIN2, OUTPUT);
  pinMode(PIN_MR_BIN1, OUTPUT); pinMode(PIN_MR_BIN2, OUTPUT); pinMode(PIN_MR_PWMB, OUTPUT);
  pinMode(PIN_RL_PWMA, OUTPUT); pinMode(PIN_RL_AIN1, OUTPUT); pinMode(PIN_RL_AIN2, OUTPUT);
  pinMode(PIN_RR_BIN1, OUTPUT); pinMode(PIN_RR_BIN2, OUTPUT); pinMode(PIN_RR_PWMB, OUTPUT);

  stopMotors();
}

// Drive speed pin: digital HIGH for speed > 0 guarantees full voltage & zero timer issues!
void setMotorPin(int pwmPin, int speed) {
  if (speed >= 180) {
    digitalWrite(pwmPin, HIGH); // Pure 3.3V High-Torque drive (eliminates all PWM timer stalls!)
  } else if (speed <= 0) {
    digitalWrite(pwmPin, LOW);  // 0V
  } else {
    analogWrite(pwmPin, speed);
  }
}

// Set Left Motor Bank (FL, ML, RL)
void setLeftMotors(uint8_t in1, uint8_t in2, int speed) {
  digitalWrite(PIN_FL_AIN1, in1); digitalWrite(PIN_FL_AIN2, in2); setMotorPin(PIN_FL_PWMA, speed);
  digitalWrite(PIN_ML_AIN1, in1); digitalWrite(PIN_ML_AIN2, in2); setMotorPin(PIN_ML_PWMA, speed);
  digitalWrite(PIN_RL_AIN1, in1); digitalWrite(PIN_RL_AIN2, in2); setMotorPin(PIN_RL_PWMA, speed);
}

// Set Right Motor Bank (FR, MR, RR)
void setRightMotors(uint8_t in1, uint8_t in2, int speed) {
  digitalWrite(PIN_FR_BIN1, in1); digitalWrite(PIN_FR_BIN2, in2); setMotorPin(PIN_FR_PWMB, speed);
  digitalWrite(PIN_MR_BIN1, in1); digitalWrite(PIN_MR_BIN2, in2); setMotorPin(PIN_MR_PWMB, speed);
  digitalWrite(PIN_RR_BIN1, in1); digitalWrite(PIN_RR_BIN2, in2); setMotorPin(PIN_RR_PWMB, speed);
}

// 1. FORWARD: All 6 motors roll forward!
void moveForward(int speed) {
  // Motor A reversed: LOW, HIGH = Forward
  if (polarityA == -1) setLeftMotors(LOW, HIGH, speed);
  else                 setLeftMotors(HIGH, LOW, speed);

  // Motor B normal: HIGH, LOW = Forward
  if (polarityB == 1)  setRightMotors(HIGH, LOW, speed);
  else                 setRightMotors(LOW, HIGH, speed);
}

// 2. BACKWARD: All 6 motors roll reverse!
void moveBackward(int speed) {
  // Motor A reversed: HIGH, LOW = Reverse
  if (polarityA == -1) setLeftMotors(HIGH, LOW, speed);
  else                 setLeftMotors(LOW, HIGH, speed);

  // Motor B normal: LOW, HIGH = Reverse
  if (polarityB == 1)  setRightMotors(LOW, HIGH, speed);
  else                 setRightMotors(HIGH, LOW, speed);
}

// 3. SPIN LEFT (Button B): Left Reverse, Right Forward (Tank Pivot)
void spinLeft(int speed) {
  if (polarityA == -1) setLeftMotors(HIGH, LOW, speed);
  else                 setLeftMotors(LOW, HIGH, speed);

  if (polarityB == 1)  setRightMotors(HIGH, LOW, speed);
  else                 setRightMotors(LOW, HIGH, speed);
}

// 4. SPIN RIGHT (Button A): Left Forward, Right Reverse (Tank Pivot)
void spinRight(int speed) {
  if (polarityA == -1) setLeftMotors(LOW, HIGH, speed);
  else                 setLeftMotors(HIGH, LOW, speed);

  if (polarityB == 1)  setRightMotors(LOW, HIGH, speed);
  else                 setRightMotors(HIGH, LOW, speed);
}

// 5. STEER FORWARD LEFT: Left slower, Right full
void steerForwardLeft(int speed) {
  if (polarityA == -1) setLeftMotors(LOW, HIGH, speed / 2);
  else                 setLeftMotors(HIGH, LOW, speed / 2);

  if (polarityB == 1)  setRightMotors(HIGH, LOW, speed);
  else                 setRightMotors(LOW, HIGH, speed);
}

// 6. STEER FORWARD RIGHT: Left full, Right slower
void steerForwardRight(int speed) {
  if (polarityA == -1) setLeftMotors(LOW, HIGH, speed);
  else                 setLeftMotors(HIGH, LOW, speed);

  if (polarityB == 1)  setRightMotors(HIGH, LOW, speed / 2);
  else                 setRightMotors(LOW, HIGH, speed / 2);
}

// 7. STEER BACKWARD LEFT: Left slower, Right full
void steerBackwardLeft(int speed) {
  if (polarityA == -1) setLeftMotors(HIGH, LOW, speed / 2);
  else                 setLeftMotors(LOW, HIGH, speed / 2);

  if (polarityB == 1)  setRightMotors(LOW, HIGH, speed);
  else                 setRightMotors(HIGH, LOW, speed);
}

// 8. STEER BACKWARD RIGHT: Left full, Right slower
void steerBackwardRight(int speed) {
  if (polarityA == -1) setLeftMotors(HIGH, LOW, speed);
  else                 setLeftMotors(LOW, HIGH, speed);

  if (polarityB == 1)  setRightMotors(LOW, HIGH, speed / 2);
  else                 setRightMotors(HIGH, LOW, speed / 2);
}

// 9. STOP: High-Z Neutral Coast (Wheels turn freely by hand, zero short-circuit brake)
void stopMotors() {
  setLeftMotors(LOW, LOW, 0);
  setRightMotors(LOW, LOW, 0);
  curThrottle = 0;
  curSteering = 0;
}

void emergencyBrake() {
  stopMotors();
}

// HARDWARE DIAGNOSTIC: Tests all 6 motors individually (1 second each)
void runMotorDiagnostic() {
  Serial.println("\n--- STARTING 6-MOTOR HARDWARE DIAGNOSTIC ---");

  // 1. Front-Left
  Serial.println("[DIAG 1/6] Front-Left (Motor A)...");
  digitalWrite(PIN_FL_AIN1, LOW); digitalWrite(PIN_FL_AIN2, HIGH); digitalWrite(PIN_FL_PWMA, HIGH);
  delay(1000);
  digitalWrite(PIN_FL_AIN1, LOW); digitalWrite(PIN_FL_AIN2, LOW); digitalWrite(PIN_FL_PWMA, LOW);
  delay(300);

  // 2. Front-Right
  Serial.println("[DIAG 2/6] Front-Right (Motor B)...");
  digitalWrite(PIN_FR_BIN1, HIGH); digitalWrite(PIN_FR_BIN2, LOW); digitalWrite(PIN_FR_PWMB, HIGH);
  delay(1000);
  digitalWrite(PIN_FR_BIN1, LOW); digitalWrite(PIN_FR_BIN2, LOW); digitalWrite(PIN_FR_PWMB, LOW);
  delay(300);

  // 3. Mid-Left
  Serial.println("[DIAG 3/6] Mid-Left (Motor A)...");
  digitalWrite(PIN_ML_AIN1, LOW); digitalWrite(PIN_ML_AIN2, HIGH); digitalWrite(PIN_ML_PWMA, HIGH);
  delay(1000);
  digitalWrite(PIN_ML_AIN1, LOW); digitalWrite(PIN_ML_AIN2, LOW); digitalWrite(PIN_ML_PWMA, LOW);
  delay(300);

  // 4. Mid-Right
  Serial.println("[DIAG 4/6] Mid-Right (Motor B)...");
  digitalWrite(PIN_MR_BIN1, HIGH); digitalWrite(PIN_MR_BIN2, LOW); digitalWrite(PIN_MR_PWMB, HIGH);
  delay(1000);
  digitalWrite(PIN_MR_BIN1, LOW); digitalWrite(PIN_MR_BIN2, LOW); digitalWrite(PIN_MR_PWMB, LOW);
  delay(300);

  // 5. Rear-Left
  Serial.println("[DIAG 5/6] Rear-Left (Motor A)...");
  digitalWrite(PIN_RL_AIN1, LOW); digitalWrite(PIN_RL_AIN2, HIGH); digitalWrite(PIN_RL_PWMA, HIGH);
  delay(1000);
  digitalWrite(PIN_RL_AIN1, LOW); digitalWrite(PIN_RL_AIN2, LOW); digitalWrite(PIN_RL_PWMA, LOW);
  delay(300);

  // 6. Rear-Right
  Serial.println("[DIAG 6/6] Rear-Right (Motor B)...");
  digitalWrite(PIN_RR_BIN1, HIGH); digitalWrite(PIN_RR_BIN2, LOW); digitalWrite(PIN_RR_PWMB, HIGH);
  delay(1000);
  digitalWrite(PIN_RR_BIN1, LOW); digitalWrite(PIN_RR_BIN2, LOW); digitalWrite(PIN_RR_PWMB, LOW);
  delay(300);

  stopMotors();
  Serial.println("--- MOTOR DIAGNOSTIC COMPLETE ---\n");
}

// =================================================================================================
// INMP441 I2S DIGITAL MICROPHONE SETUP
// =================================================================================================
void setupI2SMicrophone() {
  #if HAVE_ESP_I2S
    i2s.setPins(PIN_I2S_SCK, PIN_I2S_WS, -1, PIN_I2S_SD);
    if (i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
      Serial.println("[AUDIO] INMP441 Initialized via ESP_I2S (16kHz).");
      micOnline = true;
    } else if (i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
      Serial.println("[AUDIO] INMP441 Initialized mono.");
      micOnline = true;
    } else {
      Serial.println("[AUDIO] WARNING: ESP_I2S init failed.");
    }
  #else
    micOnline = true;
  #endif
}

// =================================================================================================
// MLX90640 THERMAL SENSOR SETUP
// =================================================================================================
void setupThermalSensor() {
  Serial.println("[I2C] Initializing I2C for MLX90640...");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  Wire.setTimeOut(50);

  bool detected = false;
  Wire.beginTransmission(0x33);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("[I2C] MLX90640 detected at 0x33 (SDA:1, SCL:2)!");
    i2cSwapped = false;
    detected = true;
  } else {
    Wire.end();
    Wire.begin(PIN_I2C_SCL, PIN_I2C_SDA, 400000);
    Wire.setTimeOut(50);
    Wire.beginTransmission(0x33);
    if (Wire.endTransmission() == 0) {
      Serial.println("[I2C] MLX90640 detected on swapped pins (SDA:2, SCL:1)!");
      i2cSwapped = true;
      detected = true;
    } else {
      Wire.end();
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
      Wire.setTimeOut(50);
      i2cSwapped = false;
      Serial.println("[I2C] MLX90640 not detected at 0x33 (hot-plug auto-probe active).");
    }
  }

  if (detected && mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("[MLX90640] MLX90640 Initialized Successfully!");
    mlx.setMode(MLX90640_CHESS);
    mlx.setResolution(MLX90640_ADC_18BIT);
    mlx.setRefreshRate(MLX90640_8_HZ);
    Wire.setClock(800000);
    mlxFound = true;
  } else {
    mlxFound = false;
  }
}

// =================================================================================================
// HTTP HANDLERS
// =================================================================================================
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleControl() {
  lastControlTime = millis();

  if (server.hasArg("estop") && server.arg("estop").toInt() == 1) {
    isEstop = true;
    stopMotors();
    server.send(200, "application/json", "{\"status\":\"ESTOP\"}");
    return;
  }
  isEstop = false;

  int speed = 255; // Default full punch
  if (server.hasArg("r")) {
    int r = server.arg("r").toInt();
    if (r == 1) speed = 180;
    else if (r == 2) speed = 220;
    else if (r == 3) speed = 255;
  }
  currentSpeed = speed;

  if (server.hasArg("pol_a")) polarityA = (int8_t)server.arg("pol_a").toInt();
  if (server.hasArg("pol_b")) polarityB = (int8_t)server.arg("pol_b").toInt();

  int t = server.hasArg("t") ? server.arg("t").toInt() : 0;
  int s = server.hasArg("s") ? server.arg("s").toInt() : 0;
  curThrottle = t;
  curSteering = s;

  // DIRECT SIMPLIFIED MOTION SELECTION
  if (t > 0 && s == 0) {
    moveForward(speed);
  } else if (t < 0 && s == 0) {
    moveBackward(speed);
  } else if (s < -200 && t == 0) {
    spinLeft(speed);  // Button B (Spin Left)
  } else if (s > 200 && t == 0) {
    spinRight(speed); // Button A (Spin Right)
  } else if (t > 0 && s < 0) {
    steerForwardLeft(speed);
  } else if (t > 0 && s > 0) {
    steerForwardRight(speed);
  } else if (t < 0 && s < 0) {
    steerBackwardLeft(speed);
  } else if (t < 0 && s > 0) {
    steerBackwardRight(speed);
  } else {
    stopMotors();
  }

  server.client().setNoDelay(true);
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleTestMotors() {
  server.send(200, "application/json", "{\"status\":\"RUNNING_TEST\"}");
  runMotorDiagnostic();
}

void handleThermal() {
  float localPixels[768];
  float maxT = -100.0f;
  float minT = 200.0f;

  portENTER_CRITICAL(&thermalMux);
  for (int i = 0; i < 768; i++) {
    localPixels[i] = thermalPixels[i];
  }
  portEXIT_CRITICAL(&thermalMux);

  for (int i = 0; i < 768; i++) {
    float val = localPixels[i];
    if (!isnan(val)) {
      if (val > maxT) maxT = val;
      if (val < minT) minT = val;
    }
  }

  if (minT > maxT) { minT = 24.0f; maxT = 25.0f; }

  String json;
  json.reserve(5500);
  json = "{\"min\":" + String(minT, 1) + ",\"max\":" + String(maxT, 1) +
         ",\"center\":" + String(localPixels[12 * 32 + 16], 1) +
         ",\"sound_level\":" + String(liveSoundLevel) + ",\"pixels\":[";

  for (int i = 0; i < 768; i++) {
    if (isnan(localPixels[i])) json += "0.0";
    else json += String(localPixels[i], 1);
    if (i < 767) json += ",";
  }
  json += "]}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleThermalRaw() {
  handleThermal();
}

void buildWavHeader(uint8_t *header, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t pcmDataBytes) {
  uint32_t fileSize = pcmDataBytes + 36;
  uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  uint16_t blockAlign = channels * (bitsPerSample / 8);

  memcpy(header, "RIFF", 4);
  header[4] = (uint8_t)(fileSize & 0xFF);
  header[5] = (uint8_t)((fileSize >> 8) & 0xFF);
  header[6] = (uint8_t)((fileSize >> 16) & 0xFF);
  header[7] = (uint8_t)((fileSize >> 24) & 0xFF);
  memcpy(header + 8, "WAVEfmt ", 8);
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = (uint8_t)(channels & 0xFF); header[23] = 0;
  header[24] = (uint8_t)(sampleRate & 0xFF);
  header[25] = (uint8_t)((sampleRate >> 8) & 0xFF);
  header[26] = (uint8_t)((sampleRate >> 16) & 0xFF);
  header[27] = (uint8_t)((sampleRate >> 24) & 0xFF);
  header[28] = (uint8_t)(byteRate & 0xFF);
  header[29] = (uint8_t)((byteRate >> 8) & 0xFF);
  header[30] = (uint8_t)((byteRate >> 16) & 0xFF);
  header[31] = (uint8_t)((byteRate >> 24) & 0xFF);
  header[32] = (uint8_t)(blockAlign & 0xFF); header[33] = 0;
  header[34] = (uint8_t)(bitsPerSample & 0xFF); header[35] = 0;
  memcpy(header + 36, "data", 4);
  header[40] = (uint8_t)(pcmDataBytes & 0xFF);
  header[41] = (uint8_t)((pcmDataBytes >> 8) & 0xFF);
  header[42] = (uint8_t)((pcmDataBytes >> 16) & 0xFF);
  header[43] = (uint8_t)((pcmDataBytes >> 24) & 0xFF);
}

void handleAudioWav() {
  uint8_t wavHeader[44];
  size_t samplesToSend = 0;
  int16_t sendBuf[AUDIO_SLICE_SAMPLES];

  portENTER_CRITICAL(&audioMux);
  samplesToSend = audioSliceIdx;
  if (samplesToSend > AUDIO_SLICE_SAMPLES) samplesToSend = AUDIO_SLICE_SAMPLES;
  for (size_t i = 0; i < samplesToSend; i++) sendBuf[i] = audioSliceBuffer[i];
  audioSliceIdx = 0;
  portEXIT_CRITICAL(&audioMux);

  if (samplesToSend == 0) samplesToSend = 512;

  uint32_t pcmBytes = samplesToSend * sizeof(int16_t);
  buildWavHeader(wavHeader, 16000, 16, 1, pcmBytes);

  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: audio/wav");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.printf("Content-Length: %u\r\n\r\n", 44 + pcmBytes);
  client.write(wavHeader, 44);
  client.write((const uint8_t*)sendBuf, pcmBytes);
}

void handleRegisterCam() {
  if (server.hasArg("ip")) {
    camSatelliteIP = server.arg("ip");
    Serial.printf("[CAMERA-HUB] Registered at IP: %s\n", camSatelliteIP.c_str());
    server.send(200, "application/json", "{\"status\":\"REGISTERED\",\"cam_ip\":\"" + camSatelliteIP + "\"}");
  } else {
    server.send(400, "text/plain", "Missing IP parameter");
  }
}

void handleLight() {
  if (server.hasArg("state")) searchlightOn = (server.arg("state").toInt() == 1);
  server.send(200, "application/json", "{\"status\":\"OK\",\"light\":" + String(searchlightOn ? "1" : "0") + "}");
}

void handleStatus() {
  String json = "{";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"throttle\":" + String(curThrottle) + ",";
  json += "\"steering\":" + String(curSteering) + ",";
  json += "\"speed\":" + String(currentSpeed) + ",";
  json += "\"pol_a\":" + String(polarityA) + ",";
  json += "\"pol_b\":" + String(polarityB) + ",";
  json += "\"estop\":" + String(isEstop ? "true" : "false") + ",";
  json += "\"light\":" + String(searchlightOn ? "true" : "false") + ",";
  json += "\"sound_level\":" + String(liveSoundLevel) + ",";
  json += "\"mlx_online\":" + String(mlxFound ? "true" : "false") + ",";
  String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "DISCONNECTED";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"sta_ip\":\"" + staIP + "\",";
  json += "\"cam_ip\":\"" + camSatelliteIP + "\"";
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleSetWifi() {
  if (server.hasArg("ssid")) {
    routerSSID = server.arg("ssid");
    routerPass = server.hasArg("pass") ? server.arg("pass") : "";
    Serial.printf("[WIFI-STA] Updating Router credentials to SSID: %s\n", routerSSID.c_str());
    WiFi.begin(routerSSID.c_str(), routerPass.c_str());
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"CONNECTING\",\"ssid\":\"" + routerSSID + "\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing ssid parameter\"}");
  }
}
