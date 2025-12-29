#include "uav_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath> // for sin() function
#include <random>
#include <fstream>
#include <filesystem>

int main()
{
    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : std::string("fusion_service:6000");
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    UAVClient client(channel);

    std::cout << "[UAV] Simulation started (1 Hz)..." << std::endl;

    double time_s = 0.0; // Simulation time in seconds
    double current_lat = 39.920;
    double current_lon = 32.850;
    double current_alt = 1200.0;
    double current_heading = 45.0;
    double current_speed = 80.0;

    // RNG kept for optional use; default ground truth is noise-free
    std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<> dist(-0.0001, 0.0001); // unused unless you enable noise

    // Shared ground truth file (can be overridden)
    const char *env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : std::string("/workspace/shared/ground_truth.txt");

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
        sensors::UAVTelemetry msg;

        // Set timestamp and UAV ID
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.set_uav_id("UAV-ALFA");

        // Position (ground-truth, noise-free)
        current_lat += 0.0005;                           // Towards north (no noise)
        current_lon += std::sin(time_s / 50.0) * 0.0002; // East-West oscillation
        current_alt += std::cos(time_s / 10.0) * 50;     // Small altitude changes
        msg.mutable_position()->set_lat(current_lat);
        msg.mutable_position()->set_lon(current_lon);
        msg.mutable_position()->set_alt(current_alt); // Altitude oscillation

        current_heading += std::sin(time_s / 10.0) * 10.0; // Small heading changes
        msg.set_speed(current_speed);                      // Constant speed
        msg.set_heading(current_heading);                  // Heading changes by the time
        msg.set_status("Flying");

        if (!client.sendTelemetry(msg))
        {
            std::cerr << "[UAV] Connection lost. Breaking loop." << std::endl;
            break;
        }

        // Publish ground truth to shared file for sensors/fusion comparisons
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
                    << msg.heading() << "\n"; // HEADING BURAYA EKLENDİ
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