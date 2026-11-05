# Rubble Rover 🚜

Autonomous IoT Search & Reconnaissance Rover for Post-Earthquake First Responders.

Built with **ESP32-S3** (Master Controller & Sensor Hub) + **ESP32-CAM** (Optical Video Satellite).

---

## Features

- **6WD Skid-Steer Mobility:** Powered by 3x TB6612FNG dual motor drivers (6 DC gearmotors) with high-torque direct digital drive and High-Z neutral coasting.
- **Thermal Imaging:** MLX90640 Far-Infrared array (32x24 pixels) with live heatmap and automatic survivor body heat alert ($\ge 33.5^\circ\text{C}$).
- **Digital Audio Stream:** INMP441 I2S MEMS microphone with real-time loudness VU meter.
- **Dedicated Video Feed:** ESP32-CAM streams live 640x480 MJPEG video with searchlight control.
- **Zero Internet Required:** Self-hosted standalone Wi-Fi Access Point (`192.168.4.1`).
- **Tactical Web Console:** Retro Neo-Brutalist GameBoy interface with multi-touch D-pad and dedicated A/B pivot spin buttons.

---

## System Overview

```
                      +-----------------------------+
                      |   Field Operator Device     |
                      |  (Phone / Tablet / Laptop)  |
                      +--------------+--------------+
                                     |
                          Wi-Fi: RubbleRover-Tactical-AP
                                     |
                                     v
+------------------------------------+------------------------------------+
|                   ESP32-S3 Master Tactical Controller                   |
|                             (192.168.4.1)                               |
|                                                                         |
|  - Core 0: 8Hz Thermal Polling (MLX90640) + Audio Sampling (INMP441)   |
|  - Core 1: WebServer (Port 80) + High-Torque Motor Control              |
+-------------------+---------------------------------+-------------------+
                    |                                 |
           18 PWM & DIR Pins                  Wi-Fi Direct STA
                    |                         (192.168.4.2 Handshake)
                    v                                 v
        +-----------------------+         +-----------------------+
        |   3x TB6612 Drivers   |         |       ESP32-CAM       |
        |   (6 DC Gearmotors)   |         |   Live Video Satellite|
        +-----------------------+         +-----------------------+
```

---

## Wiring & Pinout

### 1. Motors (3x TB6612FNG)
*All TB6612 `STBY` pins are hardwired to 3.3V.*

| Motor | Driver Net | ESP32-S3 Pin | Function |
|---|---|---|---|
| **Front Left** | `PWMA` / `AIN1` / `AIN2` | **GPIO 17** / **GPIO 18** / **GPIO 8** | Speed (PWM) / Dir 1 / Dir 2 |
| **Front Right** | `PWMB` / `BIN1` / `BIN2` | **GPIO 11** / **GPIO 9** / **GPIO 10** | Speed (PWM) / Dir 1 / Dir 2 |
| **Mid Left** | `PWMA` / `AIN1` / `AIN2` | **GPIO 4** / **GPIO 5** / **GPIO 6** | Speed (PWM) / Dir 1 / Dir 2 |
| **Mid Right** | `PWMB` / `BIN1` / `BIN2` | **GPIO 16** / **GPIO 7** / **GPIO 15** | Speed (PWM) / Dir 1 / Dir 2 |
| **Rear Left** | `PWMA` / `AIN1` / `AIN2` | **GPIO 12** / **GPIO 13** / **GPIO 14** | Speed (PWM) / Dir 1 / Dir 2 |
| **Rear Right** | `PWMB` / `BIN1` / `BIN2` | **GPIO 21** / **GPIO 38** / **GPIO 47** | Speed (PWM) / Dir 1 / Dir 2 |

### 2. Sensors (Header J3)
| Sensor | Net | ESP32-S3 Pin | Notes |
|---|---|---|---|
| **MLX90640** | `SCL` | **GPIO 1** | I2C Clock (with 4.7k pull-up) |
| **MLX90640** | `SDA` | **GPIO 2** | I2C Data (with 4.7k pull-up) |
| **INMP441** | `SCK` | **GPIO 42** | I2S Bit Clock (BCLK) |
| **INMP441** | `WS` | **GPIO 41** | I2S Word Select (LRCK) |
| **INMP441** | `SD` | **GPIO 40** | I2S Serial Data (DIN) |
| **INMP441** | `L/R` | **GND** | Tied to GND (Left channel) |

---

## Quick Setup & Flashing

### Step 1: Install Libraries in Arduino IDE
Open the **Library Manager** in Arduino IDE and install:
1. `Adafruit MLX90640` (v1.1.2+)
2. `Adafruit BusIO`

### Step 2: Flash the ESP32-S3 Master
1. Open `RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino`.
2. Select Board: **ESP32S3 Dev Module**.
3. Set Tools settings:
   - **USB CDC On Boot:** `Enabled`
   - **Partition Scheme:** `Huge APP (3MB No OTA/1MB SPIFFS)`
   - **Upload Mode:** `UART0 / Hardware CDC`
   - **USB Mode:** `Hardware CDC and JTAG`
   - **PSRAM:** `Disabled`
4. Select your COM port and click **Upload**.

### Step 3: Flash the ESP32-CAM
1. Open `RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino`.
2. Select Board: **AI Thinker ESP32-CAM**.
3. Bridge `GPIO 0` to `GND`, upload, then remove bridge and press **RST**.

### Step 4: Connect & Drive
1. On your phone or laptop, connect to Wi-Fi:
   - **SSID:** `RubbleRover-Tactical-AP`
   - **Password:** `RubbleRover2026`
2. Open your browser and go to: `http://192.168.4.1`

---

## Expected Serial Monitor Output (How to Know It Works)

Open the Serial Monitor in Arduino IDE set to **115200 baud**. Below is the exact output sequence that confirms all systems are working properly.

### 1. ESP32-S3 Master Controller (Normal Boot)

When you press the `RST` button, you should see this banner and initialization log:

```text
====================================================================
  RUBBLEROVER: DIRECT-DRIVE 6WD CONTROLLER ONLINE
  Motor A: Inverted (-1) | Motor B: Normal (+1) | High-Z Coasting
====================================================================
[WIFI-AP] Online! SSID: RubbleRover-Tactical-AP | IP: 192.168.4.1
[AUDIO] INMP441 Initialized via ESP_I2S (16kHz).
[I2C] Initializing I2C for MLX90640...
[I2C] MLX90640 detected at 0x33 (SDA:1, SCL:2)!
[MLX90640] MLX90640 Initialized Successfully!
[HTTP] Web Server Online on Port 80
[MOTORS] All 18 pins configured as OUTPUT. High-Z Coast Stop active.
[HEARTBEAT] Clients:0 | MLX:ONLINE | Mic:ONLINE (Lvl:2%) | IP:192.168.4.1
```

#### What each line confirms:
- **`[WIFI-AP] Online!`** $\rightarrow$ The Wi-Fi Access Point is broadcasting and ready for connections at `192.168.4.1`.
- **`[AUDIO] INMP441 Initialized`** $\rightarrow$ I2S microphone bus on GPIO 42/41/40 is active and sampling at 16 kHz.
- **`[I2C] MLX90640 detected at 0x33`** $\rightarrow$ I2C communication succeeded on SDA: GPIO 2, SCL: GPIO 1.
- **`[MOTORS] All 18 pins configured... High-Z Coast`** $\rightarrow$ All 18 motor pins are set to LOW. Motors spin freely by hand without dynamic brake drag.
- **`[HEARTBEAT] Clients:0 | MLX:ONLINE | Mic:ONLINE (Lvl:X%)`** $\rightarrow$ The background core task is alive and healthy, printing every 2.5 seconds. Clap near the mic to see `Lvl:%` increase!

### 2. Runtime Event Logs on ESP32-S3

Once operating, the Serial Monitor will log key events:

- **When ESP32-CAM connects and registers:**
  ```text
  [CAMERA-HUB] Registered at IP: 192.168.4.2
  ```
- **When phone/laptop connects to Wi-Fi:**
  ```text
  [HEARTBEAT] Clients:1 | MLX:ONLINE | Mic:ONLINE (Lvl:6%) | IP:192.168.4.1
  ```
- **When running the 6-motor diagnostic test (`http://192.168.4.1/test_motors`):**
  ```text
  --- STARTING 6-MOTOR HARDWARE DIAGNOSTIC ---
  [DIAG 1/6] Front-Left (Motor A)...
  [DIAG 2/6] Front-Right (Motor B)...
  [DIAG 3/6] Mid-Left (Motor A)...
  [DIAG 4/6] Mid-Right (Motor B)...
  [DIAG 5/6] Rear-Left (Motor A)...
  [DIAG 6/6] Rear-Right (Motor B)...
  --- MOTOR DIAGNOSTIC COMPLETE ---
  ```
- **If Wi-Fi signal is lost while driving (Watchdog Failsafe):**
  ```text
  [WATCHDOG] Failsafe timeout! Stopping motors.
  ```

### 3. ESP32-CAM Video Satellite (Normal Boot)

When powering on the ESP32-CAM (with GPIO 0 jumper removed), its Serial Monitor (115200 baud) should print:

```text
====================================================================
  RUBBLEROVER: ESP32-CAM HIGH-SPEED VIDEO SATELLITE
  Instant Handshake & Low-Latency Stream Architecture
====================================================================
[WIFI-STA] Connecting immediately to Master AP: RubbleRover-Tactical-AP (Ch 1)...
.
[WIFI-STA] CONNECTED in 240ms! Node IP: 192.168.4.2
[HANDSHAKE] Registered 192.168.4.2 with ESP32-S3 Master!
[CAMERA] MJPEG Streamer ready on http://192.168.4.2/stream
```

- **When Searchlight is toggled from the WebApp:**
  ```text
  [LIGHT] Searchlight LED state: 1
  ```

---

## Controls

| Control | Action | Keyboard Shortcut |
|---|---|---|
| **D-Pad Up** | Forward | `W` / `↑` |
| **D-Pad Down** | Reverse | `S` / `↓` |
| **D-Pad Left** | Steer Left | `A` / `←` |
| **D-Pad Right** | Steer Right | `D` / `→` |
| **Button B** | **Spin Left ↺** (Tank pivot in place) | `Q` / `B` |
| **Button A** | **Spin Right ↻** (Tank pivot in place) | `E` / `A` |
| **E-Stop** | Emergency Brake | `Space` |
| **Speed Gears** | Crawl (40%) / Normal (70%) / Turbo (100%) | Click button |
| **MOT A / B** | Invert Left or Right Motor Polarity | Click button |

---

## Firmware Edition 2: Dual Wi-Fi Router + AP Mode (`Firmware_v2_Router_And_AP/`)

If you want both the **ESP32-S3 Motor Controller** and the **ESP32-CAM Surveillance Hub** to operate on independent connections—capable of joining a **Wi-Fi Router / Hotspot (`STA`)** while simultaneously broadcasting their own **Access Points (`AP`)**—use the 2nd firmware folder:

### 1. `Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/`
- **Simultaneous Router + AP (`WIFI_AP_STA`):**
  - Broadcasts **`RubbleRover-Tactical-AP`** (`RubbleRover2026`) at `http://192.168.4.1`.
  - Simultaneously connects to your **Wi-Fi Router** (`routerSSID` / `routerPass` in the code, or click the top Wi-Fi badge on the HUD to join a router dynamically via `/set_wifi?ssid=...&pass=...`).
  - Prints both the **AP IP (`192.168.4.1`)** and the **Router IP** in the Serial Monitor every 2.5s:
    ```text
    [HEARTBEAT] AP:192.168.4.1 (Clients:0) | Router:192.168.1.105 | MLX:ONLINE | Mic:ONLINE (Lvl:4%)
    ```
  - Click the **`CAM IP: ... (SET)`** header on the GameBoy HUD to point the video player to any Camera Router IP or AP IP.

### 2. `Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/`
- **Independent Dual Wi-Fi Camera Hub (`WIFI_AP_STA`):**
  - Broadcasts its own standalone Camera Access Point: **`RubbleRover-CAM-AP`** (`RubbleRover2026`) at `http://192.168.5.1`.
  - Simultaneously connects to your **Wi-Fi Router** (`routerSSID` / `routerPass` in the code, or via the built-in web form at `http://192.168.5.1`).
  - Includes a built-in **Standalone Camera Surveillance Web Console** at `http://192.168.5.1/` (or `http://<CAM_ROUTER_IP>/`) with live MJPEG stream (`/stream` & `:81/stream`), Searchlight toggle (`/light`), and Router Wi-Fi setup.

---

## Repository Files

- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): **Comprehensive troubleshooting & hardware debugging guide.**
- `RubbleRover_S3_MotorController/`: **Edition 1 (Direct AP)** Master controller sketch for ESP32-S3.
- `RubbleRover_CAM_SurveillanceHub/`: **Edition 1 (Direct S3 Handshake)** Video streamer sketch for ESP32-CAM.
- `Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/`: **Edition 2 (Router + AP)** Dual Wi-Fi Master controller sketch for ESP32-S3.
- `Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/`: **Edition 2 (Independent Router + AP)** Standalone Dual Wi-Fi Surveillance sketch for ESP32-CAM.
- `RubbleRover_WebApp.html`: Standalone GameBoy GCS interface.
- `ESP32_S3_ROVER_PINOUT.csv`: Complete pin assignment table.
- `ESP32_S3_ROVER_PINOUT_PCB_GUIDE.txt`: Hardware wiring and PCB routing manual.

---

## Troubleshooting

Encountering an issue? Refer to the full [TROUBLESHOOTING.md](TROUBLESHOOTING.md) guide.

**Quick-Fix Summary:**
- **ESP32-S3 stuck in download mode:** Unplug USB, release `BOOT` button, replug, click `RST` once.
- **Serial Monitor blank:** Set **Tools → USB CDC On Boot → Enabled** in Arduino IDE and re-flash.
- **Motors stiff / hard to turn:** Upload direct-drive firmware; `stopMotors()` sets pins LOW for neutral High-Z coast.
- **Motors won't move:** Ensure TB6612 `STBY` is wired to 3.3V, VM battery is connected, and battery ground is tied to ESP32 GND.
- **Web App won't open at 192.168.4.1:** Turn OFF mobile/cellular data on your phone so it doesn't bypass the rover AP.
- **ESP32-CAM stream offline:** Remove programming jumper on GPIO 0; ensure camera is powered from 5V rail.
- **Serial Monitor Error Decoder:** See [TROUBLESHOOTING.md Section 9](TROUBLESHOOTING.md#9-serial-monitor-error-message-decoder-what-if-i-see-this) for exact error string translations (Brownout, Guru Meditation, I2C timeouts, gibberish, and camera errors).

---

## License

[MIT License](LICENSE)
