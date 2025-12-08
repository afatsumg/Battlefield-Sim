#include "radar_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath> // For sin/cos motion

int main()
{
    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : std::string("fusion_service:6000");
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    RadarClient client(channel);

    std::cout << "[RADAR] Simulation started..." << std::endl;

    double t = 0.0;

    while (true)
    {
        sensors::RadarDetection msg;

        // Timestamp and sensor ID
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.mutable_header()->set_sensor_id("RADAR-Main");

        // Simulated target data
        // Target is moving away (range increasing) and changing bearing
        msg.set_track_id("TRK-001");
        msg.set_range(12000.0 + (t * 10.0));             // Range increases by 10 m per second
        msg.set_bearing(45.0 + std::sin(t * 0.1) * 5.0); // Oscillates roughly between 40-50 degrees
        msg.set_rcs(5.0);
        msg.set_velocity(250.0); // 250 m/s

        if (!client.sendDetection(msg))
        {
            std::cerr << "[RADAR] Send failed, retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // In a real scenario the client should be recreated here (reconnect)
            break;
        }

        std::cout << "[RADAR] Sent Track " << msg.track_id()
                  << " | Range: " << msg.range()
                  << " | Brg: " << msg.bearing() << std::endl;

        // Wait 100 ms (10 Hz radar update rate)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t += 0.1;
    }

    return 0;
}