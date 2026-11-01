# Rubble Rover Hardware & Firmware Troubleshooting Guide 🛠️

This guide covers step-by-step solutions for common issues encountered during flashing, powering, driving, and operating the Rubble Rover.

---

## ⚡ The 30-Second Quick-Fix Drill
If something is unresponsive or behaving erratically, run this quick sequence before changing any wiring or code:

1. **Unplug USB-C**, wait 3 seconds, then **plug it back in**.
2. **Press and release the `RST` (Reset / EN) button** once on the ESP32-S3.
3. If using a phone to control the rover: **Turn OFF Cellular / Mobile Data** (prevents phone from ignoring the offline rover AP).
4. Check the **Battery Switch**: Ensure the motor battery (VM rail, 6V–12V) is switched ON and shares a common Ground with the ESP32-S3.

---

## 1. ESP32-S3 Boot & Flashing Issues

### Problem: Stuck in Download Mode (`waiting for download` / `boot:0x0 (DOWNLOAD)`)
* **Symptom:** In Serial Monitor you see `rst:0x1 (POWERON),boot:0x0 (DOWNLOAD(USB/UART0/1/...))` and your sketch won't execute.
* **Why it happens:** The ESP32-S3 boots into ROM download mode when `GPIO 0` is pulled LOW during startup (e.g. the `BOOT` button is held down or stuck, or external hardware is pulling GPIO 0 down).
* **Fix:**
  1. Unplug the USB-C cable.
  2. Ensure nothing is touching the physical `BOOT` button on the board, and no wires are connected to GPIO 0.
  3. Re-plug USB-C.
  4. Press the **`RST` button once** (do NOT hold `BOOT`). The board will immediately launch the user application.

### Problem: Arduino IDE Fails to Upload (`A fatal error occurred: Failed to connect to ESP32-S3`)
* **Why it happens:** The PC cannot initiate the automatic serial reset handshake.
* **Fix (Manual Bootloader Entry):**
  1. Hold down the **`BOOT` button** on the ESP32-S3.
  2. While still holding `BOOT`, press and release the **`RST` button** once.
  3. Release the **`BOOT` button**.
  4. In Arduino IDE, click **Upload**.
  5. When you see `Leaving... Hard resetting via RTS pin...`, press the **`RST` button once** to start the firmware.

### Problem: COM Port Does Not Appear or Disappears
* **Causes & Fixes:**
  * **Charge-only USB cable:** Many cheap USB cables lack the D+/D- data lines. Swap to a known good data sync cable.
  * **Loose USB-C connector:** Plug directly into your computer's motherboard port, avoiding unpowered USB hubs.
  * **Driver / Native USB Enumeration:** Hold `BOOT`, plug in the USB-C cable, then release `BOOT`. Windows will detect the chip as `USB JTAG/serial debug unit (COMxx)`.

---

## 2. Serial Monitor Issues (Blank Screen / No Output)

### Problem: Serial Monitor is Completely Blank (Nothing Prints)
* **Symptom:** Sketch uploads with 100% success, baud rate is set to `115200`, but Serial Monitor displays zero characters.
* **CRITICAL CAUSE:** On ESP32-S3 DevKit boards, `Serial` output goes over the native USB CDC peripheral. If **USB CDC On Boot** is disabled, the logs are redirected to pins GPIO 43/44 instead of the USB-C port!
* **Fix:**
  1. In Arduino IDE, go to menu: **Tools → USB CDC On Boot**.
  2. Select **`Enabled`**.
  3. Re-upload the sketch to the board.
  4. After upload finishes, click the **`RST` button once**. Output will start streaming immediately.

### Problem: Port Busy or "Access Denied" Error in Arduino IDE
* **Fix:**
  1. Close the Serial Monitor window (or Serial Plotter) in Arduino IDE.
  2. Close any other terminal programs (PuTTY, Cura, Python scripts) using that COM port.
  3. Click **Upload** again.

---

## 3. Motor & TB6612FNG Driver Issues

### Problem: Motors are Stiff / Hard to Turn by Hand when Powered On
* **Symptom:** Wheels won't turn under control, but when you try to rotate them with your fingers, they resist strongly and feel "locked" or heavy.
* **Root Cause: TB6612 Active Short-Brake Mode.**
  * In the TB6612 truth table:
    * `IN1 != IN2` and `PWM = LOW (0V)` $\rightarrow$ **Short Brake** (terminals shorted to GND).
    * `IN1 = HIGH` and `IN2 = HIGH` $\rightarrow$ **Short Brake**.
    * `IN1 = LOW` and `IN2 = LOW` $\rightarrow$ **Neutral High-Z Coast** (free rotation).
  * If the ESP32-S3 outputs direction signals while PWM is 0V, or if motor pins were left in brake mode, the driver dynamically brakes the motors!
* **Fix:**
  1. Our direct-drive firmware sets `stopMotors()` to write all 18 pins (PWM and DIR) to `LOW`. This forces **High-Z neutral coast**, allowing wheels to spin freely when idle.
  2. Upload the latest `sketch_nov1a.ino` / `RubbleRover_S3_MotorController.ino`.
  3. Test without driving: wheels must turn smoothly by hand with power ON.

### Problem: Motors Turn Freely by Hand, but Don't Move when Commanded
* **Check 1 — TB6612 `STBY` Pin:**
  * The TB6612 has an active-high `STBY` (Standby) pin. If `STBY` is floating or 0V, **all outputs are disabled**.
  * Ensure the `STBY` pin on all 3 motor driver boards is wired to **3.3V**.
* **Check 2 — Motor Battery (VM rail):**
  * TB6612 `VCC` powers the logic (3.3V from ESP32).
  * TB6612 `VM` powers the motors (6V–12V from battery pack).
  * If the battery switch is off, fuse is blown, or battery voltage drops below 5V, logic runs but motors have no power.
* **Check 3 — Common Ground:**
  * The motor battery negative (-) terminal **MUST be connected to the ESP32-S3 GND**. Without a common ground reference, the logic signals cannot trigger the driver FETs.

### Problem: One Motor / Side Runs in Reverse
* **Fix:**
  * TB6612 Channel A (Left wheels) is physically wired in reverse orientation relative to Channel B (Right wheels) on 6WD chassis.
  * Our firmware automatically defaults `polarityA = -1` (inverted) and `polarityB = 1` (normal).
  * You can also toggle the **`MOT A: REV`** or **`MOT B: NORM`** buttons on the WebApp HUD to flip polarity on the fly without touching any wires!

### Built-in 6-Second Motor Diagnostic Tool
You can test each motor individually to isolate wiring or driver issues:
1. Connect your phone or PC to `RubbleRover-Tactical-AP`.
2. In your browser, open:
   ```
   http://192.168.4.1/test_motors
   ```
3. The rover will pulse each wheel for 1 second in sequence:
   * **Second 1:** Front-Left (FL)
   * **Second 2:** Front-Right (FR)
   * **Second 3:** Mid-Left (ML)
   * **Second 4:** Mid-Right (MR)
   * **Second 5:** Rear-Left (RL)
   * **Second 6:** Rear-Right (RR)
4. Check Serial Monitor for real-time diagnostic output:
   ```
   [DIAGNOSTIC] === RUNNING HARDWARE MOTOR TEST ===
   [DIAGNOSTIC] Step 1/6: Front-Left (FL) Pulse...
   [DIAGNOSTIC] Step 2/6: Front-Right (FR) Pulse...
   ...
   [DIAGNOSTIC] === ALL MOTORS TEST COMPLETE ===
   ```

---

## 4. Wi-Fi & Web Application Issues

### Problem: Wi-Fi Network `RubbleRover-Tactical-AP` Does Not Show Up
* **Fix:**
  1. Wait 5–10 seconds after powering on the ESP32-S3 for the Wi-Fi softAP beacon to start.
  2. Ensure power supply delivers at least 500 mA (ESP32 Wi-Fi RF surges can cause brownouts).
  3. Press `RST` once to restart the softAP.

### Problem: Connected to Wi-Fi, but `192.168.4.1` Says "Site Can't Be Reached"
* **Most Common Smartphone Issue: Cellular Data Bypass!**
  * iOS and Android detect that the `RubbleRover-Tactical-AP` has no internet access, and automatically route your browser requests through 4G/5G mobile data instead!
* **Fix:**
  1. **Turn OFF Cellular / Mobile Data** on your phone.
  2. When prompted: *"This Wi-Fi network has no internet connection. Stay connected?"* → Tap **YES / Stay Connected**.
  3. Open your browser and type: `http://192.168.4.1` (ensure it is `http://`, **NOT** `https://`).

---

## 5. ESP32-CAM Video Satellite Issues

### Problem: WebApp Shows "CAM OFFLINE" / Video Frame is Black
* **Check 1 — Remove Programming Jumper:**
  * When flashing the ESP32-CAM, `GPIO 0` is bridged to `GND`.
  * **You MUST remove this jumper wire before powering on the camera**, or it will stay in bootloader mode and never run!
* **Check 2 — Power Supply Brownouts:**
  * The ESP32-CAM draws up to 350 mA peak during Wi-Fi transmission.
  * Power the ESP32-CAM from a solid **5V pin** (do NOT power from a weak 3.3V rail).
* **Check 3 — Boot Sequence:**
  * Power on the **ESP32-S3 Master first** so the Wi-Fi AP is active.
  * Once the AP is active, press **`RST` on the ESP32-CAM**. It will connect to the AP within 300 ms and auto-register at `192.168.4.2`.
* **Direct Camera Test:**
  * In your browser, open `http://192.168.4.2/stream` to view the raw camera feed directly.

---

## 6. Sensor Issues (MLX90640 & INMP441)

### Problem: Thermal Heatmap Shows "OFFLINE" or Static/Frozen
* **Check 1 — I2C Pull-Up Resistors:**
  * The MLX90640 Far-Infrared sensor communicates over I2C at 400 kHz on `SDA (GPIO 2)` and `SCL (GPIO 1)`.
  * If wires are longer than 10 cm, add $2.2\text{k}\Omega - 4.7\text{k}\Omega$ external pull-up resistors from SDA to 3.3V and SCL to 3.3V.
* **Check 2 — Power Voltage:**
  * Power the MLX90640 strictly from **3.3V** (5V can damage the sensor).
* **Check 3 — Wire Order:**
  * Verify that SDA (GPIO 2) and SCL (GPIO 1) are not swapped.

### Problem: INMP441 Microphone Shows 0% Sound Level / No Audio
* **Check 1 — L/R Pin Grounding:**
  * The `L/R` (Channel Select) pin on the INMP441 **MUST be tied directly to GND** to select the Left audio channel. Leaving it floating causes silent/undefined audio output.
* **Check 2 — Pin Mappings:**
  * `SCK (Bit Clock)` → **GPIO 42**
  * `WS (Word Select / LRCK)` → **GPIO 41**
  * `SD (Serial Data Out)` → **GPIO 40**
  * `VDD` → **3.3V** (do NOT connect to 5V).

---

## 7. Power Resets & Brownouts (Robot Reboots When Moving)

### Problem: ESP32-S3 Reboots the Moment You Press Forward
* **Root Cause: Motor Inrush Current Voltage Dip.**
  * When 6 DC motors start turning simultaneously from a dead stop, they draw a temporary current spike of 2A to 4A.
  * If the ESP32-S3 shares the same battery line without adequate filtering, the voltage briefly sags below 2.7V, triggering the ESP32 hardware Brownout Detector.
* **Fixes:**
  1. **Add Bulk Capacitance:** Solder a $470\mu\text{F} - 1000\mu\text{F}$ electrolytic capacitor directly across the battery `VM` input pins of the motor drivers.
  2. **Add Logic Decoupling:** Place a $100\mu\text{F}$ capacitor across the ESP32 5V and GND pins.
  3. **Use a Dedicated Step-Down Converter:** Use an external 5V buck converter (e.g. LM2596 or Mini-360) to provide a regulated 5.0V supply to the ESP32-S3, rather than running it directly off fluctuating battery leads.

---

## 8. Summary Table: Issue vs Instant Action

| Symptom | Primary Cause | Instant Action |
|---|---|---|
| **Stuck in download mode** | `BOOT` button or GPIO 0 held LOW | Unplug, release button, replug, click `RST` once |
| **Serial monitor blank** | USB CDC On Boot is Disabled | Set `Tools → USB CDC On Boot → Enabled` and re-flash |
| **Motors stiff / hard to turn** | TB6612 Short Brake active | Flash latest direct-drive firmware; all pins LOW when stopped |
| **Motors won't spin at all** | `STBY` pin floating or VM battery off | Wire `STBY` to 3.3V; check battery switch & common GND |
| **One side drives reverse** | Motor wiring inverted | Click `MOT A: REV` button on HUD or set `polarityA = -1` |
| **Phone can't open 192.168.4.1** | Cellular data bypass | Turn OFF Mobile Data on phone; click "Stay Connected" |
| **Camera feed offline** | GPIO 0 jumper left on CAM board | Remove GPIO 0 jumper; press `RST` on ESP32-CAM |
| **Thermal sensor offline** | Missing I2C pull-ups or swapped pins | Check SDA=GPIO 2, SCL=GPIO 1; add 4.7k pull-ups |
| **Rover reboots on forward** | Voltage dip from motor inrush | Add 1000µF capacitor across motor battery rail |

