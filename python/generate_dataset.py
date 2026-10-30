import numpy as np
import pandas as pd
import os

np.random.seed(42)

def generate_telemetry_runs(output_dir="data", num_runs_per_condition=8):
    os.makedirs(output_dir, exist_ok=True)
    
    # Conditions:
    # 0: Nominal (12.0V, normal load)
    # 1: Mild Sag (10.5V)
    # 2: Moderate Sag (9.0V) [Target Condition]
    # 3: Severe Sag (7.5V)
    # 4: Nearest Confuser: High Inertia / Cold Start (12.0V normal supply, but heavy load/viscous damping)
    # 5: Warm Motor Start (12.0V normal supply, warm motor 38C)
    
    conditions = [
        {"id": 0, "name": "Nominal_12V", "supply_v": 12.0, "inertia_factor": 1.0, "r_motor": 1.2, "temp_base": 24.0, "severity": "NORMAL"},
        {"id": 1, "name": "Mild_Sag_10.5V", "supply_v": 10.5, "inertia_factor": 1.0, "r_motor": 1.2, "temp_base": 25.0, "severity": "MILD_SAG"},
        {"id": 2, "name": "Moderate_Sag_9.0V", "supply_v": 9.0, "inertia_factor": 1.05, "r_motor": 1.25, "temp_base": 26.0, "severity": "MODERATE_SAG"},
        {"id": 3, "name": "Severe_Sag_7.5V", "supply_v": 7.5, "inertia_factor": 1.1, "r_motor": 1.3, "temp_base": 25.0, "severity": "SEVERE_SAG"},
        {"id": 4, "name": "High_Inertia_Confuser", "supply_v": 12.0, "inertia_factor": 2.4, "r_motor": 1.2, "temp_base": 18.0, "severity": "CONFUSER_INERTIA"},
        {"id": 5, "name": "Warm_Start_Nominal", "supply_v": 12.0, "inertia_factor": 0.95, "r_motor": 1.35, "temp_base": 38.0, "severity": "NORMAL_WARM"}
    ]

    all_samples = []
    run_manifest = []
    run_id_counter = 101

    time_steps = np.arange(0, 600, 10) # 0 to 590 ms at 10 ms steps (60 steps)
    dt = 0.010 # 10 ms

    for cond in conditions:
        for r_idx in range(num_runs_per_condition):
            run_id = run_id_counter
            run_id_counter += 1
            
            # Variations per run
            v_supply_nominal = cond["supply_v"] + np.random.normal(0, 0.15)
            # Internal resistance of supply (causes sag drop proportional to current)
            r_supply = 0.35 if cond["id"] in [1, 2, 3] else 0.08
            j_inertia = 0.0008 * cond["inertia_factor"] * (1.0 + np.random.normal(0, 0.03))
            b_viscous = 0.00015 * (1.2 if cond["temp_base"] < 20 else 0.95 if cond["temp_base"] > 30 else 1.0)
            kt_torque = 0.025 # Nm/A
            ke_back_emf = 0.025 # V / (rad/s)
            r_armature = cond["r_motor"] * (1.0 + 0.004 * (cond["temp_base"] - 20.0))
            
            speed_rad_s = 0.0
            encoder_pulses = 0
            temp_c = cond["temp_base"] + np.random.normal(0, 0.5)

            # Metrics tracked for summary
            peak_curr = 0.0
            time_to_100_rpm = -1.0

            for t_ms in time_steps:
                t_sec = t_ms / 1000.0

                # Startup PWM profile: soft ramp in first 150 ms to 80% duty (800)
                if t_ms < 150:
                    pwm_duty = int(200 + (600 * t_ms / 150))
                else:
                    pwm_duty = 800

                pwm_ratio = pwm_duty / 1000.0

                # Back EMF
                v_emf = ke_back_emf * speed_rad_s

                # Current dynamic approximation
                # Terminal voltage = V_supply - I * R_supply
                # V_applied = pwm_ratio * (V_supply - I * R_supply)
                # I = (pwm_ratio * V_supply - V_emf) / (r_armature + pwm_ratio * r_supply)
                ideal_current = (pwm_ratio * v_supply_nominal - v_emf) / (r_armature + pwm_ratio * r_supply + 0.01)
                ideal_current = max(0.0, ideal_current)
                
                # Noise on current sensor
                meas_current_a = ideal_current + np.random.normal(0, 0.025)
                current_ma = int(np.clip(meas_current_a * 1000, 0, 3500))

                # Effective terminal voltage at motor
                effective_v_supply = v_supply_nominal - (meas_current_a * r_supply)
                supply_proxy_mv = int(np.clip(effective_v_supply * 1000 + np.random.normal(0, 20), 4000, 15000))

                # Motor torque and mechanical acceleration
                tau_m = kt_torque * meas_current_a
                tau_load = 0.012 + (b_viscous * speed_rad_s)
                alpha = (tau_m - tau_load) / j_inertia
                speed_rad_s += alpha * dt
                if speed_rad_s < 0:
                    speed_rad_s = 0.0

                speed_rpm = int(speed_rad_s * 60.0 / (2.0 * np.pi))

                # Quadrature pulses (48 PPR)
                delta_pulses = int((speed_rad_s / (2.0 * np.pi)) * 48 * dt + np.random.uniform(0, 0.5))
                encoder_pulses += delta_pulses

                # Thermal model: gentle Joulean rise
                temp_c += (meas_current_a**2 * r_armature * 0.0003) * dt

                # Tracking
                if current_ma > peak_curr:
                    peak_curr = current_ma
                if speed_rpm >= 100 and time_to_100_rpm < 0:
                    time_to_100_rpm = t_ms

                # Motor state
                state_code = 2 if t_ms < 150 else 3 # 2: RAMPING, 3: RUNNING

                all_samples.append({
                    "variant_id": "0x0207",
                    "run_id": run_id,
                    "condition_id": cond["id"],
                    "condition_name": cond["name"],
                    "time_ms": t_ms,
                    "motor_state": state_code,
                    "pwm_duty": pwm_duty,
                    "current_ma": current_ma,
                    "speed_rpm": speed_rpm,
                    "encoder_pulses": encoder_pulses,
                    "supply_proxy_mv": supply_proxy_mv,
                    "temp_c": round(temp_c, 1),
                    "true_supply_v": round(v_supply_nominal, 2),
                    "severity": cond["severity"]
                })

            if time_to_100_rpm < 0:
                time_to_100_rpm = 600.0

            run_manifest.append({
                "run_id": run_id,
                "condition_id": cond["id"],
                "condition_name": cond["name"],
                "severity": cond["severity"],
                "nominal_supply_v": cond["supply_v"],
                "measured_peak_current_ma": peak_curr,
                "time_to_100_rpm_ms": time_to_100_rpm,
                "final_speed_rpm": speed_rpm,
                "final_temp_c": round(temp_c, 1),
                "operator_target": "ACCEPT_START" if cond["id"] in [0, 5] else ("DERATE" if cond["id"] in [1, 2] else ("STOP" if cond["id"] == 3 else "RETRY_SAFE_PROFILE"))
            })

    df_samples = pd.DataFrame(all_samples)
    df_manifest = pd.DataFrame(run_manifest)

    samples_path = os.path.join(output_dir, "seed_data_runs.csv")
    manifest_path = os.path.join(output_dir, "run_manifest.csv")

    df_samples.to_csv(samples_path, index=False)
    df_manifest.to_csv(manifest_path, index=False)

    print(f"Generated {len(df_samples)} telemetry records across {len(df_manifest)} runs.")
    print(f"Saved dataset to {samples_path} and manifest to {manifest_path}")

if __name__ == "__main__":
    generate_telemetry_runs("data")
