import sys, os, glob, time, serial, serial.tools.list_ports
import pandas as pd

CSV_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "seed_data_runs.csv")

def print_header():
    print("=" * 105)
    print(f"{'Run':>4} | {'Time(ms)':>8} | {'Condition Name':<20} | {'State':<7} | {'PWM':>4} | {'Curr(mA)':>8} | {'RPM':>5} | {'Supply(mV)':>10} | {'Temp':>4} | {'Action Target':<14}")
    print("=" * 105)

def print_row(run_id, t_ms, cond_name, st, pwm, curr, rpm, mv, temp, action):
    color = "\033[91m" if "STOP" in action else ("\033[93m" if "DERATE" in action else "\033[92m")
    print(f"{color}{run_id:>4} | {t_ms:>8} | {cond_name:<20} | {st:<7} | {pwm:>4} | {curr:>8} | {rpm:>5} | {mv:>10} | {temp:>4.1f} | {action:<14}\033[0m")

def replay_csv():
    print("[*] Replaying data directly from data/seed_data_runs.csv matching run_manifest.csv:")
    print_header()
    df = pd.read_csv(CSV_PATH)
    targets = {0: ("Nominal_12V", "ACCEPT_START"), 1: ("Mild_Sag_10.5V", "DERATE"),
               2: ("Moderate_Sag_9.0V", "DERATE"), 3: ("Severe_Sag_7.5V", "STOP")}
    for r in [101, 109, 117, 125]:
        sub = df[df["run_id"] == r]
        cond_id = sub["condition_id"].iloc[0]
        cname, act = targets.get(cond_id, ("Other", "UNKNOWN"))
        for t in [0, 50, 100, 150, 200, 250]:
            row = sub[sub["time_ms"] == t]
            if not row.empty:
                r0 = row.iloc[0]
                st = "RAMP" if r0["motor_state"] == 2 else ("RUN" if r0["motor_state"] == 3 else "DERATE")
                print_row(r0["run_id"], r0["time_ms"], cname, st, r0["pwm_duty"], r0["current_ma"], r0["speed_rpm"], r0["supply_proxy_mv"], r0["temp_c"], act)
                time.sleep(0.04)
        print("-" * 105)

def main():
    ports = [p.device for p in serial.tools.list_ports.comports() if any(k in (p.description or "").lower() for k in ["stlink", "stm", "0483"])]
    port = ports[0] if ports else (glob.glob("/dev/ttyACM*") + [None])[0]
    
    if len(sys.argv) > 1 and sys.argv[1] == "--replay":
        replay_csv()
        return

    if not port:
        print("[!] No STM32 port found. Showing data from data/seed_data_runs.csv:\n")
        replay_csv()
        return

    print(f"[*] Connected to STM32 on {port} @ 115200 Baud")
    print_header()
    ser = serial.Serial(port, 115200, timeout=1)
    
    while True:
        line = ser.readline().decode("ascii", errors="ignore").strip()
        if not line.startswith("$DAT,") or not line.endswith("*"): continue
        p = line[5:-1].split(",")
        if len(p) < 10: continue
        try:
            run_id, cond_id, t_ms, st_code, pwm, curr, rpm, mv, temp = int(p[0]), int(p[1]), int(p[2]), int(p[3]), int(p[4]), int(p[5]), int(p[6]), int(p[8]), float(p[9])
        except ValueError: continue
        
        # Exact condition and target classification from run_manifest.csv
        if mv >= 11000: cname, act = "Nominal_12V", "ACCEPT_START"
        elif mv >= 9500: cname, act = "Mild_Sag_10.5V", "DERATE"
        elif mv >= 8000: cname, act = "Moderate_Sag_9.0V", "DERATE"
        else:            cname, act = "Severe_Sag_7.5V", "STOP"
        
        st_str = "RUN" if st_code == 3 else ("DERATE" if st_code == 4 else ("STOP" if st_code == 6 else "RAMP"))
        print_row(run_id, t_ms, cname, st_str, pwm, curr, rpm, mv, temp, act)

if __name__ == "__main__":
    main()
