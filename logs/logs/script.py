import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

def load_and_clean(csv_path):
    if not os.path.exists(csv_path):
        raise FileNotFoundError(f"Statistics file not found: {csv_path}")

    df = pd.read_csv(csv_path)

    # Remove initialization frames
    df = df[df['error_m'] > 0.1]
    return df


def compute_metrics(df):
    return {
        "MAE": df['error_m'].mean(),
        "RMSE": np.sqrt((df['error_m'] ** 2).mean()),
        "STD": df['error_m'].std(),
        "MEDIAN": df['error_m'].median(),
        "MAX": df['error_m'].max(),
        "SAMPLES": len(df)
    }


def print_report(title, metrics):
    print("=" * 50)
    print(f" {title}")
    print("=" * 50)
    print(f"Mean Absolute Error (MAE): {metrics['MAE']:.2f} m")
    print(f"Root Mean Square Error:   {metrics['RMSE']:.2f} m")
    print(f"Standard Deviation:      {metrics['STD']:.2f} m")
    print(f"Median Error:            {metrics['MEDIAN']:.2f} m")
    print(f"Max Peak Error:          {metrics['MAX']:.2f} m")
    print(f"Total Samples:           {metrics['SAMPLES']}")
    print("=" * 50)


def compare_datasets(csv_physics_on, csv_physics_off):
    df_on = load_and_clean(csv_physics_on)
    df_off = load_and_clean(csv_physics_off)

    metrics_on = compute_metrics(df_on)
    metrics_off = compute_metrics(df_off)

    print_report("FUSION PERFORMANCE — PHYSICS ENABLED (RCS / 1/R⁴)", metrics_on)
    print_report("FUSION PERFORMANCE — PHYSICS DISABLED", metrics_off)

    # ---------------- PLOTTING ----------------
    plt.figure(figsize=(14, 6))

    plt.plot(df_on['error_m'], label='Physics ON (RCS Enabled)', linewidth=1.5)
    plt.plot(df_off['error_m'], label='Physics OFF', linewidth=1.5, alpha=0.7)

    plt.axhline(metrics_on['MAE'], linestyle='--', label=f"Mean ON: {metrics_on['MAE']:.2f} m")
    plt.axhline(metrics_off['MAE'], linestyle='--', label=f"Mean OFF: {metrics_off['MAE']:.2f} m")

    plt.title("Fusion Error Comparison — Physics-Based Detection Impact", fontsize=14)
    plt.xlabel("Sample Index (10 Hz)")
    plt.ylabel("Tracking Error (meters)")
    plt.legend()
    plt.grid(alpha=0.3)

    plt.tight_layout()
    plt.savefig("physics_vs_no_physics_error_comparison.png")
    print("Saved plot: physics_vs_no_physics_error_comparison.png")

    # ----------- OPTIONAL: Spatial Comparison -----------
    fig, ax = plt.subplots(figsize=(7, 7))

    ax.scatter(df_on['f_lon'], df_on['f_lat'], s=3, label='Fused (Physics ON)', alpha=0.5)
    ax.scatter(df_off['f_lon'], df_off['f_lat'], s=3, label='Fused (Physics OFF)', alpha=0.5)
    ax.scatter(df_on['uav_lon'], df_on['uav_lat'], s=5, label='UAV Ground Truth', alpha=0.6)

    ax.set_title("Spatial Track Comparison")
    ax.set_xlabel("Longitude")
    ax.set_ylabel("Latitude")
    ax.legend()
    ax.grid(alpha=0.3)

    plt.tight_layout()
    plt.savefig("spatial_comparison_physics_vs_no_physics.png")
    print("Saved plot: spatial_comparison_physics_vs_no_physics.png")


if __name__ == "__main__":
    compare_datasets(
        csv_physics_on="comparison_report.csv",
        csv_physics_off="comparison_report_without_physics.csv"
    )
