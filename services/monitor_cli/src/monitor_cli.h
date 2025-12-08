#pragma once

#include <grpcpp/grpcpp.h>
#include "fusion/fusion.grpc.pb.h"
#include <vector>
#include <string>

class MonitorCLI {
public:
    explicit MonitorCLI(const std::string& fusion_addr);

    void Run();

private:
    std::unique_ptr<fusion::FusionMonitor::Stub> stub_;

    void ClearScreen();
    void PrintTable(const std::vector<fusion::FusedTrack>& tracks);
};
