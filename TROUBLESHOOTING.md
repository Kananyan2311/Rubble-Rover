<div align="center">

# 🛠️ RUBBLE ROVER TROUBLESHOOTING & SERIAL DECODER GUIDE
### **Step-by-Step Hardware Debugging, Bootloader Recovery & 21 Serial Monitor Error Translations**

[![Back to README](https://img.shields.io/badge/%E2%AC%85%EF%B8%8F_BACK_TO_MAIN_README-121212?style=for-the-badge)](README.md)
[![Pinout CSV](https://img.shields.io/badge/%F0%9F%93%8C_PINOUT_CSV-4ECCA3?style=for-the-badge&logoColor=black)](ESP32_S3_ROVER_PINOUT.csv)
[![PCB Guide](https://img.shields.io/badge/%F0%9F%93%90_PCB_MANUAL-FFD21E?style=for-the-badge&logoColor=black)](ESP32_S3_ROVER_PINOUT_PCB_GUIDE.txt)
[![WebApp HTML](https://img.shields.io/badge/%F0%9F%8E%AE_WEBAPP_UI-9B5DE5?style=for-the-badge)](RubbleRover_WebApp.html)

</div>

---

## 🗂️ Clickable Document & Firmware Access Grid

<table>
  <tr>
    <td align="center" width="25%">
      <a href="README.md">
        <img src="https://img.shields.io/badge/%F0%9F%93%96_MAIN_README-OPEN-4D96FF?style=for-the-badge" alt="Main README" />
      </a>
      <br />
      <sub><b>System Overview & Setup</b></sub>
    </td>
    <td align="center" width="25%">
      <a href="RubbleRover_S3_MotorController/RubbleRover_S3_MotorController.ino">
        <img src="https://img.shields.io/badge/%E2%9A%A1_S3_MASTER_v1-OPEN-FF6B6B?style=for-the-badge" alt="S3 v1" />
      </a>
      <br />
      <sub><b>Direct AP Mode (<code>192.168.4.1</code>)</b></sub>
    </td>
    <td align="center" width="25%">
      <a href="RubbleRover_CAM_SurveillanceHub/RubbleRover_CAM_SurveillanceHub.ino">
        <img src="https://img.shields.io/badge/%F0%9F%93%B7_CAM_HUB_v1-OPEN-4ECCA3?style=for-the-badge" alt="CAM v1" />
      </a>
      <br />
      <sub><b>Direct S3 Link (<code>192.168.4.2</code>)</b></sub>
    </td>
    <td align="center" width="25%">
      <a href="Firmware_v2_Router_And_AP/RubbleRover_S3_MotorController_v2/RubbleRover_S3_MotorController_v2.ino">
        <img src="https://img.shields.io/badge/%F0%9F%8C%90_DUAL_WIFI_v2-OPEN-F15BB5?style=for-the-badge" alt="Dual WiFi v2" />
      </a>
      <br />
      <sub><b>Router + AP Edition (S3 & CAM)</b></sub>
    </td>
  </tr>
</table>

---

## 🧭 Jump to Troubleshooting Category *(Click Any Badge)*

<div align="center">

[![30s Drill](https://img.shields.io/badge/%E2%9A%A1_30s_Quick_Fix-FFD21E?style=for-the-badge&logoColor=black)](#%EF%B8%8F-the-30-second-quick-fix-drill)
[![Boot & Upload](https://img.shields.io/badge/1%EF%B8%8F%E2%83%A3_Boot_%26_Upload-FF6B6B?style=for-the-badge)](#-1-esp32-s3-boot--flashing-issues)
[![Blank Serial](https://img.shields.io/badge/2%EF%B8%8F%E2%83%A3_Blank_Serial_Monitor-4D96FF?style=for-the-badge)](#%EF%B8%8F-2-serial-monitor-issues-blank-screen--no-output)
[![Motors & TB6612](https://img.shields.io/badge/3%EF%B8%8F%E2%83%A3_Motors_%26_TB6612-4ECCA3?style=for-the-badge&logoColor=black)](#-3-motor--tb6612fng-driver-issues)
[![Wi-Fi & WebApp](https://img.shields.io/badge/4%EF%B8%8F%E2%83%A3_Wi--Fi_%26_WebApp-9B5DE5?style=for-the-badge)](#-4-wi-fi--web-application-issues)
[![ESP32-CAM](https://img.shields.io/badge/5%EF%B8%8F%E2%83%A3_ESP32--CAM_Video-F15BB5?style=for-the-badge)](#-5-esp32-cam-video-satellite-issues)
[![Sensors](https://img.shields.io/badge/6%EF%B8%8F%E2%83%A3_Thermal_%26_Mic-00BBF9?style=for-the-badge)](#-6-sensor-issues-mlx90640--inmp441)
[![Brownouts](https://img.shields.io/badge/7%EF%B8%8F%E2%83%A3_Power_%26_Brownouts-FF9F1C?style=for-the-badge&logoColor=black)](#-7-power-resets--brownouts-robot-reboots-when-moving)
[![21 Serial Decoders](https://img.shields.io/badge/%F0%9F%94%8D_9%EF%B8%8F%E2%83%A3_21_SERIAL_ERROR_DECODERS-00F5D4?style=for-the-badge&logoColor=black)](#-9-serial-monitor-error-message-decoder-what-if-i-see-this)

</div>

---

## ⚡ The 30-Second Quick-Fix Drill

> [!TIP]
> **Before changing any wires or code**, run this 4-step reset sequence—it resolves over **80% of field hiccups** immediately:

1. 🔌 **Unplug USB-C**, wait 3 seconds, then **plug it firmly back in**.
2. 🔘 **Press and release the `RST` (Reset / EN) button ONCE** on the ESP32-S3 (do *not* hold `BOOT`).
3. 📱 **Turn OFF Mobile / Cellular Data** on your smartphone (prevents your phone from bypassing the offline rover Wi-Fi AP).
4. 🔋 **Check the Motor Battery Switch:** Ensure the `VM` motor battery ($6\text{V}–12\text{V}$) is switched **ON** and shares a **Common GND** with the ESP32-S3.

---

## 🔴 1. ESP32-S3 Boot & Flashing Issues

### 🔸 Problem: Stuck in Download Mode (`waiting for download` / `boot:0x0 (DOWNLOAD)`)
* **Symptom:** Serial Monitor prints `rst:0x1 (POWERON),boot:0x0 (DOWNLOAD(USB/UART0/1/...))` and your rover code never starts.
* **Why it happens:** The ESP32-S3 enters ROM bootloader mode whenever `GPIO 0` is held `LOW` during power-up or reset (e.g., the `BOOT` button is stuck or a wire is grounding `GPIO 0`).
* **Instant Fix:**
  1. Unplug the USB-C cable.
  2. Check that nothing is pressing the physical **`BOOT` button** and no wires touch `GPIO 0`.
  3. Plug USB-C back in and press the **`RST` button ONCE**.

### 🔸 Problem: Arduino IDE Fails to Upload (`Failed to connect to ESP32-S3`)
* **Fix (Manual Bootloader Entry Combo):**
  1. Hold down the **`BOOT` button** on the ESP32-S3.
  2. While holding `BOOT`, tap and release the **`RST` button** once.
  3. Release the **`BOOT` button**.
  4. Click **Upload** in Arduino IDE.
  5. After upload completes (`Hard resetting via RTS pin...`), tap **`RST` once** to launch the firmware.

### 🔸 Problem: COM Port Does Not Appear in Arduino IDE
* **Causes & Fixes:**
  * 🔴 **Charge-only USB cable:** Swap to a verified USB-C **data sync cable**.
  * 🟡 **Dual USB-C ports on DevKit:** Most ESP32-S3 boards have two ports—one labeled **`USB`** (Native USB) and one labeled **`UART`** (CH343/CP2102). Plug into **`USB`** when using `USB CDC On Boot: Enabled`.

---

## 🖥️ 2. Serial Monitor Issues (Blank Screen / No Output)

> [!CAUTION]
> **THE #1 REASON FOR A BLANK SERIAL MONITOR ON ESP32-S3:**
> By default, Arduino IDE sets **USB CDC On Boot** to `Disabled`. When disabled, all `Serial.println()` output is sent to hardware pins `GPIO 43/44` instead of your USB-C cable!

### 🔸 Problem: Serial Monitor is 100% Blank at 115200 Baud
* **Instant Fix:**
  1. In Arduino IDE, open the top menu: **Tools → USB CDC On Boot**.
  2. Change it to 🟢 **`Enabled`**.
  3. Click **Upload** to re-flash the board.
  4. Open Serial Monitor (`115200 baud`) and press **`RST` once** on the ESP32-S3.

### 🔸 Problem: Port Busy or `Access Denied` Error During Upload
* **Fix:** Close the Serial Monitor tab (and any external terminal apps like PuTTY or Python scripts) before clicking **Upload**.

---

## 🛞 3. Motor & TB6612FNG Driver Issues

### 🔸 Problem: Motors are Stiff / Hard to Turn by Hand When Powered On
* **Symptom:** Wheels do not spin, and when you try to rotate them by hand, they resist strongly and feel "locked".
* **Root Cause — TB6612 Active Short-Brake Mode:**
  * In the TB6612FNG truth table, if `IN1 != IN2` while `PWM = LOW (0V)` (or if `IN1 = HIGH` and `IN2 = HIGH`), the H-bridge **shorts the motor coils together**, acting as a magnetic brake!
  * Only `IN1 = LOW, IN2 = LOW, PWM = LOW` places the driver in **High-Z Neutral Coast** mode.
* **Instant Fix:**
  1. Upload our direct-drive firmware: `stopMotors()` drives all 18 motor pins `LOW` (High-Z Coast), and driving uses pure `digitalWrite(HIGH)` full gate drive.
  2. With power ON and rover idle, verify all 6 wheels spin freely by hand.

### 🔸 Problem: Motors Spin Freely by Hand, But Never Move When Commanded
* **Check 1 — `STBY` (Standby) Pin:** All 3 TB6612FNG `STBY` pins **must be wired to `3.3V`**. If floating or `0V`, all drivers remain shut down.
* **Check 2 — `VM` Motor Battery Rail:** Logic runs on `VCC` (`3.3V`), while motors run on `VM` ($6\text{V}–12\text{V}$ battery). Verify the battery switch is ON and charged.
* **Check 3 — Common Ground:** Battery negative (`-`) **must connect to ESP32-S3 `GND`**.

### 🔸 Problem: Left Side (`Motor A`) Spins Backwards
* **Instant Fix:**
  * Our firmware defaults `polarityA = -1` (physically inverted out of the box).
  * You can also tap **`MOT A: REV`** or **`MOT B: NORM`** directly on the GameBoy WebApp HUD to flip either side instantly!

### 🧪 Built-In 6-Second Individual Wheel Diagnostic (`/test_motors`)
Open **`http://192.168.4.1/test_motors`** in your browser. The rover will pulse each wheel for 1 second in order:
`FL (1s)` $\rightarrow$ `FR (1s)` $\rightarrow$ `ML (1s)` $\rightarrow$ `MR (1s)` $\rightarrow$ `RL (1s)` $\rightarrow$ `RR (1s)` while printing progress to the Serial Monitor.

---

## 📶 4. Wi-Fi & Web Application Issues

> [!IMPORTANT]
> **Smartphone Cellular Data Bypass:** When your phone connects to `RubbleRover-Tactical-AP`, it detects *"No Internet"* and silently routes browser traffic over **4G/5G Mobile Data**, causing `http://192.168.4.1` to time out!

### 🔸 Problem: Connected to Wi-Fi, But `http://192.168.4.1` Says "Site Can't Be Reached"
* **Instant Fix:**
  1. **Turn OFF Mobile / Cellular Data** on your phone.
  2. If prompted *"This network has no internet, stay connected?"*, tap **YES / Keep Connection**.
  3. Type **`http://192.168.4.1`** explicitly (use `http://`, **not** `https://`).

---

## 📷 5. ESP32-CAM Video Satellite Issues

### 🔸 Problem: WebApp Shows `CAM OFFLINE` or Black Video Screen
* **Check 1 — Remove Flashing Jumper:** You **MUST remove the `GPIO 0` to `GND` jumper wire** after uploading to the ESP32-CAM, then press `RST`.
* **Check 2 — Power Supply (`5V` Rail):** Power the ESP32-CAM from a solid **5V supply** ($\ge 500\text{mA}$), not a weak 3.3V pin.
* **Check 3 — Edition 1 vs Edition 2 IP:**
  * **Edition 1:** Camera connects to S3 AP at **`http://192.168.4.2/stream`**.
  * **Edition 2 (`v2`):** Camera broadcasts its own AP (`RubbleRover-CAM-AP`) at **`http://192.168.5.1`** AND joins your Wi-Fi Router. Click `CAM IP: ... (SET)` on the S3 HUD to enter the camera's IP.

---

## 🌡️ 6. Sensor Issues (MLX90640 & INMP441)

| Sensor | Symptom | Root Cause & Instant Fix |
| :--- | :--- | :--- |
| 🔥 **MLX90640 Thermal** | Heatmap shows `OFFLINE` | Check `SCL = GPIO 1`, `SDA = GPIO 2`, power from **3.3V only**, and add $4.7\text{k}\Omega$ pull-up resistors to 3.3V. |
| 🎙️ **INMP441 Microphone** | VU Meter stuck at `0%` | Tie the **`L/R` pin directly to `GND`**. Check `SCK = 42`, `WS = 41`, `SD = 40`, and power from **3.3V only**. |

---

## 🔋 7. Power Resets & Brownouts (Robot Reboots When Moving)

### 🔸 Problem: ESP32-S3 Reboots the Instant You Press Forward
* **Root Cause:** 6 DC gearmotors draw a $2\text{A}–4\text{A}$ startup inrush spike. If the ESP32 shares an unfiltered battery line, voltage dips below $2.8\text{V}$ and trips the hardware Brownout Detector.
* **Instant Fixes:**
  1. Solder a **$1000\,\mu\text{F}$ ($16\text{V}+$) electrolytic capacitor** across `VM` and `GND` on the motor driver power rail.
  2. Power the ESP32-S3 via a dedicated **5V buck converter** (LM2596 / Mini-360) rather than raw battery leads.

---

## 📊 8. Master Summary Matrix: Symptom vs Instant Action

| Symptom | Primary Cause | Instant Action |
| :--- | :--- | :--- |
| 🔴 **Stuck in download mode** | `BOOT` button or `GPIO 0` held LOW | Unplug USB, release button, replug, tap `RST` once |
| 🔴 **Serial monitor blank** | `USB CDC On Boot` is Disabled | Set **Tools → USB CDC On Boot → `Enabled`** & re-flash |
| 🟡 **Motors stiff / locked** | TB6612 in Short-Brake mode | Flash direct-drive firmware (`stopMotors()` sets all pins `LOW`) |
| 🟡 **Motors won't spin at all** | `STBY` floating or `VM` battery off | Tie `STBY` to `3.3V`; turn battery switch ON; tie common `GND` |
| 🟣 **Left wheels spin reverse** | Physical motor polarity | Tap **`MOT A: REV`** on HUD (default `polarityA = -1`) |
| 🔵 **Phone can't open `192.168.4.1`** | Phone cellular data bypass | Turn **OFF Mobile Data** on phone; tap *"Stay Connected"* |
| 🟠 **Camera stream offline** | `GPIO 0` jumper still attached | Remove `GPIO 0` jumper on ESP32-CAM and press `RST` |
| 🟢 **Thermal sensor offline** | Missing I2C pull-ups or wire swap | Verify `SCL=1, SDA=2`; firmware auto-detects hot-plug & swap |
| ⚡ **Rover reboots on drive** | Motor inrush voltage sag | Add $1000\,\mu\text{F}$ capacitor across `VM` battery rail |

---

## 🔍 9. Serial Monitor Error Message Decoder ("What If I See This?")

> [!NOTE]
> Match any message printed in your Arduino IDE Serial Monitor (**`115200 baud`**) to the 21 diagnostic cases below for an exact explanation and fix.

---

### 🔴 Case 1: `waiting for download` or `boot:0x0 (DOWNLOAD...)`
```text
ESP-ROM:esp32s3-20210327
rst:0x1 (POWERON),boot:0x0 (DOWNLOAD(USB/UART0/1/...))
waiting for download
```
* **Meaning:** The ESP32-S3 is trapped in ROM Bootloader mode waiting for firmware upload, so your sketch is not running.
* **Instant Fix:** Unplug USB-C, ensure nothing is holding the `BOOT` button or grounding `GPIO 0`, plug back in, and tap **`RST` once**.

---

### 🔴 Case 2: `Brownout detector was triggered` or `rst:0xc (RTCWDT_RTC_RST)`
```text
Brownout detector was triggered
rst:0xc (RTCWDT_RTC_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```
* **Meaning:** Logic voltage dipped below $2.8\text{V}$ (typically when motors start moving), forcing a hardware reset.
* **Instant Fix:** Power motors strictly from the battery `VM` rail (never from ESP32 `3.3V`/`5V` pins), add a **$1000\,\mu\text{F}$ capacitor** across `VM` & `GND`, and use a separate 5V buck converter for the ESP32.

---

### 🔴 Case 3: `Guru Meditation Error: Core 0 panic'ed (LoadProhibited)`
```text
Guru Meditation Error: Core 0 panic'ed (LoadProhibited). Exception was unhandled.
```
* **Meaning:** Software exception on Core 0 (caused by an unguarded I2C read on a missing sensor or calling `Serial.println` before `Serial.begin`).
* **Instant Fix:** Flash the latest firmware—`Serial.begin(115200)` now runs first and `Wire.setTimeOut(50)` prevents bus hangs.

---

### 🟡 Case 4: Gibberish, Question Marks, or Garbage Characters (`⸮⸮▒▒$x@!`)
```text
⸮⸮▒▒$x@!
```
* **Meaning:** Serial Monitor baud rate mismatch.
* **Instant Fix:** Set the baud rate dropdown in the bottom-right corner of Serial Monitor to **`115200 baud`** and tap `RST`.

---

### 🟡 Case 5: `[I2C] MLX90640 not detected at 0x33 (hot-plug auto-probe active).`
```text
[I2C] Initializing I2C for MLX90640...
[I2C] MLX90640 not detected at 0x33 (hot-plug auto-probe active).
[HEARTBEAT] Clients:0 | MLX:OFFLINE | Mic:ONLINE (Lvl:2%) | IP:192.168.4.1
```
* **Meaning:** Rover booted and drives normally, but MLX90640 did not acknowledge `0x33`.
* **Instant Fix:** Check `SDA = GPIO 2`, `SCL = GPIO 1`, `VIN = 3.3V`, and $4.7\text{k}\Omega$ pull-ups. Once plugged in, the background task will auto-initialize it without a reboot!

---

### 🟢 Case 6: `[I2C] MLX90640 detected on swapped pins (SDA:2, SCL:1)!`
```text
[I2C] MLX90640 detected on swapped pins (SDA:2, SCL:1)!
[MLX90640] MLX90640 Initialized Successfully!
```
* **Meaning:** Your physical SDA and SCL wires were reversed, and the firmware **automatically swapped them in software**!
* **Instant Fix:** **No action needed!** Your thermal camera is 100% online.

---

### 🟡 Case 7: `[AUDIO] WARNING: ESP_I2S init failed.`
```text
[AUDIO] WARNING: ESP_I2S init failed.
```
* **Meaning:** The ESP32 I2S peripheral failed to bind to `GPIO 42, 41, 40`.
* **Instant Fix:** Inspect Header J3 pins (`42, 41, 40`) for solder bridges or shorts to `GND`, and ensure INMP441 `VDD` is wired to `3.3V`.

---

### 🟡 Case 8: `[HEARTBEAT] ... Mic:ONLINE (Lvl:0%)` *(Loudness Stuck at 0%)*
```text
[HEARTBEAT] Clients:0 | MLX:ONLINE | Mic:ONLINE (Lvl:0%) | IP:192.168.4.1
```
* **Meaning:** Mic is initialized, but outputs digital silence because the `L/R` channel pin is floating.
* **Instant Fix:** Solder the INMP441 **`L/R` pin directly to `GND`** (selects Left channel). Clap near the mic to see `Lvl:40%–85%`.

---

### 🟢 Case 9: `[WATCHDOG] Failsafe timeout! Stopping motors.`
```text
[WATCHDOG] Failsafe timeout! Stopping motors.
```
* **Meaning:** Safety watchdog stopped all 6 motors because no drive packet arrived for $>1000\text{ms}$ (e.g., you released the D-pad or closed the browser).
* **Instant Fix:** **Normal safety behavior!** Tap any D-pad button to resume driving immediately.

---

### 🔴 Case 10: ESP32-CAM: `[FATAL] Camera initialization failed!`
```text
[FATAL] Camera initialization failed!
```
* **Meaning:** ESP32-CAM could not initialize the OV2640 camera module.
* **Instant Fix:** Power off, lift the black retaining flap on the camera ribbon connector, reseat the gold ribbon cable squarely, lock the flap down, and verify board is set to **`AI Thinker ESP32-CAM`**.

---

### 🟡 Case 11: ESP32-CAM: `[WIFI-STA] Connecting immediately to Master AP... ............`
```text
[WIFI-STA] Connecting immediately to Master AP: RubbleRover-Tactical-AP (Ch 1)...
................................
[WIFI-STA] S3 not yet online; running background reconnect in loop().
```
* **Meaning:** The camera cannot find the ESP32-S3's Wi-Fi network.
* **Instant Fix:** Power on the **ESP32-S3 Master first** (or use **Edition 2 Firmware** where the camera broadcasts its own independent `RubbleRover-CAM-AP` at `192.168.5.1`).

---

### 🟡 Case 12: `wifi: Station disconnected from AP, reason=202` *(or `reason=15`)*
```text
wifi: Station connected to AP, AID=1
wifi: Station disconnected from AP, reason=202
```
* **Meaning:** Client device failed WPA2 authentication (`202`) or handshake timeout (`15`).
* **Instant Fix:** Check password **`RubbleRover2026`** (case-sensitive). On your phone's Wi-Fi settings for the rover AP, change **MAC Address Type** from *Randomized MAC* to ***Phone / Device MAC***.

---

### 🔴 Case 13: `CORRUPT HEAP: multi_heap.c:xxx ... abort() was called`
```text
CORRUPT HEAP: multi_heap.c:431 detected at 0x3fc94a20
abort() was called at PC 0x40378bc5 on core 0
```
* **Meaning:** Heap memory corruption from dynamic buffer overflow.
* **Instant Fix:** Flash the latest firmware, which uses 100% static pre-allocated arrays (`thermalPixels[768]`, `audioSliceBuffer[4000]`).

---

### 🔴 Case 14: `task_wdt: Task watchdog got triggered... IDLE0 (CPU 0)`
```text
E (10432) task_wdt: Task watchdog got triggered. The following tasks did not feed the watchdog:
E (10432) task_wdt:  - IDLE0 (CPU 0)
```
* **Meaning:** Core 0 was stuck waiting on an unguarded I2C bus without yielding to FreeRTOS IDLE task.
* **Instant Fix:** Latest firmware enforces `Wire.setTimeOut(50)` and `vTaskDelayUntil(..., pdMS_TO_TICKS(60))` on Core 0.

---

### 🔴 Case 15: `rst:0x3 (RTC_SW_SYS_RST)` *(Rapid Reboot Loop)*
```text
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```
* **Meaning:** Software reset loop during `setup()`.
* **Instant Fix:** Hold `BOOT` $\rightarrow$ tap `RST` $\rightarrow$ release `BOOT`, upload the latest sketch with `PSRAM: Disabled`, then tap `RST`.

---

### 🔴 Case 16: ESP32-CAM: `E (xxxx) camera: Camera probe failed with error 0x20002`
```text
E (1254) camera: Camera probe failed with error 0x20002
```
* **Meaning:** `0x20002` (`ESP_ERR_NOT_FOUND`) — OV2640 sensor did not respond on SCCB pins (`GPIO 26/27`).
* **Instant Fix:** **Remove the `GPIO 0` to `GND` flashing jumper wire!** Reseat the camera ribbon cable and press `RST`.

---

### 🟡 Case 17: `[WIFI-AP] Online!` But `[HEARTBEAT] Clients:0` Never Increases
```text
[HEARTBEAT] Clients:0 | MLX:ONLINE | Mic:ONLINE (Lvl:4%) | IP:192.168.4.1
```
* **Meaning:** Your phone disconnected in the background because the rover AP has no internet.
* **Instant Fix:** Turn **OFF Cellular Data** on your phone, reconnect to `RubbleRover-Tactical-AP`, and tap **"Keep Connection"**. `Clients:1` will appear immediately.

---

### 🔴 Case 18: `flash read err, 1000` or `ets_main.c 371`
```text
flash read err, 1000
ets_main.c 371
```
* **Meaning:** SPI Flash mode mismatch in Arduino IDE settings.
* **Instant Fix:** Set **Tools → Flash Mode → `QIO 80MHz`** (or `DIO 80MHz`) and **Tools → PSRAM → `Disabled`**, then re-upload.

---

### 🟢 Case 19: `[HTTP] 404 Not Found: /generate_204` or `/hotspot-detect.html`
```text
[HTTP] 404 Not Found: /generate_204
[HTTP] 404 Not Found: /hotspot-detect.html
```
* **Meaning:** Automatic captive-portal probes from Android/iOS/Windows.
* **Instant Fix:** **100% harmless!** This proves your phone is talking to the ESP32-S3 web server. Open `http://192.168.4.1` in your browser.

---

### 🟡 Case 20: `[LIGHT] Searchlight LED state: 1` But Flash LED Stays Dark
```text
[LIGHT] Searchlight LED state: 1
```
* **Meaning:** Command succeeded, but the ESP32-CAM 5V supply cannot provide the $150\text{mA}$ current needed by the `GPIO 4` floodlight LED.
* **Instant Fix:** Power the ESP32-CAM from a regulated **5V 2A rail** with common `GND`.

---

### 🟡 Case 21: `[DIAG 1/6] Front-Left (Motor A)...` Motor Whines/Buzzes But Doesn't Turn
```text
--- STARTING 6-MOTOR HARDWARE DIAGNOSTIC ---
[DIAG 1/6] Front-Left (Motor A)...
```
* **Meaning:** H-bridge is delivering power, but battery voltage is too low or the gearbox is mechanically bound.
* **Instant Fix:** Charge the motor battery pack ($>7.4\text{V}$), verify wheels turn freely by hand when powered off, and switch the HUD speed gear to **100% TURBO (`255`)**.

---

<div align="center">

[![Return to Top](https://img.shields.io/badge/%E2%AC%86%EF%B8%8F_BACK_TO_TOP-FFD21E?style=for-the-badge&logoColor=black)](#%EF%B8%8F-rubble-rover-troubleshooting--serial-decoder-guide)
[![Back to Main README](https://img.shields.io/badge/%F0%9F%93%96_BACK_TO_MAIN_README-4ECCA3?style=for-the-badge&logoColor=black)](README.md)

</div>
