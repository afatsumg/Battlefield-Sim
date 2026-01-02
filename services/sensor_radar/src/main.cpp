#include "radar_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <fstream>
#include <iomanip>
#include <string>

#include "config.h"
#include "geo_utils.h"
#include "physics.h"

int main()
{
    // --- Environment Config ---
    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : "fusion_service:6000";
    
    const char *env_rcs_active = std::getenv("RADAR_RCS_ACTIVE");
    bool enable_dynamic_rcs = (env_rcs_active && std::string(env_rcs_active) == "true");

    const char *env_id = std::getenv("RADAR_ID");
    std::string radar_id = env_id ? env_id : "RADAR-X";

    // Get env variables
    double radar_lat = utils::GetEnvDouble("RADAR_LAT", 39.9);
    double radar_lon = utils::GetEnvDouble("RADAR_LON", 32.8);
    double radar_sensitivity = utils::GetEnvDouble("RADAR_SENSITIVITY", 1e-12);
    double range_sigma_val = utils::GetEnvDouble("RADAR_RANGE_SIGMA", 30.0);
    double bearing_sigma_val = utils::GetEnvDouble("RADAR_BEARING_SIGMA", 1.0);
    double sim_duration = utils::GetEnvDouble("SIM_DURATION_SEC", 0.0);

    // --- Init ---
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    RadarClient client(channel);

    const char *env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : "/workspace/shared/ground_truth.txt";

    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> range_noise(0.0, range_sigma_val);
    std::normal_distribution<> bearing_noise(0.0, bearing_sigma_val);

    std::cout << "[" << radar_id << "] Booted. RCS_MODEL=" << (enable_dynamic_rcs ? "ON" : "OFF") 
              << " | SENSITIVITY=" << radar_sensitivity << std::endl;

    auto start_time = std::chrono::steady_clock::now();

    while (true)
    {
        // Zaman kontrolü
        if (sim_duration > 0) {
            auto now_clock = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration_cast<std::chrono::seconds>(now_clock - start_time).count();
            if (elapsed >= sim_duration) {
                std::cout << "[" << radar_id << "] Duration reached. Shutting down." << std::endl;
                break;
            }
        }

        double gt_lat, gt_lon, gt_alt, gt_ts, gt_heading;
        std::ifstream ifs(truth_path);
        
        if (ifs >> gt_lat >> gt_lon >> gt_alt >> gt_ts >> gt_heading)
        {
            double true_rng = geo_utils::CalculateHaversine(radar_lat, radar_lon, gt_lat, gt_lon);
            double rcs_to_use = 2.0; 

            if (enable_dynamic_rcs) {
                rcs_to_use = physics::CalculateAspectRCS(gt_lat, gt_lon, gt_heading, radar_lat, radar_lon);
            }

            // --- Physics Check ---
            double signal_strength = physics::CalculateSignalStrength(rcs_to_use, true_rng);

            if (signal_strength > radar_sensitivity)
            {
                double noisy_rng = true_rng + range_noise(gen);
                double noisy_brg = geo_utils::BearingDegrees(radar_lat, radar_lon, gt_lat, gt_lon) + bearing_noise(gen);
                
                // Compute target GPS from radar origin, range and bearing using polar-to-geo conversion
                // (Similar to PolarToGeo but returning simple pair instead of protobuf)
                const double R_EARTH = 6371000.0;
                double ad = noisy_rng / R_EARTH;
                double brng = noisy_brg * M_PI / 180.0;
                double phi1 = radar_lat * M_PI / 180.0;
                double lam1 = radar_lon * M_PI / 180.0;
                double phi2 = std::asin(std::sin(phi1) * std::cos(ad) + std::cos(phi1) * std::sin(ad) * std::cos(brng));
                double lam2 = lam1 + std::atan2(std::sin(brng) * std::sin(ad) * std::cos(phi1), std::cos(ad) - std::sin(phi1) * std::sin(phi2));
                double target_lat = phi2 * 180.0 / M_PI;
                double target_lon = lam2 * 180.0 / M_PI;

                sensors::RadarDetection msg;
                auto now = std::chrono::system_clock::now().time_since_epoch();
                msg.mutable_header()->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
                msg.mutable_header()->set_sensor_id(radar_id);
                msg.set_track_id("UAV-ALFA");
                msg.set_range(noisy_rng);
                msg.set_bearing(noisy_brg);
                msg.set_radar_lat(target_lat);
                msg.set_radar_lon(target_lon);
                msg.set_radar_alt(gt_alt);
                msg.set_rcs(rcs_to_use);

                client.sendDetection(msg);
            }
            else {
                // To avoid spamming, we only log signal drops in debug or every few seconds
                static int drop_count = 0;
                if (drop_count++ % 50 == 0)
                   std::cout << "[" << radar_id << "] Target stealthy or out of range. SNR low." << std::endl;
            }
        }
        ifs.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}