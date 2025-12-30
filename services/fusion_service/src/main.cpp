#include "fusion_service.h"
#include "fusion_monitor.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>
#include <string> // stoi için eklendi

int main()
{
    std::string fusion_address("0.0.0.0:6000");
    std::string monitor_address("0.0.0.0:6005");

    FusionServiceImpl fusion_service;

    // --- TIMEOUT KONTROLÜ BURAYA ---
    const char* env_dur = std::getenv("SIM_DURATION_SEC");
    int duration = (env_dur && std::string(env_dur) != "") ? std::stoi(env_dur) : 0;
    
    if (duration > 0) {
        std::cout << "[FUSION] Auto-shutdown scheduled in " << duration << " seconds." << std::endl;
        fusion_service.StartTimeoutThread(duration);
    }
    // -------------------------------

    // 1. Fusion server setup (for sensors)
    grpc::ServerBuilder fusion_builder;
    fusion_builder.AddListeningPort(fusion_address, grpc::InsecureServerCredentials());
    fusion_builder.RegisterService(&fusion_service);
    std::unique_ptr<grpc::Server> fusion_server(fusion_builder.BuildAndStart());
    std::cout << "[FusionService] Running at " << fusion_address << std::endl;

    // 2. Monitor server setup (for CLI/Web UI)
    // Get shared data and mutex from FusionService.
    FusionMonitorServiceImpl monitor_service(fusion_service.mtx_, fusion_service.fused_tracks_);
    grpc::ServerBuilder monitor_builder;
    monitor_builder.AddListeningPort(monitor_address, grpc::InsecureServerCredentials());
    monitor_builder.RegisterService(&monitor_service);
    std::unique_ptr<grpc::Server> monitor_server(monitor_builder.BuildAndStart());
    std::cout << "[FusionMonitor] Running at " << monitor_address << std::endl;

    // Run the monitor in a separate thread
    std::thread monitor_thread([&monitor_server]() {
        std::cout << "Monitor Server Thread Started." << std::endl;
        monitor_server->Wait(); 
    });

    // Main thread waits on the Fusion server
    fusion_server->Wait();

    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }

    std::cout << "All servers stopped." << std::endl;
    return 0;
}