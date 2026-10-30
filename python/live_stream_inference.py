#!/usr/bin/env python3
"""
Supply-Sag Emulator (Variant 02.07) - Real-Time Host Decision Engine
Connects to NUCLEO-F446RE via USB Virtual COM Port (115200 Baud).
Reads 100 Hz ASCII telemetry ($DAT,...), evaluates the first 250 ms startup window
using the trained ML Random Forest model, and transmits advisory commands back to the MCU.
"""

import sys
import os
import glob
import time
import serial
import serial.tools.list_ports
import numpy as np
import joblib

# Ensure script can import host_decision_engine from python/
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from host_decision_engine import SupplySagDecisionEngine

STATE_NAMES = {
    0: "SAFE_REST",
    1: "STARTUP_ARMED",
    2: "STARTUP_RAMPING",
    3: "RUNNING",
    4: "DERATED",
    5: "RETRY_BACKOFF",
    6: "TRIP_SHUTDOWN"
}

def find_stm32_port():
    """Autodetect ST-LINK Virtual COM port."""
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        # Check description / VID:PID for ST-LINK
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if "stlink" in desc or "stm32" in desc or "0483:" in hwid or "st-link" in desc:
            return p.device
    
    # Check standard Linux ACM devices
    acm = glob.glob("/dev/ttyACM*")
    if acm:
        return acm[0]
        
    return None

def main():
    print("=" * 70)
    print("KL UNIVERSITY - ML-ESD HACKATHON")
    print("VARIANT 02.07: LIVE ML DECISION & TELEMETRY ENGINE")
    print("=" * 70)

    # Determine serial port
    port = sys.argv[1] if len(sys.argv) > 1 else find_stm32_port()
    if not port:
        print("\n[!] ST-LINK Virtual COM Port not found automatically.")
        print("    Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"      - {p.device}: {p.description}")
        print("\nUsage: python3 live_stream_inference.py <PORT>")
        print("Example: python3 live_stream_inference.py /dev/ttyACM0 (Linux)")
        print("         python3 live_stream_inference.py COM10 (Windows)")
        sys.exit(1)

    print(f"\n[*] Target Serial Port: {port} @ 115200 Baud")

    # Load ML Model
    model_path = os.path.join(os.path.dirname(__file__), "..", "models", "model_random_forest.joblib")
    if not os.path.exists(model_path):
        model_path = os.path.join(os.path.dirname(__file__), "models", "model_random_forest.joblib")
    
    if not os.path.exists(model_path):
        print(f"[ERROR] Trained model not found at {model_path}!")
        sys.exit(1)

    engine = SupplySagDecisionEngine(model_path=model_path)
    print(f"[*] Loaded ML Model: Random Forest ({model_path})")

    # Open Serial Port
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        ser.reset_input_buffer()
        print(f"[+] Serial connection established on {port}!\n")
    except Exception as e:
        print(f"[ERROR] Failed to open {port}: {e}")
        sys.exit(1)

    print("=" * 70)
    print("AWAITING TELEMETRY STREAM FROM NUCLEO-F446RE (Press Blue Button B1 on board)...")
    print("=" * 70)

    window_samples = []
    current_run_id = None
    window_evaluated = False
    run_start_time_ms = None

    try:
        while True:
            line = ser.readline().decode("ascii", errors="ignore").strip()
            if not line:
                continue

            # Look for ASCII data frame: $DAT,run,cond,time,state,pwm,curr,rpm,pulses,mv,temp,reason*
            if line.startswith("$DAT,") and line.endswith("*"):
                content = line[5:-1]
                parts = content.split(",")
                if len(parts) < 11:
                    continue

                try:
                    run_id = int(parts[0])
                    cond_id = int(parts[1])
                    t_ms = int(parts[2])
                    st_val = int(parts[3])
                    pwm = int(parts[4])
                    curr_ma = int(parts[5])
                    speed_rpm = int(parts[6])
                    pulses = int(parts[7])
                    supply_mv = int(parts[8])
                    temp_c = int(parts[9])
                    reason = int(parts[10])
                except ValueError:
                    continue

                # New run detected (either on boot or Blue Button trial trigger)
                if current_run_id != run_id:
                    current_run_id = run_id
                    window_samples = []
                    window_evaluated = False
                    run_start_time_ms = t_ms
                    print("\n" + "-" * 70)
                    print(f" >>> NEW RUN DETECTED: Run #{run_id} (Cond: {cond_id}) <<<")
                    print(f"{'Time':>6} | {'State':<15} | {'PWM':>4} | {'Current':>8} | {'Speed':>8} | {'Supply':>7} | {'Temp':>4}")
                    print("-" * 70)

                elapsed_trial_ms = t_ms - (run_start_time_ms if run_start_time_ms is not None else t_ms)
                st_name = STATE_NAMES.get(st_val, f"STATE_{st_val}")

                # Print telemetry snapshot
                print(f"{elapsed_trial_ms:>4}ms | {st_name:<15} | {pwm/10:>3.0f}% | {curr_ma:>6}mA | {speed_rpm:>5} RPM | {supply_mv/1000:>5.2f}V | {temp_c:>3}C")

                # Accumulate first 250 ms for ML classification
                sample_dict = {
                    "current_ma": curr_ma,
                    "speed_rpm": speed_rpm,
                    "supply_proxy_mv": supply_mv,
                    "temp_c": temp_c
                }

                if elapsed_trial_ms <= 250 and not window_evaluated:
                    window_samples.append(sample_dict)

                # At 250 ms, run the ML Inference
                if elapsed_trial_ms >= 250 and not window_evaluated and len(window_samples) >= 5:
                    window_evaluated = True
                    decision = engine.evaluate_feature_window(window_samples)
                    pred_v = decision["predicted_supply_v"]
                    rec = decision["recommendation"]
                    conf = decision["confidence_pct"]
                    cmd_pkt = decision["cmd_packet"]

                    print("\n" + "=" * 70)
                    print(f" [ML DECISION ENGINE @ 250ms Transient Window]")
                    print(f"   -> Samples Analyzed: {len(window_samples)}")
                    print(f"   -> Peak Inrush Current: {max(s['current_ma'] for s in window_samples)} mA")
                    print(f"   -> Predicted Bus Supply: {pred_v:.2f} V")
                    print(f"   -> Confidence Level: {conf}%")
                    print(f"   -> Advisory Action: [{rec}]")
                    print(f"   -> Transmitting Command to MCU: {cmd_pkt.strip()}")
                    print("=" * 70 + "\n")

                    # Transmit command back to MCU over USART2
                    ser.write(cmd_pkt.encode("ascii"))
                    ser.flush()

            elif line.startswith("==="):
                print(f"\n[MCU BOOT BANNER] {line}")

    except KeyboardInterrupt:
        print("\n\n[*] Terminating Live Telemetry Stream. Closed.")
        ser.close()

if __name__ == "__main__":
    main()
