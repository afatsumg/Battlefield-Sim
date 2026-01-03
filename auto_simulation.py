import subprocess
import os
import shutil
import time
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
# Import our scalable requirements engine
from requirements import FusionValidator

# --- Configuration & Path Mapping ---
BASE_RESULTS_DIR = "./simulation_results"
LOG_SOURCE = "./logs/logs/results.csv"  # Path where C++ writes inside the shared volume
SIM_TIME = "30"                    # Duration in seconds for each test

# --- DO-178C Test Suite Definition ---
# Mapping Test Case IDs to Environmental Variables
TEST_SUITE = {
    "TC-ACC-01": {"name": "nominal_speed_50ms", "env": {"UAV_SPEED": "50.0"}},
    "TC-ACC-02": {"name": "high_speed_250ms", "env": {"UAV_SPEED": "250.0"}},
    "TC-ACC-03": {"name": "interceptor_300ms", "env": {"UAV_SPEED": "300.0"}},
    "TC-GEO-01": {"name": "north_approach", "env": {"UAV_LAT": "39.80", "UAV_HEADING": "0.0"}},
    "TC-GEO-02": {"name": "diagonal_approach", "env": {"UAV_LAT": "39.80", "UAV_HEADING": "45.0"}},
    "TC-GEO-03": {"name": "rainy_weather", "env": {"UAV_LAT": "39.80", "UAV_HEADING": "45.0", "RAIN_RATE_MMH": "5.0"}},
}

class TestExecutive:
    def __init__(self):
        self.validator = FusionValidator()
        self.batch_id = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.batch_path = os.path.join(BASE_RESULTS_DIR, f"batch_{self.batch_id}")
        
        # Create directory structure
        os.makedirs(self.batch_path, exist_ok=True)
        print(f">>> Initializing Test Batch: {self.batch_id}")

    def run_simulation(self, tc_id, config):
        """Executes the docker-compose environment for a specific Test Case."""
        print(f"\n[EXEC] {tc_id}: Starting {config['name']}...")
        
        full_env = os.environ.copy()
        full_env.update(config['env'])
        full_env["SIM_DURATION_SEC"] = SIM_TIME

        # 1. Clean environment (Pre-test cleanup)
        subprocess.run(["docker-compose", "down", "-v"], env=full_env, capture_output=True)
        
        # Clean any residual CSV log files to ensure fresh state
        if os.path.exists(LOG_SOURCE):
            os.remove(LOG_SOURCE)
            print(f"[INFO] Cleared residual log: {LOG_SOURCE}")

        # 2. Launch simulation
        try:
            # -T disables TTY, essential for clean automation
            cmd = ["docker-compose", "up", "--abort-on-container-exit", "--force-recreate", "--exit-code-from", "fusion_service"]
            process = subprocess.Popen(cmd, env=full_env)
            # Add safety margin to the timeout
            process.wait(timeout=int(SIM_TIME) + 20)
        except subprocess.TimeoutExpired:
            print(f"[ERROR] {tc_id} timed out. Terminating...")
            process.terminate()
        except Exception as e:
            print(f"[ERROR] Fatal error during {tc_id}: {e}")

        # 3. Retrieve results
        if os.path.exists(LOG_SOURCE):
            dest_file = f"{tc_id}_{config['name']}.csv"
            dest_path = os.path.join(self.batch_path, dest_file)
            shutil.copy(LOG_SOURCE, dest_path)
            print(f"[INFO] Log captured: {dest_file}")
            return dest_path
        
        print(f"[WARNING] No log found for {tc_id}")
        return None

    def plot_tc_result(self, tc_id, df, verdict, name):
        """Generates visual evidence for the Test Case."""
        # Skip plotting if dataframe is empty or too small
        if len(df) < 10:
            print(f"[WARNING] {tc_id}: Insufficient data for plotting ({len(df)} rows)")
            return
        
        plt.figure(figsize=(12, 6))
        
        # Plot Error over Time (use range instead of index to avoid type issues)
        time_axis = np.arange(len(df)) * 0.1  # 100ms ticks
        plt.plot(time_axis, df['error_m'], label='Fusion Error (m)', color='#1f77b4', linewidth=1.5)
        
        # Draw Threshold Lines from Validator
        plt.axhline(y=25, color='orange', linestyle='--', label='SRID-FUS-001 Limit (25m)')
        plt.axhline(y=100, color='red', linestyle=':', label='SRID-FUS-002 Limit (100m)')

        plt.title(f"Test Case Evidence: {tc_id} ({name})\nVerdict: {verdict['status']}")
        plt.xlabel("Time (Seconds)")
        plt.ylabel("Horizontal Error (Meters)")
        plt.legend()
        plt.grid(True, which='both', linestyle='--', alpha=0.5)
        
        plot_path = os.path.join(self.batch_path, f"{tc_id}_plot.png")
        plt.savefig(plot_path)
        plt.close()

    def generate_final_report(self, test_results):
        """Generates a formal Software Verification Results (SVR) document."""
        report_path = os.path.join(self.batch_path, "SVR_Summary_Report.txt")
        
        with open(report_path, "w") as f:
            f.write("="*70 + "\n")
            f.write(f"SOFTWARE VERIFICATION RESULTS (SVR) - BATCH {self.batch_id}\n")
            f.write(f"DATE: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("="*70 + "\n\n")

            for res in test_results:
                f.write(f"TEST CASE: {res['id']} [{res['verdict']}]\n")
                f.write(f"Scenario : {res['name']}\n")
                f.write("-" * 30 + "\n")
                
                for srid, check in res['checks'].items():
                    status = "PASS" if check['pass'] else "FAIL"
                    f.write(f"  {srid}: Measured={check['val']} | {check['detail']} -> [{status}]\n")
                f.write("\n")

        print(f"\n>>> [COMPLETED] Final SVR Report generated: {report_path}")

    def execute_batch(self):
        """Orchestrates the entire test suite execution."""
        all_results = []

        for tc_id, config in TEST_SUITE.items():
            csv_path = self.run_simulation(tc_id, config)
            
            if csv_path:
                df = pd.read_csv(csv_path)
                
                # Skip if CSV is empty (no measurements recorded)
                if len(df) == 0:
                    print(f"[WARNING] {tc_id}: No data recorded. Skipping validation and plotting.")
                    all_results.append({
                        "id": tc_id,
                        "name": config['name'],
                        "verdict": "INCOMPLETE",
                        "checks": {}
                    })
                else:
                    # Call our scalable validator
                    verdict = self.validator.verify_scenario(df)
                    
                    all_results.append({
                        "id": tc_id,
                        "name": config['name'],
                        "verdict": verdict["status"],
                        "checks": verdict["checks"]
                    })

                    # Generate Plot
                    self.plot_tc_result(tc_id, df, verdict, config['name'])

                # Cleanup after each TC
                subprocess.run(["docker-compose", "down", "-v"], capture_output=True)
            else:
                print(f"[ERROR] {tc_id}: No CSV log found, cannot process.")

        self.generate_final_report(all_results)

if __name__ == "__main__":
    executive = TestExecutive()
    executive.execute_batch()