#include "uav_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <fstream>
#include <filesystem>
#include <iomanip>

double get_env_double(const std::string &key, double default_val)
{
    const char *val = std::getenv(key.c_str());
    if (!val)
        return default_val;
    try
    {
        return std::stod(val);
    }
    catch (...)
    {
        return default_val;
    }
}

int main()
{
    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : std::string("fusion_service:6000");
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    UAVClient client(channel);

    // Parameters from docker compose or defaults
    double current_lat = get_env_double("UAV_START_LAT", 39.920);
    double current_lon = get_env_double("UAV_START_LON", 32.850);
    double current_alt = get_env_double("UAV_START_ALT", 1200.0);
    double current_heading = get_env_double("UAV_START_HEADING", 45.0);
    double current_speed = get_env_double("UAV_SPEED", 80.0);

    std::cout << "[UAV] Simulation started with settings:" << std::endl;
    std::cout << "      Lat: " << current_lat << " Lon: " << current_lon << " Speed: " << current_speed << std::endl;

    double time_s = 0.0;
    const char *env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : std::string("/workspace/shared/ground_truth.txt");

    double max_time = get_env_double("SIM_DURATION_SEC", -1.0);
    // Ensure shared directory exists for ground truth
    try
    {
        auto parent = std::filesystem::path(truth_path).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);
    }
    catch (...)
    {
        // ignore
    }

    while (true)
    {
        if (max_time > 0 && time_s >= max_time) {
            std::cout << "[UAV] Simulation time finished. Exiting." << std::endl;
            break; 
        }

        sensors::UAVTelemetry msg;

        // Set timestamp and UAV ID
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.set_uav_id("UAV-ALFA");

        // Movement simulation. Changes in the position based on simple functions.
        current_lat += 0.0005;
        current_lon += std::sin(time_s / 50.0) * 0.0002;
        current_alt += std::cos(time_s / 10.0) * 5.0;

        msg.mutable_position()->set_lat(current_lat);
        msg.mutable_position()->set_lon(current_lon);
        msg.mutable_position()->set_alt(current_alt);

        current_heading += std::sin(time_s / 10.0) * 2.0;
        msg.set_speed(current_speed);
        msg.set_heading(current_heading);
        msg.set_status("Flying");

        if (!client.sendTelemetry(msg))
        {
            std::cerr << "[UAV] Connection lost." << std::endl;
            break;
        }

        // Ground Truth
        try
        {
            // Ensure shared directory exists (best-effort)
            std::ofstream ofs(truth_path, std::ofstream::trunc);
            if (ofs)
            {
                ofs << std::fixed << std::setprecision(9)
                    << msg.position().lat() << " "
                    << msg.position().lon() << " "
                    << msg.position().alt() << " "
                    << msg.header().timestamp() << " "
                    << msg.heading() << "\n";
                ofs.close();
            }
        }
        catch (...)
        {
        }

        // Send data at 1 Hz
        std::this_thread::sleep_for(std::chrono::seconds(1));
        time_s += 1.0;
    }
    return 0;
}