#!/usr/bin/env python3
"""
Supply-Sag Emulator (Variant 02.07) - Real-Time Telemetry & Dataset Matcher
Short, clean Python engine that reads live STM32 serial telemetry (or replays data/seed_data_runs.csv)
and displays output matching the exact columns and values of the dataset.
"""

import sys
import os
import time
import glob
import serial
import serial.tools.list_ports
import pandas as pd
import joblib
import numpy as np

CONDITIONS = {
    0: ("Nominal_12V", "ACCEPT_START"),
    1: ("Mild_Sag_10.5V", "DERATE"),
    2: ("Moderate_Sag_9.0V", "DERATE"),
    3: ("Severe_Sag_7.5V", "STOP"),
    4: ("High_Inertia_Confuser", "RETRY_SAFE_PROFILE"),
    5: ("Warm_Start_Nominal", "ACCEPT_START")
}

STATE_NAMES = {
    0: "REST",
    1: "ARMED",
    2: "RAMP",
    3: "RUNNING",
    4: "DERATED",
    5: "RETRY",
    6: "STOPPED"
}

def print_header():
    print("=" * 110)
    print(f"{'Run':>4} | {'Time(ms)':>8} | {'Condition Name':<20} | {'State':<7} | {'PWM':>4} | {'Curr(mA)':>8} | {'RPM':>5} | {'Supply(mV)':>10} | {'Temp':>5} | {'ML Action Target':<16}")
    print("=" * 110)

def print_row(run_id, t_ms, cond_name, st_name, pwm, curr, rpm, mv, temp, action):
    # Color coding
    if "STOP" in action:
        color = "\033[91m" # Red
    elif "DERATE" in action:
        color = "\033[93m" # Yellow
    else:
        color = "\033[92m" # Green
    reset = "\033[0m"

    print(f"{color}{run_id:>4} | {t_ms:>8} | {cond_name:<20} | {st_name:<7} | {pwm:>4} | {curr:>8} | {rpm:>5} | {mv:>10} | {temp:>5.1f} | {action:<16}{reset}")

def run_csv_replay(csv_path="data/seed_data_runs.csv"):
    print("\n[*] Replaying data directly from: " + csv_path)
    print("[*] Displaying rows matching the exact dataset schema...\n")
    df = pd.read_csv(csv_path)
    print_header()
    
    # Pick sample runs from each condition
    sample_runs = [101, 109, 117, 125, 133]
    for r in sample_runs:
        sub = df[df["run_id"] == r]
        cond_id = sub["condition_id"].iloc[0]
        cond_name, target = CONDITIONS.get(cond_id, ("Unknown", "UNKNOWN"))
        # Show key points: start, 100ms, 250ms
        for t in [0, 50, 100, 150, 200, 250]:
            row = sub[sub["time_ms"] == t]
            if not row.empty:
                r0 = row.iloc[0]
                st = STATE_NAMES.get(int(r0["motor_state"]), "RUN")
                print_row(r0["run_id"], r0["time_ms"], cond_name, st, r0["pwm_duty"],
                          r0["current_ma"], r0["speed_rpm"], r0["supply_proxy_mv"], r0["temp_c"], target)
                time.sleep(0.04)
        print("-" * 110)

def run_live_serial(port):
    print(f"\n[*] Listening on {port} @ 115200 Baud...")
    ser = serial.Serial(port, 115200, timeout=1.0)
    print_header()
    while True:
        line = ser.readline().decode("ascii", errors="ignore").strip()
        if not line or not line.startswith("$DAT,") or not line.endswith("*"):
            continue
        parts = line[5:-1].split(",")
        if len(parts) < 10:
            continue
        try:
            run_id = int(parts[0])
            cond_id = int(parts[1])
            t_ms = int(parts[2])
            st = STATE_NAMES.get(int(parts[3]), "RUN")
            pwm = int(parts[4])
            curr = int(parts[5])
            rpm = int(parts[6])
            mv = int(parts[8])
            temp = float(parts[9])
        except ValueError:
            continue
            
        # Determine Condition & Target matching run_manifest.csv
        if mv >= 11000:
            cond_name, target = "Nominal_12V", "ACCEPT_START"
        elif mv >= 9500:
            cond_name, target = "Mild_Sag_10.5V", "DERATE"
        elif mv >= 8000:
            cond_name, target = "Moderate_Sag_9.0V", "DERATE"
        else:
            cond_name, target = "Severe_Sag_7.5V", "STOP"

        print_row(run_id, t_ms, cond_name, st, pwm, curr, rpm, mv, temp, target)

if __name__ == "__main__":
    ports = [p.device for p in serial.tools.list_ports.comports() if any(k in (p.description or "").lower() for k in ["stlink", "stm", "0483"])]
    target_port = ports[0] if ports else (glob.glob("/dev/ttyACM*") + [None])[0]
    
    if len(sys.argv) > 1 and sys.argv[1] == "--demo":
        run_csv_replay()
    elif target_port:
        try:
            run_live_serial(target_port)
        except Exception as e:
            print(f"[!] Could not read serial ({e}). Falling back to CSV dataset display:")
            run_csv_replay()
    else:
        print("[!] No STM32 device found on COM port. Displaying CSV dataset data:")
        run_csv_replay()
