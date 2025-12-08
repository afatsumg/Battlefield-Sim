#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "fusion/fusion.grpc.pb.h"

// Uses the FusedTrack map and mutex defined in the Fusion Service.
class FusionMonitorServiceImpl final : public fusion::FusionMonitor::Service {
public:
    // Constructor, takes references to shared data.
    FusionMonitorServiceImpl(std::mutex& track_mtx,
                             std::unordered_map<uint32_t, fusion::FusedTrack>& tracks);

    // Server-streaming RPC: sends MonitorResponse streams to subscribed clients.
    grpc::Status SubscribeFusedTracks(grpc::ServerContext* context,
                                      const fusion::MonitorRequest* request,
                                      grpc::ServerWriter<fusion::MonitorResponse>* writer) override;
                                
    // Note: Add here if server streaming RPC (SubscribeFusedTracks) is used.

private:
    // Reference to the shared mutex from Fusion Service
    std::mutex& mtx_; 
    
    // Reference to the shared track list from Fusion Service
    std::unordered_map<uint32_t, fusion::FusedTrack>& fused_tracks_;
};