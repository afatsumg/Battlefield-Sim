#include "sigint_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <fstream>

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

    // noise / base params from env
    const char* env_freq_mean = std::getenv("SIGINT_FREQ_MEAN");
    const char* env_freq_sigma = std::getenv("SIGINT_FREQ_SIGMA");
    double freq_mean = env_freq_mean ? atof(env_freq_mean) : 1450.0;
    double freq_sigma = env_freq_sigma ? atof(env_freq_sigma) : 5.0;
    std::normal_distribution<> freq_dist(freq_mean, freq_sigma);
    std::uniform_real_distribution<> bearing_dist(0.0, 360.0); // 0-360 degrees

    const char* env_truth = std::getenv("SHARED_TRUTH_PATH");
    std::string truth_path = env_truth ? env_truth : std::string("/workspace/shared/ground_truth.txt");

    int packet_count = 0;
    while (true)
    {
        sensors::SigintHit msg;

        // Timestamp and sensor ID
        auto now = std::chrono::system_clock::now().time_since_epoch();
        msg.mutable_header()->set_timestamp(
            std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
        msg.mutable_header()->set_sensor_id("SIGINT-01");

        // Payload: try reading ground truth to correlate signal
        double gt_lat=0, gt_lon=0, gt_alt=0;
        bool have_truth=false;
        std::ifstream ifs(truth_path);
        if (ifs) { ifs >> gt_lat >> gt_lon >> gt_alt; have_truth=true; ifs.close(); }

        double freq_val = freq_dist(gen);
        double power_val = current_power + (std::rand() % 10);
        double bearing_val = bearing_dist(gen);

        if (have_truth) {
            // simple deterministic modulation: higher altitude -> slightly higher frequency
            freq_val += (gt_alt - 1000.0) * 0.01;
            // bearing: approximate bearing from origin (0,0) for demo
            bearing_val = std::fmod(std::abs(gt_lon) * 10.0, 360.0) + (bearing_dist(gen) - 180.0) * 0.05;
        }

        msg.set_frequency(freq_val);
        msg.set_power(power_val);
        msg.set_confidence(current_confidence);
        msg.set_bearing(bearing_val);

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