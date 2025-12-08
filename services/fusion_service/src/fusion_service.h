#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <chrono>

#include "fusion/fusion.grpc.pb.h"
#include "sensors/uav.pb.h"
#include "sensors/radar.pb.h"
#include "sensors/sigint.pb.h"
#include "common/geo.pb.h" // Required for GeoPoint conversions

class FusionServiceImpl final : public fusion::FusionService::Service {
public:
    FusionServiceImpl();

    // Shared data must be public so the Monitor can read it
    std::unordered_map<uint32_t, fusion::FusedTrack> fused_tracks_;
    std::mutex mtx_; // Single mutex for track buffer and fused tracks

    grpc::Status StreamUAV(
        grpc::ServerContext* context,
        grpc::ServerReader<sensors::UAVTelemetry>* reader,
        fusion::FusionAck* ack) override;

    grpc::Status StreamRadar(
        grpc::ServerContext* context,
        grpc::ServerReader<sensors::RadarDetection>* reader,
        fusion::FusionAck* ack) override;

    grpc::Status StreamSigint(
        grpc::ServerContext* context,
        grpc::ServerReader<sensors::SigintHit>* reader,
        fusion::FusionAck* ack) override;

private:
    struct TrackData {
        bool has_uav = false;
        bool has_radar = false;
        bool has_sigint = false;

        sensors::UAVTelemetry uav;
        sensors::RadarDetection radar;
        sensors::SigintHit sigint;
        
        long last_update_ms = 0;
    };

    // Buffer that stores all sensor data keyed by track ID
    std::unordered_map<uint32_t, TrackData> track_buffer_; 
    
    // Radar'ın sabit konumu (GeoPoint dönüşümü için)
    common::GeoPoint radar_position_;

    // Füzyon ve Koordinat Metotları
    fusion::FusedTrack Fuse(uint32_t track_id);
    common::GeoPoint PolarToGeo(double range_m, double bearing_deg, const common::GeoPoint& origin);
};