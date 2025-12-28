#include "fusion_monitor.h"
#include <iostream>

// Constructor: stores references to the shared mutex and track map.
FusionMonitorServiceImpl::FusionMonitorServiceImpl(
    std::mutex& track_mtx,
    std::unordered_map<uint32_t, fusion::FusedTrack>& tracks)
    : mtx_(track_mtx), fused_tracks_(tracks)
{}
// GetFusedTracks RPC implementation
// Returns the current list of fused tracks to the client.
grpc::Status FusionMonitorServiceImpl::SubscribeFusedTracks(
    grpc::ServerContext* context,
    const fusion::MonitorRequest* request,
    grpc::ServerWriter<fusion::MonitorResponse>* writer)
{
    // For simplicity we send a single MonitorResponse containing the current
    // fused tracks and then return; a real streaming implementation could
    // push updates periodically or on-change.
    fusion::MonitorResponse resp;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (const auto& kv : fused_tracks_) {
            fusion::FusedTrack* track = resp.add_tracks();
            *track = kv.second;
        }
    }

    // Send one response over the stream.
    writer->Write(resp);

    return grpc::Status::OK;
}