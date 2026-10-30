import time
import json
import joblib
import numpy as np

class SupplySagDecisionEngine:
    def __init__(self, model_path="../models/model_random_forest.joblib"):
        self.model = joblib.load(model_path)
        self.model_version = "v1.2-rf"
        self.cmd_seq = 0

    def evaluate_feature_window(self, window_samples):
        """
        window_samples: list of dicts collected in first 250 ms:
        [{'current_ma', 'speed_rpm', 'supply_proxy_mv', 'temp_c', ...}]
        """
        self.cmd_seq += 1
        currents = [s["current_ma"] for s in window_samples]
        speeds = [s["speed_rpm"] for s in window_samples]
        supplies = [s["supply_proxy_mv"] for s in window_samples]
        temps = [s["temp_c"] for s in window_samples]

        peak_current_ma = max(currents)
        mean_current_ma = float(np.mean(currents))
        current_integral = float(np.sum(currents) * 0.010)

        # Speed slope in first 200 ms
        speed_slope = (speeds[-1] - speeds[0]) / 0.200 if len(speeds) > 1 else 0.0

        supply_init_mv = supplies[0]
        supply_min_mv = min(supplies)
        supply_mean_mv = float(np.mean(supplies))
        supply_sag_dip_mv = supply_init_mv - supply_min_mv
        temp_init_c = temps[0]

        features = np.array([[
            peak_current_ma, mean_current_ma, current_integral,
            speed_slope, supply_init_mv, supply_min_mv,
            supply_mean_mv, supply_sag_dip_mv, temp_init_c
        ]])

        predicted_v = float(self.model.predict(features)[0])

        # Map predicted supply voltage to graded safe actions
        # Accept start: healthy supply >= 11.0 V
        # Derate: moderate sag 8.5 V to 11.0 V
        # Stop: severe sag < 8.5 V
        if predicted_v >= 11.0:
            rec = "ACCEPT_START"
            conf = int(min(99, max(75, (predicted_v / 12.0) * 95)))
        elif predicted_v >= 8.5:
            rec = "DERATE"
            conf = int(min(98, max(80, 90)))
        else:
            rec = "STOP"
            conf = 98

        expiry_ms = 400

        # Output packet string matching MCU format:
        # "CMD,<ACTION>,<SEQ>,<CONF>,<EXP>"
        cmd_string = f"CMD,{rec},{self.cmd_seq},{conf},{expiry_ms}\r\n"

        return {
            "recommendation": rec,
            "predicted_supply_v": round(predicted_v, 2),
            "confidence_pct": conf,
            "expiry_ms": expiry_ms,
            "seq": self.cmd_seq,
            "cmd_packet": cmd_string
        }
