#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "fusion/fusion.grpc.pb.h"

// Uses the FusedTrack map and mutex defined in the Fusion Service.
class FusionMonitorServiceImpl final : public fusion::FusionMonitor::Service {
public:
    // Constructor: takes references to shared data.
    FusionMonitorServiceImpl(std::mutex& track_mtx,
                             std::unordered_map<uint32_t, fusion::FusedTrack>& tracks);

    // Server-streaming RPC: sends MonitorResponse streams to subscribed clients.
    grpc::Status SubscribeFusedTracks(grpc::ServerContext* context,
                                      const fusion::MonitorRequest* request,
                                      grpc::ServerWriter<fusion::MonitorResponse>* writer) override;
                                
    // Note: Additional methods for server-streaming can be added here.

private:
    // Fusion Service'den gelen paylaşımlı mutex'e referans
    std::mutex& mtx_; 
    
    // Fusion Service'den gelen paylaşılan track listesine referans
    std::unordered_map<uint32_t, fusion::FusedTrack>& fused_tracks_;
};