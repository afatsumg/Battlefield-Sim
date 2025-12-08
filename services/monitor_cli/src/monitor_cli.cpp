#include "monitor_cli.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

MonitorCLI::MonitorCLI(const std::string& fusion_addr) {
    auto channel = grpc::CreateChannel(fusion_addr, grpc::InsecureChannelCredentials());
    stub_ = fusion::FusionMonitor::NewStub(channel);
}

void MonitorCLI::ClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    // ANSI clear for most UNIX terminals
    std::cout << "\033[2J\033[H";
#endif
}

void MonitorCLI::PrintTable(const std::vector<fusion::FusedTrack>& tracks) {
    std::cout << "==================== FUSED TRACK MONITOR ====================\n";
    std::cout << std::left << std::setw(12) << "TRACK ID"
              << std::setw(12) << "LAT"
              << std::setw(12) << "LON"
              << std::setw(14) << "ALT(m)"
              << std::setw(10) << "CONF"
              << "\n";
    std::cout << "------------------------------------------------------------\n";

    for (const auto& t : tracks) {
        std::cout << std::left << std::setw(12) << t.track_id()
                  << std::fixed << std::setprecision(5)
                  << std::setw(12) << t.position().lat()
                  << std::setw(12) << t.position().lon()
                  << std::setw(14) << t.position().alt()
                  << std::setw(10) << std::setprecision(3) << t.confidence()
                  << std::setprecision(6) << "\n";
    }
    std::cout << "=============================================================" << std::endl;
}

void MonitorCLI::Run() {
    while (true) {
        fusion::MonitorRequest req;
        grpc::ClientContext ctx;

        std::unique_ptr<grpc::ClientReader<fusion::MonitorResponse>> reader(
            stub_->SubscribeFusedTracks(&ctx, req)
        );

        fusion::MonitorResponse resp;
        std::vector<fusion::FusedTrack> tracks;
        while (reader->Read(&resp)) {
            for (const auto& t : resp.tracks()) tracks.push_back(t);
        }

        grpc::Status status = reader->Finish();

        ClearScreen();

        if (!status.ok()) {
            std::cout << "[MonitorCLI] Fusion service unreachable: "
                      << status.error_message() << std::endl;
        } else {
            PrintTable(tracks);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
}
