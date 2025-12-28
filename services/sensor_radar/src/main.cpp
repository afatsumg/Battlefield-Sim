#include "radar_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <fstream>
#include <iomanip>

// Helper: compute target GPS from radar origin, range and bearing
std::pair<double, double> calculate_target_gps(double lat1, double lon1, double range, double bearing) {
    const double R_EARTH = 6371000.0; // Earth radius (meters)
    double ad = range / R_EARTH;
    double brng = bearing * M_PI / 180.0;
    double phi1 = lat1 * M_PI / 180.0;
    double lam1 = lon1 * M_PI / 180.0;

    double phi2 = std::asin(std::sin(phi1) * std::cos(ad) + std::cos(phi1) * std::sin(ad) * std::cos(brng));
    double lam2 = lam1 + std::atan2(std::sin(brng) * std::sin(ad) * std::cos(phi1), std::cos(ad) - std::sin(phi1) * std::sin(phi2));

    return {phi2 * 180.0 / M_PI, lam2 * 180.0 / M_PI};
}

int main() {
    // 1. Connection settings
    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : "fusion_service:6000";
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    RadarClient client(channel);

    // 2. Sensor position (provided by docker-compose)
    const char *env_lat = std::getenv("RADAR_LAT");
    const char *env_lon = std::getenv("RADAR_LON");
    const char *env_alt = std::getenv("RADAR_ALT");
    double radar_lat = env_lat ? atof(env_lat) : 39.9;
    double radar_lon = env_lon ? atof(env_lon) : 32.8;
    double radar_alt = env_alt ? atof(env_alt) : 150.0;

    // 3. Ground truth file path
    const char *env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : "/workspace/shared/ground_truth.txt";

    // 4. Noise and bias settings
    const char *env_r_sigma = std::getenv("RADAR_RANGE_SIGMA");
    const char *env_b_sigma = std::getenv("RADAR_BEARING_SIGMA");
    double range_sigma = env_r_sigma ? atof(env_r_sigma) : 30.0;
    double bearing_sigma = env_b_sigma ? atof(env_b_sigma) : 1.0;

    const char *env_r_bias = std::getenv("RADAR_RANGE_BIAS");
    const char *env_b_bias = std::getenv("RADAR_BEARING_BIAS");
    double range_bias = env_r_bias ? atof(env_r_bias) : 0.0;
    double bearing_bias = env_b_bias ? atof(env_b_bias) : 0.0;

    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> range_noise(0.0, range_sigma);
    std::normal_distribution<> bearing_noise(0.0, bearing_sigma);

    // Haversine and bearing lambdas (to compute true distance/angle)
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

    std::cout << "[RADAR] Sensor " << (std::getenv("RADAR_ID") ? std::getenv("RADAR_ID") : "UNKNOWN") 
              << " at Lat:" << radar_lat << " Lon:" << radar_lon << " started." << std::endl;

    while (true) {
        sensors::RadarDetection msg;
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.mutable_header()->set_sensor_id(std::getenv("RADAR_ID") ? std::getenv("RADAR_ID") : "RADAR-X");

        double gt_lat = 0, gt_lon = 0, gt_alt = 0;
        std::ifstream ifs(truth_path);
        if (ifs >> gt_lat >> gt_lon >> gt_alt) {
            // True distance / bearing
            double true_rng = haversine(radar_lat, radar_lon, gt_lat, gt_lon);
            double true_brg = bearing_deg(radar_lat, radar_lon, gt_lat, gt_lon);

            // Noisy distance / bearing
            double noisy_rng = true_rng + range_noise(gen) + range_bias;
            double noisy_brg = true_brg + bearing_noise(gen) + bearing_bias;

            // IMPORTANT: compute target GPS from noisy measurements
            auto target_gps = calculate_target_gps(radar_lat, radar_lon, noisy_rng, noisy_brg);

            msg.set_track_id("UAV-ALFA");
            msg.set_range(noisy_rng);
            msg.set_bearing(noisy_brg);
            
            // Set coordinates sent to fusion service
            msg.set_radar_lat(target_gps.first); 
            msg.set_radar_lon(target_gps.second);
            msg.set_radar_alt(gt_alt);
            msg.set_rcs(5.0);
            msg.set_velocity(250.0);

            if (client.sendDetection(msg)) {
                std::cout << "[" << msg.header().sensor_id() << "] Target calculated at: " 
                          << std::fixed << std::setprecision(6) << target_gps.first << ", " << target_gps.second << std::endl;
            }
        }
        ifs.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}