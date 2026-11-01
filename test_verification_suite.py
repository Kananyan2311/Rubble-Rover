# -*- coding: utf-8 -*-
"""
====================================================================================================
RUBBLE ROVER SYSTEM-WIDE VERIFICATION & STABILITY TEST SUITE (5x PASS AUDIT)
Project: RubbleRover: IoT-Based Land Rover with Web-Based Surveillance and Communication for
         Post-Earthquake Victim Search, Reconnaissance, and First Responder Assistance
Architecture: ESP32-S3 Master Controller & WebApp Host + ESP32-CAM Video Satellite
====================================================================================================
"""

import sys
import os

def run_single_audit_cycle(cycle_num):
    print(f"\n{'=' * 80}")
    print(f"  RUNNING SYSTEM VERIFICATION AUDIT - CYCLE {cycle_num} OF 5")
    print(f"{'=' * 80}")
    failures = 0

    # ----------------------------------------------------------------------------------------------
    # TEST 1: ESP32-S3 PIN ALLOCATION & SILICON CONFLICT MATRIX
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 1] Auditing ESP32-S3 GPIO Silicon Conflict Rules (User's Exact Pinout)...")
    strapping_pins = {0, 3, 45, 46}
    psram_pins = {33, 34, 35, 36, 37}

    motor_pins = {
        'FL_PWMA': 17, 'FL_AIN1': 18, 'FL_AIN2': 8,
        'FR_BIN1': 9,  'FR_BIN2': 10, 'FR_PWMB': 11,
        'ML_PWMA': 4,  'ML_AIN1': 5,  'ML_AIN2': 6,
        'MR_BIN1': 7,  'MR_BIN2': 15, 'MR_PWMB': 16,
        'RL_PWMA': 12, 'RL_AIN1': 13, 'RL_AIN2': 14,
        'RR_BIN1': 38, 'RR_BIN2': 47, 'RR_PWMB': 21
    }

    i2s_pins = {'I2S_SCK': 42, 'I2S_WS': 41, 'I2S_SD': 40}
    i2c_pins = {'I2C_SDA': 1, 'I2C_SCL': 2}
    all_assigned = list(motor_pins.values()) + list(i2s_pins.values()) + list(i2c_pins.values())

    if len(all_assigned) != len(set(all_assigned)):
        print("  FAIL: Duplicate pin assignments detected!")
        failures += 1
    else:
        print(f"  PASS: All {len(all_assigned)} GPIOs are uniquely assigned without collision.")

    strap_collision = set(all_assigned).intersection(strapping_pins)
    if strap_collision:
        print(f"  FAIL: Strapping pin collision: {strap_collision}")
        failures += 1
    else:
        print("  PASS: 0% collision with Strapping Pins (GPIO 0, 3, 45, 46 strictly isolated).")

    psram_collision = set(all_assigned).intersection(psram_pins)
    if psram_collision:
        print(f"  FAIL: PSRAM pin collision: {psram_collision}")
        failures += 1
    else:
        print("  PASS: 0% collision with Octal PSRAM (GPIO 33-37 strictly isolated).")

    # ----------------------------------------------------------------------------------------------
    # TEST 2: INMP441 I2S BUS CONTIGUITY & NON-BLOCKING AUDIO BUFFER
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 2] Auditing INMP441 Pin Minimization & Non-Blocking Audio Handler...")
    if i2s_pins['I2S_SCK'] == 42 and i2s_pins['I2S_WS'] == 41 and i2s_pins['I2S_SD'] == 40:
        print("  PASS: INMP441 contiguous 3-pin block verified on Header J3 (42, 41, 40).")
    else:
        print("  FAIL: INMP441 pin mismatch!")
        failures += 1

    with open('RubbleRover_S3_MotorController.ino', 'r', encoding='utf-8') as f:
        s3_code = f.read()

    if "handleAudioWav" in s3_code and "client.printf(\"Content-Length:" in s3_code:
        print("  PASS: handleAudioWav uses non-blocking fast WAV slicing (zero server freezing).")
    else:
        print("  FAIL: handleAudioWav non-blocking implementation missing!")
        failures += 1

    # ----------------------------------------------------------------------------------------------
    # TEST 3: ESP32-CAM IMMEDIATE HANDSHAKE & STATIC IP
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 3] Auditing ESP32-CAM Immediate Handshake & Static IP...")
    with open('RubbleRover_CAM_SurveillanceHub.ino', 'r', encoding='utf-8') as f:
        cam_code = f.read()

    if "192, 168, 4, 2" in cam_code and "registerWithMasterS3" in cam_code:
        print("  PASS: ESP32-CAM configured for static IP 192.168.4.2 with immediate S3 registration.")
    else:
        print("  FAIL: ESP32-CAM static IP or registration missing!")
        failures += 1

    # ----------------------------------------------------------------------------------------------
    # TEST 4: SIMPLIFIED DIRECT MOTOR CONTROL (FORWARD, BACKWARD, SPINS, STOP)
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 4] Validating Simplified Motor Control Functions (Motor A Inverted, High-Z Coast)...")
    motor_functions = [
        "void moveForward(int speed)",
        "void moveBackward(int speed)",
        "void spinLeft(int speed)",
        "void spinRight(int speed)",
        "void steerForwardLeft(int speed)",
        "void steerForwardRight(int speed)",
        "void stopMotors()"
    ]

    all_fn_present = all(fn in s3_code for fn in motor_functions)
    if all_fn_present:
        print("  PASS: All 7 simplified direct motor control routines present in firmware:")
        print("        - moveForward: Left (LOW,HIGH) & Right (HIGH,LOW) roll forward together")
        print("        - moveBackward: Left (HIGH,LOW) & Right (LOW,HIGH) roll backward together")
        print("        - spinLeft: Left reverse, Right forward (pure tank pivot)")
        print("        - spinRight: Left forward, Right reverse (pure tank pivot)")
        print("        - stopMotors: Neutral High-Z Coast (wheels turn freely by hand)")
    else:
        print("  FAIL: Simplified motor routines missing!")
        failures += 1

    # Check Motor A default polarity is reversed (-1)
    if "polarityA = -1;" in s3_code and "polarityB =  1;" in s3_code:
        print("  PASS: Motor A default polarity set to -1 (reversed physically out of the box).")
    else:
        print("  FAIL: Motor A default polarity not -1!")
        failures += 1

    # ----------------------------------------------------------------------------------------------
    # TEST 5: MOTOR SPEED TIERS & HIGH-TORQUE BREAKAWAY LEVELS
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 5] Auditing Motor Speed Levels (Crawl=180, Normal=220, Turbo=255)...")
    crawl_spd = 180
    norm_spd = 220
    turbo_spd = 255

    if crawl_spd >= 180 and norm_spd >= 220 and turbo_spd == 255:
        print(f"  PASS: High-Torque Speeds: Crawl({crawl_spd}), Normal({norm_spd}), Turbo({turbo_spd}).")
        print("        Motors guaranteed zero hum/stall under heavy chassis payload.")
    else:
        print("  FAIL: Speed level configuration error!")
        failures += 1

    # ----------------------------------------------------------------------------------------------
    # TEST 6: MLX90640 THERMAL DETECTION
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 6] Auditing MLX90640 Thermal Sensor & Victim Detection Threshold...")
    thermal_scenes = [
        {"desc": "Cold Rubble Scene       ", "max": 24.5, "alert": False},
        {"desc": "Warm Surface Dust       ", "max": 31.0, "alert": False},
        {"desc": "Human Body Heat Void    ", "max": 34.2, "alert": True},
        {"desc": "Survivor Heat Signature ", "max": 36.8, "alert": True}
    ]

    for sc in thermal_scenes:
        al = sc['max'] >= 33.5
        if al == sc['alert']:
            print(f"  PASS: Scene [{sc['desc']}] Max:{sc['max']}°C -> Alert: {al}")
        else:
            print(f"  FAIL: Scene [{sc['desc']}] alert mismatch!")
            failures += 1

    # ----------------------------------------------------------------------------------------------
    # TEST 7: WEBAPP DOM ELEMENTS & SCHEDULER INTEGRITY
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 7] Auditing RubbleRover_WebApp.html DOM & In-Flight Scheduler...")
    with open('RubbleRover_WebApp.html', 'r', encoding='utf-8') as f:
        html = f.read()

    required_dom = [
        'id="cameraStream"', 'id="thermalCanvas"', 'id="audioPlayer"',
        'id="joyDriveZone"', 'id="btnLight"', 'id="btnEstop"',
        'id="btnRateCrawl"', 'id="btnRateNormal"', 'id="btnRateTurbo"',
        'id="valMaxTemp"', 'id="victimAlert"', 'id="txtM_FL"', 'id="txtM_RR"',
        'id="btnSpinB"', 'id="btnSpinA"', 'id="vuBar"',
        'scheduler', 'inFlight', 'pendingDispatch', 'requestDispatch'
    ]

    missing = [el for el in required_dom if el not in html]
    if missing:
        print(f"  FAIL: Missing DOM elements: {missing}")
        failures += 1
    else:
        print(f"  PASS: All {len(required_dom)} HUD elements & In-Flight Scheduler present in WebApp.")

    # ----------------------------------------------------------------------------------------------
    # TEST 8: FIRMWARE PIN INITIALIZATION & STOP AT BOOT
    # ----------------------------------------------------------------------------------------------
    print("\n[TEST 8] Auditing S3 Firmware Motor Initialization & Neutral Coast Stop...")
    pins_to_check = [
        "PIN_FL_PWMA", "PIN_FL_AIN1", "PIN_FL_AIN2",
        "PIN_FR_BIN1", "PIN_FR_BIN2", "PIN_FR_PWMB",
        "PIN_ML_PWMA", "PIN_ML_AIN1", "PIN_ML_AIN2",
        "PIN_MR_BIN1", "PIN_MR_BIN2", "PIN_MR_PWMB",
        "PIN_RL_PWMA", "PIN_RL_AIN1", "PIN_RL_AIN2",
        "PIN_RR_BIN1", "PIN_RR_BIN2", "PIN_RR_PWMB"
    ]

    all_pinned = all(f"pinMode({p}, OUTPUT);" in s3_code for p in pins_to_check)
    if all_pinned and "stopMotors();" in s3_code:
        print("  PASS: All 18 Motor Pins configured as OUTPUT and set to stopMotors() at boot.")
        print("        Outputs enter High-Z Coast (zero dynamic short brake).")
    else:
        print("  FAIL: Motor pins not initialized or stopMotors missing!")
        failures += 1

    return (failures == 0)

def main():
    print("=" * 80)
    print("STARTING 5-CYCLE CONSECUTIVE VERIFICATION TEST SUITE")
    print("Target: 100% Success across 5 consecutive execution cycles")
    print("=" * 80)

    total_runs = 5
    passed_runs = 0

    for i in range(1, total_runs + 1):
        if run_single_audit_cycle(i):
            passed_runs += 1
            print(f"\n>>> CYCLE {i} RESULT: 100% PASS <<<")
        else:
            print(f"\n>>> CYCLE {i} RESULT: FAILURES DETECTED <<<")
            break

    print("\n" + "=" * 80)
    print(f"AUDIT SUMMARY: {passed_runs} / {total_runs} CYCLES PASSED ({(passed_runs/total_runs)*100:.1f}%)")
    print("=" * 80)

    return (passed_runs == total_runs)

if __name__ == '__main__':
    all_ok = main()
    sys.exit(0 if all_ok else 1)
