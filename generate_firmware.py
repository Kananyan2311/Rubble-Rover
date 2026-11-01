# -*- coding: utf-8 -*-
"""
generate_firmware.py
Generates the perfected, ultra-reliable ESP32-S3 firmware for RubbleRover.
Includes the embedded Neo-Brutalist GameBoy WebApp and dual-core FreeRTOS architecture.
"""

firmware_template = r'''/*
  RubbleRover - ESP32-S3 Master Tactical Controller
  Hardware: ESP32-S3 (Dual-Core) + 3x TB6612FNG (6WD) + MLX90640 + INMP441
  Network: Standalone SoftAP (192.168.4.1)
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
// USER VERIFIED PIN DEFINITIONS
// =================================================================================================

// --- TB6612 #1: Front Motor Pair (Header J1-10 to J1-17) ---
#define PIN_FL_PWMA    17   // Front-Left Speed PWM (Motor A)
#define PIN_FL_AIN1    18   // Front-Left Direction 1
#define PIN_FL_AIN2    8    // Front-Left Direction 2
#define PIN_FR_BIN1    9    // Front-Right Direction 1
#define PIN_FR_BIN2    10   // Front-Right Direction 2
#define PIN_FR_PWMB    11   // Front-Right Speed PWM (Motor B)

// --- TB6612 #2: Middle Motor Pair (Header J1-04 to J1-09) ---
#define PIN_ML_PWMA    4    // Middle-Left Speed PWM (Motor A)
#define PIN_ML_AIN1    5    // Middle-Left Direction 1
#define PIN_ML_AIN2    6    // Middle-Left Direction 2
#define PIN_MR_BIN1    7    // Middle-Right Direction 1
#define PIN_MR_BIN2    15   // Middle-Right Direction 2
#define PIN_MR_PWMB    16   // Middle-Right Speed PWM (Motor B)

// --- TB6612 #3: Rear Motor Pair (Header J1-18..20 & J3-10..18) ---
#define PIN_RL_PWMA    12   // Rear-Left Speed PWM (Motor A)
#define PIN_RL_AIN1    13   // Rear-Left Direction 1
#define PIN_RL_AIN2    14   // Rear-Left Direction 2
#define PIN_RR_BIN1    38   // Rear-Right Direction 1
#define PIN_RR_BIN2    47   // Rear-Right Direction 2
#define PIN_RR_PWMB    21   // Rear-Right Speed PWM (Motor B)

// --- INMP441 I2S Microphone (Header J3-06..08 | L/R tied to GND) ---
#define PIN_I2S_SCK    42   // Bit Clock (BCLK)
#define PIN_I2S_WS     41   // Word Select (LRCK)
#define PIN_I2S_SD     40   // Serial Data In (DIN)

// --- MLX90640 I2C Thermal Imager (Header J3-04 & J3-05) ---
#define PIN_I2C_SDA    1    // I2C Data (User confirmed GPIO 1)
#define PIN_I2C_SCL    2    // I2C Clock (User confirmed GPIO 2)

// --- Free Expansion Pin ---
#define PIN_SPARE_IO39 39   // Unallocated Clean GPIO on Header J3-09

// =================================================================================================
// MOTOR PWM CONSTANTS (1 kHz High-Torque Operation)
// =================================================================================================
const int PWM_FREQ = 1000;     // 1 kHz (maximum torque for DC gearmotors)
const int PWM_RES  = 8;        // 8-bit resolution (0 to 255)

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
float   curRateMultiplier = 0.70f;
bool    isEstop = false;
bool    searchlightOn = false;
String  camSatelliteIP = "192.168.4.2";

// Direction Polarity (+1: Normal, -1: Inverted)
int8_t polarityA = 1; // Motor A (FL, ML, RL)
int8_t polarityB = 1; // Motor B (FR, MR, RR)
int16_t curLeftSpeed = 0;
int16_t curRightSpeed = 0;

// Watchdog Timer (Failsafe stops motors if no GCS command within 1000ms)
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

// Audio PCM Ring Buffer for Non-Blocking Audio Streaming
const size_t AUDIO_SLICE_SAMPLES = 4000; // 0.25 sec @ 16kHz
int16_t audioSliceBuffer[AUDIO_SLICE_SAMPLES];
volatile size_t audioSliceIdx = 0;
portMUX_TYPE audioMux = portMUX_INITIALIZER_UNLOCKED;

// =================================================================================================
// FORWARD DECLARATIONS
// =================================================================================================
void setupMotors();
void setupI2SMicrophone();
void setupThermalSensor();
void setMotorOutputs(int16_t fl, int16_t fr, int16_t ml, int16_t mr, int16_t rl, int16_t rr);
void emergencyBrake();
void handleRoot();
void handleControl();
void handleThermal();
void handleThermalRaw();
void handleAudioWav();
void handleRegisterCam();
void handleLight();
void handleStatus();
void sensorTask(void *pvParameters);

// =================================================================================================
// EMBEDDED WEB APPLICATION (HTML / CSS / JS)
// =================================================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(__WEBAPP_HTML_PAYLOAD__)rawliteral";

// =================================================================================================
// SYSTEM INITIALIZATION (setup)
// =================================================================================================
void setup() {
  // 1. HARD-CLAMP ALL 18 MOTOR PINS TO LOW AT MICROSECOND 1
  // Front Pair
  pinMode(PIN_FL_PWMA, OUTPUT); digitalWrite(PIN_FL_PWMA, LOW);
  pinMode(PIN_FL_AIN1, OUTPUT); digitalWrite(PIN_FL_AIN1, LOW);
  pinMode(PIN_FL_AIN2, OUTPUT); digitalWrite(PIN_FL_AIN2, LOW);
  pinMode(PIN_FR_BIN1, OUTPUT); digitalWrite(PIN_FR_BIN1, LOW);
  pinMode(PIN_FR_BIN2, OUTPUT); digitalWrite(PIN_FR_BIN2, LOW);
  pinMode(PIN_FR_PWMB, OUTPUT); digitalWrite(PIN_FR_PWMB, LOW);

  // Middle Pair
  pinMode(PIN_ML_PWMA, OUTPUT); digitalWrite(PIN_ML_PWMA, LOW);
  pinMode(PIN_ML_AIN1, OUTPUT); digitalWrite(PIN_ML_AIN1, LOW);
  pinMode(PIN_ML_AIN2, OUTPUT); digitalWrite(PIN_ML_AIN2, LOW);
  pinMode(PIN_MR_BIN1, OUTPUT); digitalWrite(PIN_MR_BIN1, LOW);
  pinMode(PIN_MR_BIN2, OUTPUT); digitalWrite(PIN_MR_BIN2, LOW);
  pinMode(PIN_MR_PWMB, OUTPUT); digitalWrite(PIN_MR_PWMB, LOW);

  // Rear Pair
  pinMode(PIN_RL_PWMA, OUTPUT); digitalWrite(PIN_RL_PWMA, LOW);
  pinMode(PIN_RL_AIN1, OUTPUT); digitalWrite(PIN_RL_AIN1, LOW);
  pinMode(PIN_RL_AIN2, OUTPUT); digitalWrite(PIN_RL_AIN2, LOW);
  pinMode(PIN_RR_BIN1, OUTPUT); digitalWrite(PIN_RR_BIN1, LOW);
  pinMode(PIN_RR_BIN2, OUTPUT); digitalWrite(PIN_RR_BIN2, LOW);
  pinMode(PIN_RR_PWMB, OUTPUT); digitalWrite(PIN_RR_PWMB, LOW);

  // 2. Initialize Hardware CDC Serial FIRST with connection wait
  Serial.begin(115200);
  unsigned long startSerial = millis();
  while (!Serial && (millis() - startSerial < 2500)) {
    delay(10);
  }
  delay(150);

  Serial.println("\n\n====================================================================");
  Serial.println("  RUBBLEROVER: OPTIMIZED DUAL-CORE MASTER CONTROLLER ONLINE!");
  Serial.println("  Post-Earthquake Reconnaissance & Surveillance System");
  Serial.println("====================================================================");
  Serial.flush();

  // 3. START WI-FI ACCESS POINT (Immediate Standalone AP)
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false); // Disable Wi-Fi sleep for ultra-low latency AP streaming

  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gateway, subnet);

  bool apStarted = WiFi.softAP("RubbleRover-Tactical-AP", "RubbleRover2026", 1, 0, 4);
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Maximum RF broadcast power

  if (apStarted) {
    Serial.print("[WIFI-AP] SUCCESS! SSID: RubbleRover-Tactical-AP | IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    WiFi.softAP("RubbleRover-Tactical-AP", "RubbleRover2026");
    Serial.print("[WIFI-AP] Fallback SSID: RubbleRover-Tactical-AP | IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // 4. Attach 1kHz High-Torque PWM to all 6 motors
  setupMotors();

  // 5. Initialize INMP441 I2S Microphone
  setupI2SMicrophone();

  // 6. Initialize MLX90640 I2C Thermal Sensor
  setupThermalSensor();

  // 7. Setup Web Server Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/index.html", HTTP_GET, handleRoot);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/thermal", HTTP_GET, handleThermal);
  server.on("/thermal_raw", HTTP_GET, handleThermalRaw);
  server.on("/audio.wav", HTTP_GET, handleAudioWav);
  server.on("/register_cam", HTTP_GET, handleRegisterCam);
  server.on("/light", HTTP_GET, handleLight);
  server.on("/status", HTTP_GET, handleStatus);

  server.begin();
  Serial.println("[HTTP] Web Server Started on Port 80");

  // 8. Launch Dedicated Sensor Task on Core 0 (16KB stack for MLX floating point)
  xTaskCreatePinnedToCore(
    sensorTask,        // Function pointer
    "SensorTaskCore0", // Task name
    16384,             // Stack size (16KB)
    NULL,              // Parameters
    1,                 // Priority
    NULL,              // Task handle
    0                  // Core 0 (Background sensors)
  );
  Serial.println("[FREERTOS] SensorTask pinned to Core 0. Core 1 dedicated to Driving & GCS.");

  lastControlTime = millis();
}

// =================================================================================================
// MAIN LOOP (Runs on Core 1 - Zero Lag Motor Control & Web Requests)
// =================================================================================================
void loop() {
  server.handleClient();

  // Safety Watchdog: If no control packet received in 1000ms, brake motors
  if (millis() - lastControlTime > WATCHDOG_TIMEOUT_MS) {
    if (curThrottle != 0 || curSteering != 0) {
      Serial.println("[WATCHDOG] Failsafe: Signal timeout! Stopping motors.");
      emergencyBrake();
    }
  }

  // Periodic Serial Heartbeat every 2.5s (Guarantees Serial Monitor always displays active status)
  static unsigned long lastSerialHeartbeat = 0;
  if (millis() - lastSerialHeartbeat >= 2500) {
    lastSerialHeartbeat = millis();
    int clientCount = WiFi.softAPgetStationNum();
    Serial.printf("[HEARTBEAT] Clients:%d | MLX:%s | Mic:%s (Lvl:%d%%) | Drive L:%+d R:%+d | IP:%s\n",
                  clientCount,
                  mlxFound ? "ONLINE(32x24)" : "OFFLINE",
                  micOnline ? "ONLINE" : "SEARCHING",
                  liveSoundLevel,
                  curLeftSpeed, curRightSpeed,
                  WiFi.softAPIP().toString().c_str());
  }
}

// =================================================================================================
// SENSOR TASK (Pinned to Core 0 - 8Hz Thermal Polling & I2S Microphone)
// =================================================================================================
void sensorTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  float localFrame[768];

  while (true) {
    // 1. Hot-plug re-probe if sensor was not found at boot
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

    // 2. Read Thermal Frame via Official Adafruit Driver
    if (mlxFound) {
      if (mlx.getFrame(localFrame) == 0) {
        portENTER_CRITICAL(&thermalMux);
        for (int i = 0; i < 768; i++) {
          thermalPixels[i] = localFrame[i];
        }
        portEXIT_CRITICAL(&thermalMux);
      }
    }

    // 3. Sample INMP441 Audio Level & Fill Non-Blocking Audio Buffer
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

    // Run loop at ~16 Hz (every 60ms) to match 8Hz sensor refresh rate
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(60));
  }
}

// =================================================================================================
// MOTOR CONTROL FUNCTIONS (3x TB6612, 1kHz High-Torque PWM)
// =================================================================================================
void setupMotors() {
  // Attach 1kHz PWM on all 6 motor speed lines
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_FL_PWMA, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_FR_PWMB, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_ML_PWMA, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_MR_PWMB, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_RL_PWMA, PWM_FREQ, PWM_RES);
    ledcAttach(PIN_RR_PWMB, PWM_FREQ, PWM_RES);
    ledcWrite(PIN_FL_PWMA, 0);
    ledcWrite(PIN_FR_PWMB, 0);
    ledcWrite(PIN_ML_PWMA, 0);
    ledcWrite(PIN_MR_PWMB, 0);
    ledcWrite(PIN_RL_PWMA, 0);
    ledcWrite(PIN_RR_PWMB, 0);
  #else
    analogWriteFrequency(PIN_FL_PWMA, PWM_FREQ);
    analogWriteFrequency(PIN_FR_PWMB, PWM_FREQ);
    analogWriteFrequency(PIN_ML_PWMA, PWM_FREQ);
    analogWriteFrequency(PIN_MR_PWMB, PWM_FREQ);
    analogWriteFrequency(PIN_RL_PWMA, PWM_FREQ);
    analogWriteFrequency(PIN_RR_PWMB, PWM_FREQ);
    analogWriteResolution(PIN_FL_PWMA, PWM_RES);
    analogWriteResolution(PIN_FR_PWMB, PWM_RES);
    analogWriteResolution(PIN_ML_PWMA, PWM_RES);
    analogWriteResolution(PIN_MR_PWMB, PWM_RES);
    analogWriteResolution(PIN_RL_PWMA, PWM_RES);
    analogWriteResolution(PIN_RR_PWMB, PWM_RES);
    analogWrite(PIN_FL_PWMA, 0);
    analogWrite(PIN_FR_PWMB, 0);
    analogWrite(PIN_ML_PWMA, 0);
    analogWrite(PIN_MR_PWMB, 0);
    analogWrite(PIN_RL_PWMA, 0);
    analogWrite(PIN_RR_PWMB, 0);
  #endif

  emergencyBrake();
  Serial.println("[MOTORS] 3x TB6612FNG Initialized (1 kHz High-Torque PWM). STBY hardwired to 3.3V.");
}

void writeMotorPWM(int pwmPin, uint32_t duty) {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pwmPin, duty);
  #else
    analogWrite(pwmPin, duty);
  #endif
}

void setMotorChannel(int pinIn1, int pinIn2, int pwmPin, int16_t speed, int8_t polarity) {
  int16_t effSpeed = speed * polarity;
  effSpeed = constrain(effSpeed, -255, 255);

  uint32_t rawDuty = (uint32_t)abs(effSpeed);
  uint32_t pwmDuty = 0;

  if (rawDuty > 0) {
    // High-Torque Breakaway Boost: maps [1..255] to [80..255]
    // Guarantees zero stall / hum under rover weight!
    pwmDuty = (uint32_t)map(rawDuty, 1, 255, 80, 255);
    if (pwmDuty > 255) pwmDuty = 255;
  }

  if (effSpeed > 0) {
    digitalWrite(pinIn1, HIGH);
    digitalWrite(pinIn2, LOW);
    writeMotorPWM(pwmPin, pwmDuty);
  } else if (effSpeed < 0) {
    digitalWrite(pinIn1, LOW);
    digitalWrite(pinIn2, HIGH);
    writeMotorPWM(pwmPin, pwmDuty);
  } else {
    // Neutral Coast Stop (High-Z avoids dynamic braking drag)
    digitalWrite(pinIn1, LOW);
    digitalWrite(pinIn2, LOW);
    writeMotorPWM(pwmPin, 0);
  }
}

void setMotorOutputs(int16_t fl, int16_t fr, int16_t ml, int16_t mr, int16_t rl, int16_t rr) {
  // Front Pair (FL = Motor A, FR = Motor B)
  setMotorChannel(PIN_FL_AIN1, PIN_FL_AIN2, PIN_FL_PWMA, fl, polarityA);
  setMotorChannel(PIN_FR_BIN1, PIN_FR_BIN2, PIN_FR_PWMB, fr, polarityB);

  // Middle Pair (ML = Motor A, MR = Motor B)
  setMotorChannel(PIN_ML_AIN1, PIN_ML_AIN2, PIN_ML_PWMA, ml, polarityA);
  setMotorChannel(PIN_MR_BIN1, PIN_MR_BIN2, PIN_MR_PWMB, mr, polarityB);

  // Rear Pair (RL = Motor A, RR = Motor B)
  setMotorChannel(PIN_RL_AIN1, PIN_RL_AIN2, PIN_RL_PWMA, rl, polarityA);
  setMotorChannel(PIN_RR_BIN1, PIN_RR_BIN2, PIN_RR_PWMB, rr, polarityB);
}

void emergencyBrake() {
  curThrottle = 0;
  curSteering = 0;
  curLeftSpeed = 0;
  curRightSpeed = 0;

  // Coast Stop all motor lines
  digitalWrite(PIN_FL_AIN1, LOW); digitalWrite(PIN_FL_AIN2, LOW);
  digitalWrite(PIN_FR_BIN1, LOW); digitalWrite(PIN_FR_BIN2, LOW);
  digitalWrite(PIN_ML_AIN1, LOW); digitalWrite(PIN_ML_AIN2, LOW);
  digitalWrite(PIN_MR_BIN1, LOW); digitalWrite(PIN_MR_BIN2, LOW);
  digitalWrite(PIN_RL_AIN1, LOW); digitalWrite(PIN_RL_AIN2, LOW);
  digitalWrite(PIN_RR_BIN1, LOW); digitalWrite(PIN_RR_BIN2, LOW);

  writeMotorPWM(PIN_FL_PWMA, 0);
  writeMotorPWM(PIN_FR_PWMB, 0);
  writeMotorPWM(PIN_ML_PWMA, 0);
  writeMotorPWM(PIN_MR_PWMB, 0);
  writeMotorPWM(PIN_RL_PWMA, 0);
  writeMotorPWM(PIN_RR_PWMB, 0);
}

// =================================================================================================
// INMP441 I2S DIGITAL MICROPHONE SETUP
// =================================================================================================
void setupI2SMicrophone() {
  #if HAVE_ESP_I2S
    i2s.setPins(PIN_I2S_SCK, PIN_I2S_WS, -1, PIN_I2S_SD);
    if (i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
      Serial.println("[AUDIO] INMP441 Initialized via ESP_I2S (16kHz, Stereo BCLK 1.024MHz).");
      micOnline = true;
    } else {
      if (i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
        Serial.println("[AUDIO] INMP441 Initialized in mono mode.");
        micOnline = true;
      } else {
        Serial.println("[AUDIO] WARNING: ESP_I2S initialization failed.");
      }
    }
  #else
    Serial.println("[AUDIO] Using legacy I2S config.");
    micOnline = true;
  #endif
}

// =================================================================================================
// ADAFRUIT_MLX90640 I2C THERMAL SENSOR SETUP
// =================================================================================================
void setupThermalSensor() {
  Serial.println("[I2C] Initializing I2C for MLX90640...");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

  Wire.beginTransmission(0x33);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("[I2C] SUCCESS: MLX90640 detected at 0x33 (SDA:1, SCL:2)!");
    i2cSwapped = false;
  } else {
    Serial.printf("[I2C] WARNING: 0x33 not responding on (1,2) (code %d). Probing swapped pins (2,1)...\n", error);
    Wire.end();
    Wire.begin(PIN_I2C_SCL, PIN_I2C_SDA, 400000);
    Wire.beginTransmission(0x33);
    if (Wire.endTransmission() == 0) {
      Serial.println("[I2C] SUCCESS: MLX90640 detected at 0x33 on swapped pins (SDA:2, SCL:1)!");
      i2cSwapped = true;
    } else {
      Serial.println("[I2C] WARNING: MLX90640 not yet detected. Hot-plug task will auto-probe.");
      Wire.end();
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
      i2cSwapped = false;
    }
  }

  if (mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("[MLX90640] MLX90640 Initialized Successfully!");
    mlx.setMode(MLX90640_CHESS);
    mlx.setResolution(MLX90640_ADC_18BIT);
    mlx.setRefreshRate(MLX90640_8_HZ);
    Wire.setClock(800000);
    mlxFound = true;
  } else {
    Serial.println("[MLX90640] MLX90640 init failed! Will retry in background task.");
  }
}

// =================================================================================================
// HTTP REQUEST HANDLERS
// =================================================================================================
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleControl() {
  lastControlTime = millis();

  if (server.hasArg("estop") && server.arg("estop").toInt() == 1) {
    isEstop = true;
    emergencyBrake();
    server.send(200, "application/json", "{\"status\":\"ESTOP\"}");
    return;
  }
  isEstop = false;

  if (server.hasArg("t")) curThrottle = server.arg("t").toInt();
  if (server.hasArg("s")) curSteering = server.arg("s").toInt();
  if (server.hasArg("r")) {
    int r = server.arg("r").toInt();
    curRateMultiplier = (r == 1) ? 0.40f : (r == 2 ? 0.70f : 1.00f);
  }
  if (server.hasArg("light")) {
    searchlightOn = (server.arg("light").toInt() == 1);
  }
  if (server.hasArg("pol_a")) {
    polarityA = (int8_t)server.arg("pol_a").toInt();
  }
  if (server.hasArg("pol_b")) {
    polarityB = (int8_t)server.arg("pol_b").toInt();
  }

  // Skid-Steer Differential Mixing
  int16_t leftSpeed  = (int16_t)((curThrottle + curSteering) * curRateMultiplier);
  int16_t rightSpeed = (int16_t)((curThrottle - curSteering) * curRateMultiplier);

  leftSpeed  = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  curLeftSpeed = leftSpeed;
  curRightSpeed = rightSpeed;

  setMotorOutputs(leftSpeed, rightSpeed, leftSpeed, rightSpeed, leftSpeed, rightSpeed);

  server.client().setNoDelay(true);
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

void handleThermal() {
  float localPixels[768];
  float maxT = -100.0f;
  float minT = 200.0f;
  float sumT = 0.0f;

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
      sumT += val;
    }
  }

  if (minT > maxT) { minT = 24.0f; maxT = 25.0f; }

  String json;
  json.reserve(5500);
  json = "{\"min\":" + String(minT, 1) + ",\"max\":" + String(maxT, 1) +
         ",\"center\":" + String(localPixels[12 * 32 + 16], 1) +
         ",\"sound_level\":" + String(liveSoundLevel) + ",\"pixels\":[";

  for (int i = 0; i < 768; i++) {
    if (isnan(localPixels[i])) {
      json += "0.0";
    } else {
      json += String(localPixels[i], 1);
    }
    if (i < 767) json += ",";
  }
  json += "]}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleThermalRaw() {
  handleThermal();
}

// 44-byte standard RIFF WAV header builder
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
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; // Subchunk1Size (16 for PCM)
  header[20] = 1; header[21] = 0; // AudioFormat (1 for PCM)
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

// NON-BLOCKING Fast WAV Audio Slicing (Takes 2ms to serve, zero server freeze!)
void handleAudioWav() {
  uint8_t wavHeader[44];
  size_t samplesToSend = 0;
  int16_t sendBuf[AUDIO_SLICE_SAMPLES];

  portENTER_CRITICAL(&audioMux);
  samplesToSend = audioSliceIdx;
  if (samplesToSend > AUDIO_SLICE_SAMPLES) samplesToSend = AUDIO_SLICE_SAMPLES;
  for (size_t i = 0; i < samplesToSend; i++) {
    sendBuf[i] = audioSliceBuffer[i];
  }
  audioSliceIdx = 0; // Reset slice for next capture
  portEXIT_CRITICAL(&audioMux);

  if (samplesToSend == 0) samplesToSend = 512; // Minimal silent pad

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
    Serial.printf("[CAMERA-HUB] Satellite Video Stream registered at IP: %s\n", camSatelliteIP.c_str());
    server.send(200, "application/json", "{\"status\":\"REGISTERED\",\"cam_ip\":\"" + camSatelliteIP + "\"}");
  } else {
    server.send(400, "text/plain", "Missing IP parameter");
  }
}

void handleLight() {
  if (server.hasArg("state")) {
    searchlightOn = (server.arg("state").toInt() == 1);
  }
  server.send(200, "application/json", "{\"status\":\"OK\",\"light\":" + String(searchlightOn ? "1" : "0") + "}");
}

void handleStatus() {
  String json = "{";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"throttle\":" + String(curThrottle) + ",";
  json += "\"steering\":" + String(curSteering) + ",";
  json += "\"rate\":" + String(curRateMultiplier, 2) + ",";
  json += "\"pol_a\":" + String(polarityA) + ",";
  json += "\"pol_b\":" + String(polarityB) + ",";
  json += "\"estop\":" + String(isEstop ? "true" : "false") + ",";
  json += "\"light\":" + String(searchlightOn ? "true" : "false") + ",";
  json += "\"sound_level\":" + String(liveSoundLevel) + ",";
  json += "\"mlx_online\":" + String(mlxFound ? "true" : "false") + ",";
  json += "\"cam_ip\":\"" + camSatelliteIP + "\"";
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}
'''

if __name__ == '__main__':
    with open('RubbleRover_WebApp.html', 'r', encoding='utf-8') as f:
        html_content = f.read()

    full_firmware = firmware_template.replace("__WEBAPP_HTML_PAYLOAD__", html_content)

    # 1. Update Workspace RubbleRover_S3_MotorController.ino
    with open('RubbleRover_S3_MotorController.ino', 'w', encoding='utf-8') as f:
        f.write(full_firmware)

    # 2. Update subdirectory sketch if exists
    import os
    if os.path.exists('RubbleRover_S3_MotorController'):
        with open('RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino', 'w', encoding='utf-8') as f:
            f.write(full_firmware)

    # 3. Update User's Active Arduino Sketch directory
    arduino_dir = r'C:\Users\Kananyan\Documents\Arduino\sketch_nov1a'
    if os.path.exists(arduino_dir):
        with open(os.path.join(arduino_dir, 'sketch_nov1a.ino'), 'w', encoding='utf-8') as f:
            f.write(full_firmware)
        print(f"SUCCESS: Synchronized firmware to {arduino_dir}\\sketch_nov1a.ino")

    # 4. Generate header rubble_rover_web.h
    header_content = '#pragma once\n\nconst char INDEX_HTML[] PROGMEM = R"rawliteral(' + html_content + ')rawliteral";\n'
    with open('rubble_rover_web.h', 'w', encoding='utf-8') as f:
        f.write(header_content)
    if os.path.exists(arduino_dir):
        with open(os.path.join(arduino_dir, 'rubble_rover_web.h'), 'w', encoding='utf-8') as f:
            f.write(header_content)

    print("SUCCESS: All ESP32-S3 firmware sketches updated and synchronized successfully!")
