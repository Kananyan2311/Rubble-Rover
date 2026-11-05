<div align="center">

# 🚜 RUBBLE ROVER
### **Autonomous IoT Search, Thermal Reconnaissance & Tactical Communication Rover**
*Designed for Post-Earthquake Victim Search & First Responder Assistance*

[![ESP32-S3 Master](https://img.shields.io/badge/MCU-ESP32--S3%20Dual--Core-FF6B6B?style=for-the-badge&logo=espressif&logoColor=white)](RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino)
[![ESP32-CAM Video](https://img.shields.io/badge/OPTICAL-ESP32--CAM%20OV2640-4D96FF?style=for-the-badge&logo=espressif&logoColor=white)](RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino)
[![MLX90640 Thermal](https://img.shields.io/badge/THERMAL-MLX90640%2032x24%20FLIR-FFD21E?style=for-the-badge&logoColor=black)](#2-sensors-header-j3)
[![INMP441 Audio](https://img.shields.io/badge/AUDIO-INMP441%20I2S%20MEMS-4ECCA3?style=for-the-badge&logoColor=black)](#2-sensors-header-j3)
[![License MIT](https://img.shields.io/badge/LICENSE-MIT-9B5DE5?style=for-the-badge)](LICENSE)

</div>

---

## 🗂️ Quick-Access Document & Firmware Hub *(Click Any Box)*

<table>
  <tr>
    <td align="center" width="33%">
      <a href="TROUBLESHOOTING.md">
        <img src="https://img.shields.io/badge/%F0%9F%9B%A0%EF%B8%8F_TROUBLESHOOTING_GUIDE-OPEN_DOC-FF6B6B?style=for-the-badge" alt="Troubleshooting Guide" />
      </a>
      <br />
      <sub><b>21 Serial Error Decoders, Bootloader Fixes, Motor & Sensor Debugging</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="ESP32_S3_ROVER_PINOUT.csv">
        <img src="https://img.shields.io/badge/%F0%9F%93%8C_MASTER_PINOUT_CSV-OPEN_TABLE-4ECCA3?style=for-the-badge" alt="Pinout CSV" />
      </a>
      <br />
      <sub><b>Complete 23-GPIO Assignment Spreadsheet (Zero Strapping Conflicts)</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="ESP32_S3_ROVER_PINOUT_PCB_GUIDE.txt">
        <img src="https://img.shields.io/badge/%F0%9F%93%90_PCB_%26_HARDWARE_GUIDE-OPEN_MANUAL-FFD21E?style=for-the-badge" alt="PCB Guide" />
      </a>
      <br />
      <sub><b>Full Hardware Architecture, Power Isolation & PCB Routing Manual</b></sub>
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <a href="RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino">
        <img src="https://img.shields.io/badge/%E2%9A%A1_S3_FIRMWARE_v1-DIRECT_AP-4D96FF?style=for-the-badge" alt="S3 Firmware v1" />
      </a>
      <br />
      <sub><b>Edition 1: Standalone Tactical AP (<code>192.168.4.1</code>) Master Sketch</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino">
        <img src="https://img.shields.io/badge/%F0%9F%93%B7_CAM_FIRMWARE_v1-DIRECT_STA-4D96FF?style=for-the-badge" alt="CAM Firmware v1" />
      </a>
      <br />
      <sub><b>Edition 1: Auto-Handshake Video Satellite (<code>192.168.4.2</code>)</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="RubbleRover_WebApp.html">
        <img src="https://img.shields.io/badge/%F0%9F%8E%AE_WEBAPP_CONSOLE-OPEN_HTML-9B5DE5?style=for-the-badge" alt="WebApp HTML" />
      </a>
      <br />
      <sub><b>Standalone Neo-Brutalist GameBoy Ground Control Station UI</b></sub>
    </td>
  </tr>
  <tr>
    <td align="center" width="33%">
      <a href="Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/RubbleRover_S3_MotorController_v2.ino">
        <img src="https://img.shields.io/badge/%F0%9F%8C%90_S3_FIRMWARE_v2-ROUTER_%2B_AP-F15BB5?style=for-the-badge" alt="S3 Firmware v2" />
      </a>
      <br />
      <sub><b>Edition 2: Connects to Wi-Fi Router + Broadcasts Fallback AP</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/RubbleRover_CAM_SurveillanceHub_v2.ino">
        <img src="https://img.shields.io/badge/%F0%9F%93%A1_CAM_FIRMWARE_v2-ROUTER_%2B_AP-F15BB5?style=for-the-badge" alt="CAM Firmware v2" />
      </a>
      <br />
      <sub><b>Edition 2: Independent Camera Router Connection + Own AP (<code>192.168.5.1</code>)</b></sub>
    </td>
    <td align="center" width="33%">
      <a href="LICENSE">
        <img src="https://img.shields.io/badge/%F0%9F%93%9C_MIT_LICENSE-VIEW_TERMS-00BBF9?style=for-the-badge" alt="MIT License" />
      </a>
      <br />
      <sub><b>Open-Source MIT Hardware & Software License</b></sub>
    </td>
  </tr>
</table>

---

## 🧭 Quick Section Jump

<div align="center">

[![Features](https://img.shields.io/badge/%E2%9C%A8_Features-FFD21E?style=flat-square&logoColor=black)](#-key-features)
[![Architecture](https://img.shields.io/badge/%F0%9F%8F%97%EF%B8%8F_Architecture-4ECCA3?style=flat-square&logoColor=black)](#%EF%B8%8F-system-architecture)
[![Wiring & Pinout](https://img.shields.io/badge/%F0%9F%94%8C_Wiring_%26_Pinout-4D96FF?style=flat-square&logoColor=white)](#-wiring--pinout)
[![Flashing Setup](https://img.shields.io/badge/%F0%9F%9A%80_Quick_Setup-FF6B6B?style=flat-square&logoColor=white)](#-quick-setup--flashing)
[![Serial Output](https://img.shields.io/badge/%F0%9F%96%A5%EF%B8%8F_Serial_Monitor_Check-9B5DE5?style=flat-square&logoColor=white)](#%EF%B8%8F-expected-serial-monitor-output-how-to-know-it-works)
[![Edition 2 Dual WiFi](https://img.shields.io/badge/%F0%9F%8C%90_Firmware_v2_(Router%2BAP)-F15BB5?style=flat-square&logoColor=white)](#-firmware-edition-2-dual-wi-fi-router--ap-mode-firmware_v2_router_and_ap)
[![Controls](https://img.shields.io/badge/%F0%9F%8E%AE_Controls-00F5D4?style=flat-square&logoColor=black)](#-tactical-controls)

</div>

---

## ✨ Key Features

> [!TIP]
> **Built for Zero-Infrastructure Disaster Zones:** Rubble Rover operates with **zero external internet or router dependencies** in Edition 1, or seamlessly bridges into an existing **Wi-Fi Router + Fallback AP** in Edition 2.

- 🟢 **6WD High-Torque Skid-Steer Mobility:** Powered by **3x TB6612FNG** dual H-bridge drivers (6 DC gearmotors) with direct digital full-torque drive, default Motor A physical inversion (`polarityA = -1`), and **High-Z Neutral Coasting** (wheels spin freely by hand when idle).
- 🔴 **Survivor Thermal Imaging:** **MLX90640** Far-Infrared array ($32 \times 24$ pixels) with live bicubic FLIR heatmap and automatic human body heat alert ($\ge 33.5^\circ\text{C}$).
- 🟡 **Digital Acoustic Tracking:** **INMP441** 24-bit I2S MEMS microphone with real-time VU meter and non-blocking WAV audio streaming.
- 🔵 **Low-Latency Optical Feed:** **ESP32-CAM (OV2640)** streams live $640 \times 480$ MJPEG video with remote high-power LED searchlight control.
- 🟣 **Neo-Brutalist GameBoy Console:** Embedded multi-touch D-pad, dedicated **A/B tank spin buttons**, live temperature telemetry, and an In-Flight Coalescing Control Scheduler.

---

## 🏗️ System Architecture

```mermaid
flowchart TD
    subgraph GCS["📱 Field Operator Device (Phone / Tablet / Laptop)"]
        UI["🎮 Neo-Brutalist GameBoy Web Console"]
    end

    subgraph S3["🧠 ESP32-S3 Master Tactical Controller (192.168.4.1 / Router IP)"]
        C1["⚡ Core 1: Port 80 WebServer + Direct 6WD Motor Control + 1000ms Watchdog"]
        C0["🌡️ Core 0: 8Hz MLX90640 Thermal Polling + 16kHz INMP441 I2S Audio"]
    end

    subgraph HW["⚙️ Rover Hardware & Satellite Nodes"]
        MOT["🛞 3x TB6612FNG Drivers (6x DC Gearmotors)"]
        MLX["🔥 MLX90640 Thermal Array (GPIO 1 & 2)"]
        MIC["🎙️ INMP441 Digital Mic (GPIO 42, 41, 40)"]
        CAM["📷 ESP32-CAM Video Satellite (192.168.4.2 / 192.168.5.1 / Router IP)"]
    end

    UI <-->|"Wi-Fi HTTP / MJPEG / WAV"| C1
    C1 -->|"18 PWM & DIR Pins"| MOT
    C0 <-->|"I2C 400kHz/800kHz"| MLX
    C0 <--|"I2S DMA"| MIC
    CAM <-->|"MJPEG /stream & /light"| UI
```

---

## 🔌 Wiring & Pinout

> [!IMPORTANT]
> **Silicon Safety Verified:** All 23 assigned ESP32-S3 pins strictly avoid Octal PSRAM pins (`GPIO 33–37`) and hardware strapping pins (`GPIO 0, 3, 45, 46`). All TB6612 `STBY` pins must be tied to **3.3V**.

### 1. Motors (3x TB6612FNG — 18 GPIOs)

| Wheel Position | Driver Channel | PWM (Speed) | IN1 (Dir 1) | IN2 (Dir 2) | Default Polarity |
| :--- | :--- | :---: | :---: | :---: | :---: |
| 🛞 **Front-Left (FL)** | Driver #1 — Motor A | `GPIO 17` | `GPIO 18` | `GPIO 8` | 🔴 **Inverted (`-1`)** |
| 🛞 **Front-Right (FR)** | Driver #1 — Motor B | `GPIO 11` | `GPIO 9` | `GPIO 10` | 🟢 **Normal (`+1`)** |
| 🛞 **Mid-Left (ML)** | Driver #2 — Motor A | `GPIO 4` | `GPIO 5` | `GPIO 6` | 🔴 **Inverted (`-1`)** |
| 🛞 **Mid-Right (MR)** | Driver #2 — Motor B | `GPIO 16` | `GPIO 7` | `GPIO 15` | 🟢 **Normal (`+1`)** |
| 🛞 **Rear-Left (RL)** | Driver #3 — Motor A | `GPIO 12` | `GPIO 13` | `GPIO 14` | 🔴 **Inverted (`-1`)** |
| 🛞 **Rear-Right (RR)** | Driver #3 — Motor B | `GPIO 21` | `GPIO 38` | `GPIO 47` | 🟢 **Normal (`+1`)** |

### 2. Sensors (Header J3 — 5 GPIOs)

| Sensor Module | Signal Net | ESP32-S3 Pin | Electrical Requirement |
| :--- | :--- | :---: | :--- |
| 🔥 **MLX90640 Thermal** | `SCL` (I2C Clock) | **`GPIO 1`** | 3.3V only + $4.7\text{k}\Omega$ pull-up to 3.3V |
| 🔥 **MLX90640 Thermal** | `SDA` (I2C Data) | **`GPIO 2`** | 3.3V only + $4.7\text{k}\Omega$ pull-up to 3.3V |
| 🎙️ **INMP441 Microphone** | `SCK` (Bit Clock) | **`GPIO 42`** | 3.3V only |
| 🎙️ **INMP441 Microphone** | `WS` (Word Select) | **`GPIO 41`** | 3.3V only |
| 🎙️ **INMP441 Microphone** | `SD` (Serial Data) | **`GPIO 40`** | 3.3V only |
| 🎙️ **INMP441 Microphone** | `L/R` (Channel) | **`GND`** | **Hardwire to GND** (Selects Left Channel) |

---

## 🚀 Quick Setup & Flashing

### Step 1: Install Required Libraries
In Arduino IDE, open **Tools → Manage Libraries** and install:
1. 📦 **`Adafruit MLX90640`** *(v1.1.2+)*
2. 📦 **`Adafruit BusIO`**

### Step 2: Flash the ESP32-S3 Master Controller
1. Open either:
   - **Edition 1 (Direct AP):** [`RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino`](RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino)
   - **Edition 2 (Router + AP):** [`Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/RubbleRover_S3_MotorController_v2.ino`](Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/RubbleRover_S3_MotorController_v2.ino)
2. Select Board: **ESP32S3 Dev Module** and configure **Tools**:

| Arduino IDE Setting | Required Value | Why It Matters |
| :--- | :--- | :--- |
| **USB CDC On Boot** | 🟢 **`Enabled`** | **CRITICAL:** Routes `Serial.println()` to the USB-C port |
| **Partition Scheme** | 🟣 **`Huge APP (3MB No OTA/1MB SPIFFS)`** | Fits full WebApp + Wi-Fi + Thermal stack |
| **Upload Mode** | 🔵 **`UART0 / Hardware CDC`** | Ensures reliable native USB flashing |
| **USB Mode** | 🔵 **`Hardware CDC and JTAG`** | Enables native USB serial monitor |
| **PSRAM** | 🟡 **`Disabled`** | Prevents Octal/Quad PSRAM pin conflicts |

3. Select your COM port and click **Upload**.

### Step 3: Flash the ESP32-CAM Video Satellite
1. Open either:
   - **Edition 1 (Auto S3 Handshake):** [`RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino`](RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino)
   - **Edition 2 (Independent Router + AP):** [`Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/RubbleRover_CAM_SurveillanceHub_v2.ino`](Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/RubbleRover_CAM_SurveillanceHub_v2.ino)
2. Select Board: **AI Thinker ESP32-CAM**.
3. Bridge `GPIO 0` to `GND` to upload, then **remove the `GPIO 0` bridge** and press **RST**.

### Step 4: Connect & Drive
1. Connect your phone or laptop to Wi-Fi:
   - 📶 **SSID:** `RubbleRover-Tactical-AP` *(Password: `RubbleRover2026`)*
2. *(On smartphones: temporarily turn OFF Mobile Cellular Data so the browser stays on the rover network).*
3. Open your browser to **`http://192.168.4.1`** (or the Router IP shown in Serial Monitor for Edition 2).

---

## 🖥️ Expected Serial Monitor Output (How to Know It Works)

Open the Arduino IDE Serial Monitor at **`115200 baud`** and press **`RST`** on the board.

### 1. ESP32-S3 Master Controller (Normal Boot)

```text
====================================================================
  RUBBLEROVER: DIRECT-DRIVE 6WD CONTROLLER ONLINE
  Motor A: Inverted (-1) | Motor B: Normal (+1) | High-Z Coasting
====================================================================
[MOTORS] All 18 pins configured as OUTPUT. High-Z Coast Stop active.
[WIFI-AP] Online! SSID: RubbleRover-Tactical-AP | IP: 192.168.4.1
[AUDIO] INMP441 Initialized via ESP_I2S (16kHz).
[I2C] Initializing I2C for MLX90640...
[I2C] MLX90640 detected at 0x33 (SDA:1, SCL:2)!
[MLX90640] MLX90640 Initialized Successfully!
[HTTP] Web Server Online on Port 80
[HEARTBEAT] Clients:0 | MLX:ONLINE | Mic:ONLINE (Lvl:2%) | IP:192.168.4.1
```

| Serial Log Line | What It Confirms |
| :--- | :--- |
| 🟢 `[MOTORS] High-Z Coast Stop active` | All 18 motor pins are driven `LOW`. Wheels spin freely by hand with zero dynamic brake drag. |
| 🟢 `[WIFI-AP] Online! ... 192.168.4.1` | SoftAP is broadcasting and ready for your phone or laptop. |
| 🟢 `[AUDIO] INMP441 Initialized` | I2S microphone on `GPIO 42/41/40` is sampling at 16 kHz. |
| 🟢 `[I2C] MLX90640 detected at 0x33` | Thermal camera is communicating on `SDA:2, SCL:1` (or auto-swapped if wires were reversed). |
| 🟢 `[HEARTBEAT] ... Mic:ONLINE (Lvl:X%)` | Core 0 task is healthy, printing every 2.5s. Clap near the mic to watch `Lvl:%` jump! |

### 2. Runtime Event Logs on ESP32-S3

- 📷 **When ESP32-CAM connects and registers:**
  ```text
  [CAMERA-HUB] Registered at IP: 192.168.4.2
  ```
- 🔧 **When running the 6-motor diagnostic (`http://192.168.4.1/test_motors`):**
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
- 🛡️ **When operator releases controls or disconnects (>1000 ms Watchdog):**
  ```text
  [WATCHDOG] Failsafe timeout! Stopping motors.
  ```

### 3. ESP32-CAM Video Satellite (Normal Boot)

```text
====================================================================
  RUBBLEROVER: ESP32-CAM HIGH-SPEED VIDEO SATELLITE
  Instant Handshake & Low-Latency Stream Architecture
====================================================================
[CAMERA] OV2640 Initialized Successfully (640x480, Low Latency)!
[WIFI-STA] Connecting immediately to Master AP: RubbleRover-Tactical-AP (Ch 1)...
[WIFI-STA] CONNECTED in 240ms! Node IP: 192.168.4.2
[HANDSHAKE] Registered 192.168.4.2 with ESP32-S3 Master!
[HTTP] Camera MJPEG Streaming Server online on port 80
```

---

## 🌐 Firmware Edition 2: Dual Wi-Fi Router + AP Mode (`Firmware_v2_Router_And_AP/`)

> [!NOTE]
> Use **Edition 2** when you want both the **Motor Controller** and the **Camera Surveillance Hub** to connect to an external **Wi-Fi Router / Hotspot (`STA`)** while simultaneously broadcasting their own independent **Access Points (`AP`)**.

| Firmware Sketch | Simultaneous AP Mode | Simultaneous Router Mode (`STA`) | Special Features |
| :--- | :--- | :--- | :--- |
| 🧠 **[S3 Motor Controller v2](Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/RubbleRover_S3_MotorController_v2.ino)** | **SSID:** `RubbleRover-Tactical-AP`<br>**IP:** `http://192.168.4.1` | Joins `routerSSID` (`DHCP IP` printed in Serial Monitor) | Click top Wi-Fi badge on HUD to join a router; click `CAM IP` header to switch camera IP anytime. |
| 📷 **[CAM Surveillance Hub v2](Firmware_v2_Router_And_AP/RubbleRover_CAM_SurveillanceHub_v2/RubbleRover_CAM_SurveillanceHub_v2.ino)** | **SSID:** `RubbleRover-CAM-AP`<br>**IP:** `http://192.168.5.1` | Joins `routerSSID` (`DHCP IP` printed in Serial Monitor) | Includes its own standalone **Camera Web Console** at `http://192.168.5.1/` with video, searchlight & router setup! |

---

## 🎮 Tactical Controls

| Control Input | Rover Action | Keyboard Shortcut |
| :--- | :--- | :---: |
| ⬆️ **D-Pad Up** | Drive All 6 Wheels Forward | <kbd>W</kbd> / <kbd>↑</kbd> |
| ⬇️ **D-Pad Down** | Drive All 6 Wheels Reverse | <kbd>S</kbd> / <kbd>↓</kbd> |
| ⬅️ **D-Pad Left** | Arc Steer Left | <kbd>A</kbd> / <kbd>←</kbd> |
| ➡️ **D-Pad Right** | Arc Steer Right | <kbd>D</kbd> / <kbd>→</kbd> |
| 🔴 **Button B** | **Spin Left ↺** *(Tank Pivot Counter-Clockwise)* | <kbd>Q</kbd> / <kbd>B</kbd> |
| 🔴 **Button A** | **Spin Right ↻** *(Tank Pivot Clockwise)* | <kbd>E</kbd> / <kbd>A</kbd> |
| 🛑 **E-Stop** | Instant Emergency Stop (High-Z Coast) | <kbd>Space</kbd> |
| ⚙️ **Speed Gears** | **40% Crawl** (`180`) / **70% Norm** (`220`) / **100% Turbo** (`255`) | Click HUD Button |
| 🔄 **MOT A / B** | Live Left / Right Motor Polarity Inversion | Click HUD Button |

---

## 🛠️ Troubleshooting Quick-Reference

> [!WARNING]
> Encountering a blank Serial Monitor, stuck download mode, stiff motors, or offline camera? Click the button below to open the full color-coded **Troubleshooting & 21-Error Decoder Guide**:

<div align="center">

[![Open Full Troubleshooting Guide](https://img.shields.io/badge/%F0%9F%9B%A0%EF%B8%8F_OPEN_FULL_TROUBLESHOOTING_%26_SERIAL_DECODER_GUIDE-FF6B6B?style=for-the-badge)](TROUBLESHOOTING.md)

</div>

---

## 📜 License

Released under the [MIT License](LICENSE).
