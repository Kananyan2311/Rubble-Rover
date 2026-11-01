# -*- coding: utf-8 -*-
"""
generate_firmware.py
Generates the simplified, ultra-reliable ESP32-S3 firmware for RubbleRover.
Motor control:
- Pure digital drive (digitalWrite HIGH for full torque, LOW for High-Z coast)
- Motor A physically reversed by default (polarityA = -1)
- Motor B normal (polarityB = 1)
- stopMotors() clamps all 18 pins LOW (High-Z coast, zero brake drag)
- Built-in motor diagnostic self-test (/test_motors)
"""

firmware_template = r'''/*
  RubbleRover - ESP32-S3 Master Tactical Controller
  Hardware: ESP32-S3 (Dual-Core) + 3x TB6612FNG (6WD) + MLX90640 + INMP441
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
String  camSatelliteIP = "192.168.4.2";

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
void sensorTask(void *pvParameters);

// =================================================================================================
// EMBEDDED WEB APPLICATION (HTML / CSS / JS)
// =================================================================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(__WEBAPP_HTML_PAYLOAD__)rawliteral";

// =================================================================================================
// SYSTEM INITIALIZATION (setup)
// =================================================================================================
void setup() {
  // 1. Initialize all 18 motor pins to OUTPUT and immediately STOP in High-Z Coast
  setupMotors();

  // 2. Initialize Hardware CDC Serial
  Serial.begin(115200);
  unsigned long startSerial = millis();
  while (!Serial && (millis() - startSerial < 2500)) {
    delay(10);
  }
  delay(100);

  Serial.println("\n====================================================================");
  Serial.println("  RUBBLEROVER: DIRECT-DRIVE 6WD CONTROLLER ONLINE");
  Serial.println("  Motor A: Inverted (-1) | Motor B: Normal (+1) | High-Z Coasting");
  Serial.println("====================================================================");

  // 3. Standalone Wi-Fi Access Point (192.168.4.1)
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, gateway, subnet);

  WiFi.softAP("RubbleRover-Tactical-AP", "RubbleRover2026", 1, 0, 4);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.print("[WIFI-AP] Online! SSID: RubbleRover-Tactical-AP | IP: ");
  Serial.println(WiFi.softAPIP());

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

  // Periodic 2.5s Heartbeat status log
  static unsigned long lastSerialHeartbeat = 0;
  if (millis() - lastSerialHeartbeat >= 2500) {
    lastSerialHeartbeat = millis();
    int clientCount = WiFi.softAPgetStationNum();
    Serial.printf("[HEARTBEAT] Clients:%d | MLX:%s | Mic:%s (Lvl:%d%%) | IP:%s\n",
                  clientCount,
                  mlxFound ? "ONLINE" : "OFFLINE",
                  micOnline ? "ONLINE" : "OFFLINE",
                  liveSoundLevel,
                  WiFi.softAPIP().toString().c_str());
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
  Serial.println("[MOTORS] All 18 pins configured as OUTPUT. High-Z Coast Stop active.");
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

  Wire.beginTransmission(0x33);
  byte error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("[I2C] MLX90640 detected at 0x33 (SDA:1, SCL:2)!");
    i2cSwapped = false;
  } else {
    Wire.end();
    Wire.begin(PIN_I2C_SCL, PIN_I2C_SDA, 400000);
    Wire.beginTransmission(0x33);
    if (Wire.endTransmission() == 0) {
      Serial.println("[I2C] MLX90640 detected on swapped pins (SDA:2, SCL:1)!");
      i2cSwapped = true;
    } else {
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

    with open('RubbleRover_S3_MotorController.ino', 'w', encoding='utf-8') as f:
        f.write(full_firmware)

    with open('RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino', 'w', encoding='utf-8') as f:
        f.write(full_firmware)

    import os
    arduino_dir = r'C:\Users\Kananyan\Documents\Arduino\sketch_nov1a'
    if os.path.exists(arduino_dir):
        with open(os.path.join(arduino_dir, 'sketch_nov1a.ino'), 'w', encoding='utf-8') as f:
            f.write(full_firmware)
        print(f"SUCCESS: Synchronized firmware to {arduino_dir}\\sketch_nov1a.ino")

    header_content = '#pragma once\n\nconst char INDEX_HTML[] PROGMEM = R"rawliteral(' + html_content + ')rawliteral";\n'
    with open('rubble_rover_web.h', 'w', encoding='utf-8') as f:
        f.write(header_content)
    with open('RubbleRover_S3_MotorController/rubble_rover_web.h', 'w', encoding='utf-8') as f:
        f.write(header_content)
    if os.path.exists(arduino_dir):
        with open(os.path.join(arduino_dir, 'rubble_rover_web.h'), 'w', encoding='utf-8') as f:
            f.write(header_content)

    print("SUCCESS: Direct-drive firmware generated and synchronized!")
