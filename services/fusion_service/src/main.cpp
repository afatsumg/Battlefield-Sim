#include "fusion_service.h"
#include "fusion_monitor.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>

int main()
{
    std::string fusion_address("0.0.0.0:6000");
    std::string monitor_address("0.0.0.0:6005");

    FusionServiceImpl fusion_service;

    // 1. Fusion Server Kurulumu (Sensörler için)
    grpc::ServerBuilder fusion_builder;
    fusion_builder.AddListeningPort(fusion_address, grpc::InsecureServerCredentials());
    fusion_builder.RegisterService(&fusion_service);
    std::unique_ptr<grpc::Server> fusion_server(fusion_builder.BuildAndStart());
    std::cout << "[FusionService] Running at " << fusion_address << std::endl;

    // 2. Monitor Server Kurulumu (CLI/Web UI için)
    // Paylaşılan veriyi ve mutex'i FusionService'den alıyoruz.
    FusionMonitorServiceImpl monitor_service(fusion_service.mtx_, fusion_service.fused_tracks_);
    grpc::ServerBuilder monitor_builder;
    monitor_builder.AddListeningPort(monitor_address, grpc::InsecureServerCredentials());
    monitor_builder.RegisterService(&monitor_service);
    std::unique_ptr<grpc::Server> monitor_server(monitor_builder.BuildAndStart());
    std::cout << "[FusionMonitor] Running at " << monitor_address << std::endl;

    // İki sunucuyu da bekletmek için Monitor'ü ayrı bir thread'e atıyoruz.
    std::thread monitor_thread([&monitor_server]()
                               {
        std::cout << "Monitor Server Thread Started." << std::endl;
        monitor_server->Wait(); });

    // Ana thread'i Fusion Server'ı bekletir
    fusion_server->Wait();

    // Monitor thread'in bitmesini bekle (Normalde buraya hiç ulaşılmaz)
    if (monitor_thread.joinable())
    {
        monitor_thread.join();
    }

    std::cout << "All servers stopped." << std::endl;
    return 0;
}