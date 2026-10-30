import os
import json
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.model_selection import GroupShuffleSplit
from sklearn.preprocessing import StandardScaler
from sklearn.linear_model import Ridge
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score
import joblib

def extract_run_features(df_samples):
    """
    Extracts startup signature features within the first 250 ms window for each run.
    """
    features = []
    for run_id, group in df_samples.groupby("run_id"):
        window = group[group["time_ms"] <= 250]
        
        peak_current = window["current_ma"].max()
        mean_current = window["current_ma"].mean()
        current_integral = window["current_ma"].sum() * 0.010 # A*s
        
        # Acceleration slope in first 200 ms
        w200 = window[window["time_ms"] <= 200]
        speed_slope = (w200["speed_rpm"].iloc[-1] - w200["speed_rpm"].iloc[0]) / 0.200 if len(w200) > 1 else 0.0
        
        # Supply proxy voltage metrics
        supply_init = window["supply_proxy_mv"].iloc[0]
        supply_min = window["supply_proxy_mv"].min()
        supply_mean = window["supply_proxy_mv"].mean()
        supply_sag_dip = supply_init - supply_min
        
        # Time to reach 100 RPM
        t_100_rows = group[group["speed_rpm"] >= 100]
        time_to_100_rpm = t_100_rows["time_ms"].iloc[0] if len(t_100_rows) > 0 else 600.0
        
        temp_init = window["temp_c"].iloc[0]
        cond_id = group["condition_id"].iloc[0]
        cond_name = group["condition_name"].iloc[0]
        true_supply = group["true_supply_v"].iloc[0]
        severity = group["severity"].iloc[0]

        features.append({
            "run_id": run_id,
            "condition_id": cond_id,
            "condition_name": cond_name,
            "severity": severity,
            "peak_current_ma": peak_current,
            "mean_current_ma": mean_current,
            "current_integral": current_integral,
            "speed_slope": speed_slope,
            "supply_init_mv": supply_init,
            "supply_min_mv": supply_min,
            "supply_mean_mv": supply_mean,
            "supply_sag_dip_mv": supply_sag_dip,
            "time_to_100_rpm_ms": time_to_100_rpm,
            "temp_init_c": temp_init,
            "true_supply_v": true_supply
        })
    return pd.DataFrame(features)

def train_and_evaluate():
    data_dir = "data"
    plots_dir = "plots"
    models_dir = "models"
    os.makedirs(plots_dir, exist_ok=True)
    os.makedirs(models_dir, exist_ok=True)

    df_samples = pd.read_csv(os.path.join(data_dir, "seed_data_runs.csv"))
    df_feat = extract_run_features(df_samples)

    feature_cols = [
        "peak_current_ma", "mean_current_ma", "current_integral",
        "speed_slope", "supply_init_mv", "supply_min_mv",
        "supply_mean_mv", "supply_sag_dip_mv", "temp_init_c"
    ]

    target_col = "true_supply_v"

    # Strict Run-level train/test split (Stratified by condition)
    # Using 70% train (33 runs), 30% held-out test (15 runs)
    test_runs = []
    for cond_id, grp in df_feat.groupby("condition_id"):
        runs = grp["run_id"].values
        np.random.seed(42 + cond_id)
        selected = np.random.choice(runs, size=max(2, int(len(runs) * 0.3)), replace=False)
        test_runs.extend(selected)

    train_mask = ~df_feat["run_id"].isin(test_runs)
    test_mask = df_feat["run_id"].isin(test_runs)

    X_train = df_feat.loc[train_mask, feature_cols]
    y_train = df_feat.loc[train_mask, target_col]
    X_test = df_feat.loc[test_mask, feature_cols]
    y_test = df_feat.loc[test_mask, target_col]
    test_conditions = df_feat.loc[test_mask, "condition_name"].values

    # Preprocessing: StandardScaler fitted ONLY on training set
    scaler = StandardScaler()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)

    # 1. Parent Baseline Rule
    # Rules: If peak_current > 2400 mA OR time_to_100_rpm > 350 ms -> predict Trip / Poor Voltage (<8V)
    # Otherwise predict nominal (12.0V)
    y_pred_baseline = []
    for idx, row in df_feat.loc[test_mask].iterrows():
        if row["time_to_100_rpm_ms"] > 350:
            y_pred_baseline.append(8.0) # Conservative trip assumption
        elif row["peak_current_ma"] > 2300:
            y_pred_baseline.append(9.5)
        else:
            y_pred_baseline.append(11.8)
    y_pred_baseline = np.array(y_pred_baseline)

    # 2. Whitelist Model 1: Ridge Regressor
    model_ridge = Ridge(alpha=1.0, random_state=42)
    model_ridge.fit(X_train_scaled, y_train)
    y_pred_ridge = model_ridge.predict(X_test_scaled)

    # 3. Whitelist Model 2: Random Forest Regressor
    model_rf = RandomForestRegressor(n_estimators=100, max_depth=5, random_state=42)
    model_rf.fit(X_train, y_train)
    y_pred_rf = model_rf.predict(X_test)

    # Metrics Computation
    metrics = {
        "Baseline": {
            "MAE": round(float(mean_absolute_error(y_test, y_pred_baseline)), 3),
            "RMSE": round(float(np.sqrt(mean_squared_error(y_test, y_pred_baseline))), 3),
            "R2": round(float(r2_score(y_test, y_pred_baseline)), 3)
        },
        "Ridge_Regressor": {
            "MAE": round(float(mean_absolute_error(y_test, y_pred_ridge)), 3),
            "RMSE": round(float(np.sqrt(mean_squared_error(y_test, y_pred_ridge))), 3),
            "R2": round(float(r2_score(y_test, y_pred_ridge)), 3)
        },
        "Random_Forest": {
            "MAE": round(float(mean_absolute_error(y_test, y_pred_rf)), 3),
            "RMSE": round(float(np.sqrt(mean_squared_error(y_test, y_pred_rf))), 3),
            "R2": round(float(r2_score(y_test, y_pred_rf)), 3)
        }
    }

    # Evaluate Operational Action Accuracy (Safe/Unsafe Decision Mapping):
    # Safe condition: Supply >= 9.5V. Unsafe: Supply < 9.5V
    def classify_action(v_pred):
        if v_pred >= 11.0: return "ACCEPT_START"
        elif v_pred >= 8.5: return "DERATE"
        else: return "STOP"

    true_actions = [classify_action(v) for v in y_test]
    baseline_actions = [classify_action(v) for v in y_pred_baseline]
    rf_actions = [classify_action(v) for v in y_pred_rf]

    # Calculate Missed Unsafe and Unnecessary Conservative actions
    # Missed Unsafe: True is STOP, predicted is ACCEPT_START or DERATE
    # Unnecessary Conservative: True is ACCEPT_START, predicted is DERATE or STOP
    def calc_action_safety(true_acts, pred_acts):
        missed_unsafe = sum(1 for t, p in zip(true_acts, pred_acts) if t == "STOP" and p != "STOP")
        unnecessary_cons = sum(1 for t, p in zip(true_acts, pred_acts) if t == "ACCEPT_START" and p != "ACCEPT_START")
        return missed_unsafe, unnecessary_cons

    base_mu, base_uc = calc_action_safety(true_actions, baseline_actions)
    rf_mu, rf_uc = calc_action_safety(true_actions, rf_actions)

    metrics["Baseline"]["Missed_Unsafe"] = base_mu
    metrics["Baseline"]["Unnecessary_Conservative"] = base_uc
    metrics["Random_Forest"]["Missed_Unsafe"] = rf_mu
    metrics["Random_Forest"]["Unnecessary_Conservative"] = rf_uc

    # Save models and scaler
    joblib.dump(model_ridge, os.path.join(models_dir, "model_ridge_regressor.joblib"))
    joblib.dump(model_rf, os.path.join(models_dir, "model_random_forest.joblib"))
    joblib.dump(scaler, os.path.join(models_dir, "model_scaler.joblib"))
    with open(os.path.join(models_dir, "evaluation_metrics.json"), "w") as f:
        json.dump(metrics, f, indent=4)

    # Plot 1: Predicted vs Measured Plot
    plt.figure(figsize=(8, 6), dpi=300)
    plt.scatter(y_test, y_pred_baseline, color="gray", alpha=0.6, label=f"Parent Baseline (RMSE={metrics['Baseline']['RMSE']} V)", marker="s")
    plt.scatter(y_test, y_pred_ridge, color="blue", alpha=0.7, label=f"Ridge Regressor (RMSE={metrics['Ridge_Regressor']['RMSE']} V)", marker="^")
    plt.scatter(y_test, y_pred_rf, color="green", alpha=0.85, label=f"Random Forest (RMSE={metrics['Random_Forest']['RMSE']} V)", marker="o")
    
    # Parity Line
    min_v, max_v = 7.0, 13.0
    plt.plot([min_v, max_v], [min_v, max_v], "r--", linewidth=1.5, label="Perfect Estimation Line")
    plt.xlabel("Measured Supply Voltage (V)", fontsize=11, fontweight="bold")
    plt.ylabel("Predicted Supply Voltage (V)", fontsize=11, fontweight="bold")
    plt.title("Variant 02.07: Predicted vs Measured Supply Voltage Across Held-out Runs", fontsize=12, fontweight="bold")
    plt.grid(True, linestyle=":", alpha=0.6)
    plt.legend(loc="upper left")
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, "predicted_vs_measured_supply.png"))
    plt.close()

    # Plot 2: Residuals by Operating Condition
    residuals_rf = y_test - y_pred_rf
    res_df = pd.DataFrame({
        "Condition": test_conditions,
        "Residual_V": residuals_rf
    })
    plt.figure(figsize=(9, 5), dpi=300)
    box_data = [res_df[res_df["Condition"] == cond]["Residual_V"].values for cond in np.unique(test_conditions)]
    plt.boxplot(box_data, tick_labels=np.unique(test_conditions), patch_artist=True,
                boxprops=dict(facecolor="#c8e6c9", color="#2e7d32"),
                medianprops=dict(color="#1b5e20", linewidth=2))
    plt.axhline(0, color="red", linestyle="--", alpha=0.7)
    plt.ylabel("Residual Error [True - Predicted] (V)", fontsize=11, fontweight="bold")
    plt.title("Variant 02.07: Supply Prediction Residuals by Operating Condition", fontsize=12, fontweight="bold")
    plt.xticks(rotation=20, ha="right", fontsize=9)
    plt.grid(True, linestyle=":", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, "residuals_by_operating_condition.png"))
    plt.close()

    # Plot 3: Synchronized Startup Telemetry (Nominal vs Sagged vs Confuser)
    plot_synchronized_telemetry(df_samples, plots_dir)

    print("Model Training & Evaluation Complete:")
    print(json.dumps(metrics, indent=2))

def plot_synchronized_telemetry(df_samples, plots_dir):
    # Pick 1 nominal run, 1 moderate sag run, 1 high-inertia confuser run
    run_nom = df_samples[df_samples["condition_id"] == 0]["run_id"].iloc[0]
    run_sag = df_samples[df_samples["condition_id"] == 2]["run_id"].iloc[0]
    run_conf = df_samples[df_samples["condition_id"] == 4]["run_id"].iloc[0]

    df_nom = df_samples[df_samples["run_id"] == run_nom]
    df_sag = df_samples[df_samples["run_id"] == run_sag]
    df_conf = df_samples[df_samples["run_id"] == run_conf]

    fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True, dpi=300)

    # 1. Motor Speed
    axes[0].plot(df_nom["time_ms"], df_nom["speed_rpm"], label="Nominal Supply (12V)", color="green", linewidth=2)
    axes[0].plot(df_sag["time_ms"], df_sag["speed_rpm"], label="Supply Sag Target (9V)", color="orange", linewidth=2)
    axes[0].plot(df_conf["time_ms"], df_conf["speed_rpm"], label="Nearest Confuser (High Inertia, 12V)", color="purple", linestyle="--", linewidth=2)
    axes[0].axhline(100, color="gray", linestyle=":", label="100 RPM Threshold")
    axes[0].set_ylabel("Speed (RPM)", fontweight="bold")
    axes[0].set_title("Variant 02.07 - Synchronized Startup Signatures (Nominal vs Sag vs Nearest Confuser)", fontweight="bold")
    axes[0].grid(True, linestyle=":", alpha=0.6)
    axes[0].legend(loc="lower right")

    # 2. Armature Current
    axes[1].plot(df_nom["time_ms"], df_nom["current_ma"], color="green", linewidth=2)
    axes[1].plot(df_sag["time_ms"], df_sag["current_ma"], color="orange", linewidth=2)
    axes[1].plot(df_conf["time_ms"], df_conf["current_ma"], color="purple", linestyle="--", linewidth=2)
    axes[1].axhline(2500, color="red", linestyle=":", label="Hard Safety Trip Limit (2500 mA)")
    axes[1].set_ylabel("Current (mA)", fontweight="bold")
    axes[1].grid(True, linestyle=":", alpha=0.6)
    axes[1].legend(loc="upper right")

    # 3. Supply Proxy Voltage
    axes[2].plot(df_nom["time_ms"], df_nom["supply_proxy_mv"] / 1000.0, color="green", linewidth=2)
    axes[2].plot(df_sag["time_ms"], df_sag["supply_proxy_mv"] / 1000.0, color="orange", linewidth=2)
    axes[2].plot(df_conf["time_ms"], df_conf["supply_proxy_mv"] / 1000.0, color="purple", linestyle="--", linewidth=2)
    axes[2].set_ylabel("Supply Proxy (V)", fontweight="bold")
    axes[2].set_xlabel("Time from Start Command (ms)", fontweight="bold")
    axes[2].grid(True, linestyle=":", alpha=0.6)

    plt.tight_layout()
    plt.savefig(os.path.join(plots_dir, "synchronized_startup_telemetry_2.7.png"))
    plt.close()

if __name__ == "__main__":
    train_and_evaluate()
