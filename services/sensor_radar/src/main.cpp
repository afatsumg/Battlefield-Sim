#include "radar_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <fstream>
#include <iomanip>
#include <string>

// Helper: Environment değişkenini oku, yoksa default dön
double get_env_double(const std::string& key, double default_val) {
    const char* val = std::getenv(key.c_str());
    if (!val) return default_val;
    try {
        return std::stod(val);
    } catch (...) {
        return default_val;
    }
}

// Helper: compute target GPS from radar origin, range and bearing
std::pair<double, double> calculate_target_gps(double lat1, double lon1, double range, double bearing)
{
    const double R_EARTH = 6371000.0;
    double ad = range / R_EARTH;
    double brng = bearing * M_PI / 180.0;
    double phi1 = lat1 * M_PI / 180.0;
    double lam1 = lon1 * M_PI / 180.0;

    double phi2 = std::asin(std::sin(phi1) * std::cos(ad) + std::cos(phi1) * std::sin(ad) * std::cos(brng));
    double lam2 = lam1 + std::atan2(std::sin(brng) * std::sin(ad) * std::cos(phi1), std::cos(ad) - std::sin(phi1) * std::sin(phi2));

    return {phi2 * 180.0 / M_PI, lam2 * 180.0 / M_PI};
}

// Calculate Dynamic RCS based on Aspect Angle
double calculate_aspect_rcs(double uav_lat, double uav_lon, double uav_heading, double r_lat, double r_lon)
{
    double phi1 = r_lat * M_PI / 180.0;
    double phi2 = uav_lat * M_PI / 180.0;
    double dLam = (uav_lon - r_lon) * M_PI / 180.0;
    double y = std::sin(dLam) * std::cos(phi2);
    double x = std::cos(phi1) * std::sin(phi2) - std::sin(phi1) * std::cos(phi2) * std::cos(dLam);
    double bearing_to_uav = std::atan2(y, x) * 180.0 / M_PI;
    if (bearing_to_uav < 0) bearing_to_uav += 360.0;

    // Aspect Angle: Difference between where UAV points and where Radar is
    double alpha_rad = std::abs(uav_heading - bearing_to_uav) * M_PI / 180.0;

    // Small UAV RCS Model: Frontal (0.1 m2) vs Side (2.0 m2)
    double rcs_min = 0.1;
    double rcs_max = 2.0;
    return rcs_min * std::pow(std::cos(alpha_rad), 2) + rcs_max * std::pow(std::sin(alpha_rad), 2);
}

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
    double radar_lat = get_env_double("RADAR_LAT", 39.9);
    double radar_lon = get_env_double("RADAR_LON", 32.8);
    double radar_sensitivity = get_env_double("RADAR_SENSITIVITY", 1e-12);
    double range_sigma_val = get_env_double("RADAR_RANGE_SIGMA", 30.0);
    double bearing_sigma_val = get_env_double("RADAR_BEARING_SIGMA", 1.0);
    double sim_duration = get_env_double("SIM_DURATION_SEC", 0.0);

    // --- Init ---
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    RadarClient client(channel);

    const char *env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : "/workspace/shared/ground_truth.txt";

    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> range_noise(0.0, range_sigma_val);
    std::normal_distribution<> bearing_noise(0.0, bearing_sigma_val);

    const double R = 6371000.0;
    auto haversine = [&](double lat1, double lon1, double lat2, double lon2) {
        double dlat = (lat2 - lat1) * M_PI / 180.0;
        double dlon = (lon2 - lon1) * M_PI / 180.0;
        double a = std::sin(dlat / 2) * std::sin(dlat / 2) + std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) * std::sin(dlon / 2) * std::sin(dlon / 2);
        return R * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    };

    auto bearing_deg = [&](double lat1, double lon1, double lat2, double lon2) {
        double phi1 = lat1 * M_PI / 180.0; double phi2 = lat2 * M_PI / 180.0;
        double lam1 = lon1 * M_PI / 180.0; double lam2 = lon2 * M_PI / 180.0;
        double y = std::sin(lam2 - lam1) * std::cos(phi2);
        double x = std::cos(phi1) * std::sin(phi2) - std::sin(phi1) * std::cos(phi2) * std::cos(lam2 - lam1);
        double br = std::atan2(y, x) * 180.0 / M_PI;
        return (br < 0) ? br + 360.0 : br;
    };

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
            double true_rng = haversine(radar_lat, radar_lon, gt_lat, gt_lon);
            double rcs_to_use = 2.0; 

            if (enable_dynamic_rcs) {
                rcs_to_use = calculate_aspect_rcs(gt_lat, gt_lon, gt_heading, radar_lat, radar_lon);
            }

            // --- Physics Check ---
            double signal_strength = rcs_to_use / std::pow(true_rng, 4);

            if (signal_strength > radar_sensitivity)
            {
                double noisy_rng = true_rng + range_noise(gen);
                double noisy_brg = bearing_deg(radar_lat, radar_lon, gt_lat, gt_lon) + bearing_noise(gen);
                auto target_gps = calculate_target_gps(radar_lat, radar_lon, noisy_rng, noisy_brg);

                sensors::RadarDetection msg;
                auto now = std::chrono::system_clock::now().time_since_epoch();
                msg.mutable_header()->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
                msg.mutable_header()->set_sensor_id(radar_id);
                msg.set_track_id("UAV-ALFA");
                msg.set_range(noisy_rng);
                msg.set_bearing(noisy_brg);
                msg.set_radar_lat(target_gps.first);
                msg.set_radar_lon(target_gps.second);
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