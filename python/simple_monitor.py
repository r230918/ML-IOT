import sys, glob, serial, serial.tools.list_ports

def get_port():
    ports = [p.device for p in serial.tools.list_ports.comports() if "stlink" in (p.description or "").lower() or "stm" in (p.description or "").lower() or "0483" in (p.hwid or "")]
    if ports: return ports[0]
    acm = glob.glob("/dev/ttyACM*")
    return acm[0] if acm else (sys.argv[1] if len(sys.argv) > 1 else None)

port = get_port()
if not port:
    print("[!] No STM32 port found. Plug in USB or run: python simple_monitor.py COM3 (or /dev/ttyACM0)")
    sys.exit(1)

print(f"[*] Connected to STM32 on {port} @ 115200 Baud")
print("=" * 65)

ser = serial.Serial(port, 115200, timeout=1)
while True:
    line = ser.readline().decode("ascii", errors="ignore").strip()
    if not line: continue
    
    # Check if line contains SAG or NORMAL
    if "[SAG DETECTED]" in line:
        print(f"\033[91m{line}\033[0m") # Red alert
    elif "[NORMAL" in line:
        print(f"\033[92m{line}\033[0m") # Green normal
    elif line.startswith("$DAT,"):
        parts = line[5:-1].split(",")
        if len(parts) >= 10:
            v_mv, c_ma = int(parts[8]), int(parts[5])
            status = "CRITICAL SAG" if v_mv < 9500 else "NORMAL 12V"
            print(f" -> ML Telemetry: Supply = {v_mv/1000:4.1f}V | Current = {c_ma:4d}mA | Status: {status}")
