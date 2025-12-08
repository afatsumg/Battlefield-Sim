#include "sigint_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

int main()
{
    std::cout << "[SIGINT] Simulator starting..." << std::endl;

    const char *env_addr = std::getenv("FUSION_ADDR");
    std::string fusion_target = env_addr ? env_addr : std::string("fusion_service:6000");
    auto channel = grpc::CreateChannel(fusion_target, grpc::InsecureChannelCredentials());
    SigintClient client(channel);

    double current_power = -40.0;
    double current_confidence = 0.95;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> freq_dist(1450.0, 5.0);         // Around 1450 MHz
    std::uniform_real_distribution<> bearing_dist(0.0, 360.0); // 0-360 degrees

    int packet_count = 0;
    while (true)
    {
        sensors::SigintHit msg;

        // Timestamp and sensor ID
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.mutable_header()->set_sensor_id("SIGINT-01");

        // Payload
        msg.set_frequency(freq_dist(gen));
        current_power += std::rand() % 10;
        msg.set_power(current_power);
        msg.set_confidence(current_confidence);
        msg.set_bearing(bearing_dist(gen)); // Random bearing

        if (!client.sendHit(msg))
        {
            std::cerr << "[SIGINT] Failed to send. Retrying in 5s..." << std::endl;
            // Wait for 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
            // A reconnect logic would go here in a real scenario
            break;
        }

        std::cout << "[SIGINT] Sent packet #" << ++packet_count
                  << " | Freq: " << msg.frequency()
                  << " | Bear: " << msg.bearing() << std::endl;

        // 1 Hz
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}