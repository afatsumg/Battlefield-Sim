#include "monitor_cli.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

MonitorCLI::MonitorCLI(const std::string &fusion_addr)
{
    auto channel = grpc::CreateChannel(fusion_addr, grpc::InsecureChannelCredentials());
    stub_ = fusion::FusionMonitor::NewStub(channel);
}

void MonitorCLI::ClearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    // ANSI clear for most UNIX terminals
    std::cout << "\033[2J\033[H";
#endif
}

void MonitorCLI::PrintTable(const std::vector<fusion::FusedTrack> &tracks)
{
    std::cout << "==================== FUSED TRACK MONITOR ====================\n";
    std::cout << std::left << std::setw(12) << "TRACK ID"
              << std::setw(12) << "LAT"
              << std::setw(12) << "LON"
              << std::setw(14) << "ALT(m)"
              << std::setw(10) << "CONF"
              << std::setw(12) << "ERR(m)"
              << std::setw(18) << "SOURCES"
              << "\n";
    std::cout << "------------------------------------------------------------\n";

    for (const auto &t : tracks)
    {
        // If we have a UAV source, show external UAV id as the primary label (Estimated)
        std::string primary_label;
        for (int i = 0; i < t.source_sensors_size(); ++i)
        {
            if (t.source_sensors(i).rfind("UAV", 0) == 0)
            {
                primary_label = t.source_sensors(i) + std::string(" (Estimated)");
                break;
            }
        }
        if (primary_label.empty())
            primary_label = std::to_string(t.track_id());

        std::cout << std::left << std::setw(12) << primary_label
                  << std::fixed << std::setprecision(5)
                  << std::setw(12) << t.position().lat()
                  << std::setw(12) << t.position().lon()
                  << std::setw(14) << t.position().alt()
                  << std::setw(10) << std::setprecision(3) << t.confidence()
                  << std::setprecision(6);

        // Build sources string
        std::string srcs;
        for (int i = 0; i < t.source_sensors_size(); ++i)
        {
            if (i)
                srcs += ",";
            srcs += t.source_sensors(i);
        }
        // print error (if present) and sources
        double err = 0.0;
        // fused proto now contains uav_error_m
        err = t.uav_error_m();
        std::cout << std::setw(12) << std::fixed << std::setprecision(1) << err;
        std::cout << std::setw(18) << srcs << "\n";

        // If we have a reported UAV position, print it on a separate line for comparison
        if (t.uav_reported().lat() != 0.0 || t.uav_reported().lon() != 0.0)
        {
            std::string uav_id;
            for (int i = 0; i < t.source_sensors_size(); ++i)
            {
                // heuristically pick the first source that starts with "UAV"
                if (t.source_sensors(i).rfind("UAV", 0) == 0)
                {
                    uav_id = t.source_sensors(i);
                    break;
                }
            }
            if (uav_id.empty())
            {
                uav_id = "UAV-?";
            }
            std::cout << "  " << std::left << std::setw(12) << (uav_id + std::string(" (reported)"))
                      << std::setw(12) << std::fixed << std::setprecision(5) << t.uav_reported().lat()
                      << std::setw(12) << t.uav_reported().lon()
                      << std::setw(14) << t.uav_reported().alt()
                      << std::setw(10) << "-" << std::setw(18) << "\n";
        }
    }
    std::cout << "=============================================================" << std::endl;
}

void MonitorCLI::Run()
{
    while (true)
    {
        fusion::MonitorRequest req;
        grpc::ClientContext ctx;

        std::unique_ptr<grpc::ClientReader<fusion::MonitorResponse>> reader(
            stub_->SubscribeFusedTracks(&ctx, req));

        fusion::MonitorResponse resp;
        std::vector<fusion::FusedTrack> tracks;
        while (reader->Read(&resp))
        {
            for (const auto &t : resp.tracks())
                tracks.push_back(t);
        }

        grpc::Status status = reader->Finish();

        ClearScreen();

        if (!status.ok())
        {
            std::cout << "[MonitorCLI] Fusion service unreachable: "
                      << status.error_message() << std::endl;
        }
        else
        {
            PrintTable(tracks);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }
}
