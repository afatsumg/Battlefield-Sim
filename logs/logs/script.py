import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

def run_performance_analysis(csv_path):
    if not os.path.exists(csv_path):
        print(f"Error: Statistics file not found at {csv_path}")
        return

    # Load the fusion report
    df = pd.read_csv(csv_path)
    
    # Filter out initialization frames (where error is 0 or no UAV ground truth yet)
    df = df[df['error_m'] > 0.1]

    # Calculate Metrics
    avg_error = df['error_m'].mean()
    median_error = df['error_m'].median()
    std_dev = df['error_m'].std()
    rmse = np.sqrt((df['error_m']**2).mean())

    print("=" * 40)
    print(" FUSION SERVICE PERFORMANCE REPORT ")
    print("=" * 40)
    print(f"Mean Absolute Error (MAE): {avg_error:.2f} m")
    print(f"Root Mean Square Error (RMSE): {rmse:.2f} m")
    print(f"Standard Deviation:        {std_dev:.2f} m")
    print(f"Median Error:              {median_error:.2f} m")
    print(f"Max Peak Error:            {df['error_m'].max():.2f} m")
    print(f"Total Samples Analyzed:    {len(df)}")
    print("=" * 40)

    # Plotting
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

    # Plot 1: Error over Time
    ax1.plot(df['error_m'], color='#2ecc71', label='Tracking Error (m)', linewidth=1.5)
    ax1.axhline(y=avg_error, color='#e74c3c', linestyle='--', label=f'Mean: {avg_error:.2f}m')
    ax1.set_title('Real-time Fusion Error (Distance from Ground Truth)', fontsize=14)
    ax1.set_xlabel('Sample Index (10Hz)')
    ax1.set_ylabel('Error in Meters')
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # Plot 2: Scatter of Fused vs UAV
    ax2.scatter(df['uav_lon'], df['uav_lat'], color='blue', s=2, label='Ground Truth (UAV)', alpha=0.5)
    ax2.scatter(df['f_lon'], df['f_lat'], color='red', s=2, label='Fused Track (Kalman)', alpha=0.5)
    ax2.set_title('Spatial Accuracy: Ground Truth vs. Fused Path', fontsize=14)
    ax2.set_xlabel('Longitude')
    ax2.set_ylabel('Latitude')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('performance_metrics.png')
    print("Success: Performance visualization saved as 'performance_metrics.png'")

if __name__ == "__main__":
    # Default path for Docker environment
    log_file = "comparison_report.csv"
    
    # Fallback for local execution
    if not os.path.exists(log_file):
        log_file = "comparison_report.csv"
        
    run_performance_analysis(log_file)