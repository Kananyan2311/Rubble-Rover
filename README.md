# Rubble Rover 🚜

Autonomous IoT Search & Reconnaissance Rover for Post-Earthquake First Responders.

Built with **ESP32-S3** (Master Controller & Sensor Hub) + **ESP32-CAM** (Optical Video Satellite).

---

## Features

- **6WD Skid-Steer Mobility:** Powered by 3x TB6612FNG dual motor drivers (6 DC gearmotors) with 1 kHz PWM and starting torque compensation.
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
|  - Core 1: WebServer (Port 80) + 1kHz High-Torque Motor PWM             |
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

## Controls

| Control | Action | Keyboard |
|---|---|---|
| **D-Pad Up** | Forward | `W` / `↑` |
| **D-Pad Down** | Reverse | `S` / `↓` |
| **D-Pad Left** | Steer Left | `A` / `←` |
| **D-Pad Right** | Steer Right | `D` / `→` |
| **Button B** | **Spin Left ↺** (Tank pivot in place) | `Q` / `B` |
| **Button A** | **Spin Right ↻** (Tank pivot in place) | `E` / `A` |
| **E-Stop** | Emergency Brake | `Space` |
| **Speed Gears** | Crawl (40%) / Norm (70%) / Turbo (100%) | Click button |
| **MOT A / B** | Invert Left or Right Motor Polarity | Click button |

---

## Repository Files

- [TROUBLESHOOTING.md](TROUBLESHOOTING.md): **Comprehensive troubleshooting & hardware debugging guide.**
- `RubbleRover_S3_MotorController/`: Master controller sketch for ESP32-S3 (`.ino` + `rubble_rover_web.h`).
- `RubbleRover_CAM_SurveillanceHub/`: Video streamer sketch for ESP32-CAM (`.ino`).
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

---

## License

[MIT License](LICENSE)
