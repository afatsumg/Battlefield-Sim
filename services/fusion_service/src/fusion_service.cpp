#include "fusion_service.h"
#include <iostream>
#include <cmath>
#include <algorithm> // std::max için
#include <sstream>

// Fixed radar position (for simulation simplicity)
namespace
{
    constexpr double RADAR_LAT = 39.9;
    constexpr double RADAR_LON = 32.8;
    constexpr double RADAR_ALT = 150.0;
    constexpr double EARTH_RADIUS = 6371000.0; // Metre
}

FusionServiceImpl::FusionServiceImpl()
{
    // Define static radar position
    radar_position_.set_lat(RADAR_LAT);
    radar_position_.set_lon(RADAR_LON);
    radar_position_.set_alt(RADAR_ALT);
}

// Haversine formula (sufficient for simulation though not exact)
common::GeoPoint FusionServiceImpl::PolarToGeo(double range_m, double bearing_deg, const common::GeoPoint &origin)
{
    common::GeoPoint target;

    // Convert degrees to radians
    double bearing_rad = bearing_deg * M_PI / 180.0;
    double lat_rad = origin.lat() * M_PI / 180.0;
    double lon_rad = origin.lon() * M_PI / 180.0;

    // Calculate new latitude
    double new_lat_rad = std::asin(std::sin(lat_rad) * std::cos(range_m / EARTH_RADIUS) +
                                   std::cos(lat_rad) * std::sin(range_m / EARTH_RADIUS) * std::cos(bearing_rad));

    // Calculate new longitude
    double new_lon_rad = lon_rad + std::atan2(std::sin(bearing_rad) * std::sin(range_m / EARTH_RADIUS) * std::cos(lat_rad),
                                              std::cos(range_m / EARTH_RADIUS) - std::sin(lat_rad) * std::sin(new_lat_rad));

    target.set_lat(new_lat_rad * 180.0 / M_PI);
    target.set_lon(new_lon_rad * 180.0 / M_PI);
    // Altitude is ignored here or could be calculated from radar elevation.
    target.set_alt(origin.alt());

    return target;
}

fusion::FusedTrack FusionServiceImpl::Fuse(uint32_t track_id)
{
    // Assumes this method is invoked while mtx_ is locked by StreamXxx methods.

    if (track_buffer_.find(track_id) == track_buffer_.end())
    {
        fusion::FusedTrack empty_track;
        empty_track.set_track_id(0);
        return empty_track;
    }

    const auto &t = track_buffer_[track_id];
    fusion::FusedTrack fused;
    fused.set_track_id(track_id);

    double lat_sum = 0, lon_sum = 0, alt_sum = 0;
    int count = 0;
    double total_confidence = 0.0;

    // 1. UAV data (GeoPoint already available)
    if (t.has_uav)
    {
        lat_sum += t.uav.position().lat();
        lon_sum += t.uav.position().lon();
        alt_sum += t.uav.position().alt();
        count++;
        total_confidence += 0.4;
    }

    // 2. Radar data (requires polar-to-GeoPoint conversion)
    if (t.has_radar)
    {
        // Range ve Bearing kullanarak GeoPoint hesapla
        common::GeoPoint estimated_pos = PolarToGeo(t.radar.range(), t.radar.bearing(), radar_position_);

        lat_sum += estimated_pos.lat();
        lon_sum += estimated_pos.lon();
        alt_sum += estimated_pos.alt();
        count++;
        total_confidence += 0.4;
    }

    // 3. SIGINT data (ignored for position for simplicity; only confidence is increased)
    // Gerçekte: Birden fazla SIGINT hit'i ile triangülasyon yapılmalıdır.
    // In reality: triangulation from multiple SIGINT hits would be required.
    if (t.has_sigint)
    {
        // Current TrackData does not contain enough information to position SIGINT.
        // We only indicate presence and increase confidence.
        total_confidence += 0.2;
    }

    if (count > 0)
    {
        // Position averaging
        fused.mutable_position()->set_lat(lat_sum / count);
        fused.mutable_position()->set_lon(lon_sum / count);
        fused.mutable_position()->set_alt(alt_sum / count);

        // Confidence (capped to 1.0)
        fused.set_confidence(std::min(1.0, total_confidence));

        // Age field removed from proto; skip setting age here. Clients can
        // compute age from timestamps in headers if needed.
    }
    else
    {
        // No data -> invalid track
        fused.set_track_id(0);
    }

    // Save result into persistent memory (under mtx_ protection)
    if (fused.track_id() != 0)
    {
        fused_tracks_[track_id] = fused;
    }

    return fused;
}

// ----------------------------------------------------------------------
// SENSOR STREAM IMPLEMENTATIONS
// ----------------------------------------------------------------------

grpc::Status FusionServiceImpl::StreamUAV(
    grpc::ServerContext *context,
    grpc::ServerReader<sensors::UAVTelemetry> *reader,
    fusion::FusionAck *ack)
{

    sensors::UAVTelemetry msg;
    uint32_t track_id = 0; // Should obtain ID from UAV

    while (reader->Read(&msg))
    {
        // Assumption: UAV telemetry ID should be convertible to uint32 or be uint32 directly.
        // If the proto uses a string, hash it here.
        // Simplification: track_id = 1 // OR use std::stoi(msg.uav_id())

        track_id = 1; // For simplicity assign all UAVs to the same track.

        std::lock_guard<std::mutex> lock(mtx_); // LOCK
        auto &t = track_buffer_[track_id];

        // Update
        t.has_uav = true;
        t.uav = msg;

        // Record timestamp
        t.last_update_ms = msg.header().timestamp();

        // Run fusion
        Fuse(track_id);
    }

    // Acknowledgement (sent after the loop finishes)
    ack->set_ok(true);
    std::ostringstream oss;
    oss << "UAV stream processed " << track_id << " successfully.";
    ack->set_message(oss.str());

    return grpc::Status::OK;
}

grpc::Status FusionServiceImpl::StreamRadar(
    grpc::ServerContext *context,
    grpc::ServerReader<sensors::RadarDetection> *reader,
    fusion::FusionAck *ack)
{

    sensors::RadarDetection msg;
    uint32_t track_id = 0;

    while (reader->Read(&msg))
    {
        // Assumption: Radar track_id should be convertible to uint32_t.
        track_id = 2; // Basitlik için tüm Radar izlerini aynı track'e atıyoruz.

        std::lock_guard<std::mutex> lock(mtx_); // LOCK
        auto &t = track_buffer_[track_id];

        // Update
        t.has_radar = true;
        t.radar = msg;

        // Record timestamp
        t.last_update_ms = msg.header().timestamp();

        // Run fusion
        Fuse(track_id);
    }

    // Acknowledgement (sent after the loop finishes)
    ack->set_ok(true);
    std::ostringstream oss;
    oss << "Radar stream processed " << track_id << " successfully.";
    ack->set_message(oss.str());

    return grpc::Status::OK;
}

grpc::Status FusionServiceImpl::StreamSigint(
    grpc::ServerContext *context,
    grpc::ServerReader<sensors::SigintHit> *reader,
    fusion::FusionAck *ack)
{

    sensors::SigintHit msg;
    uint32_t track_id = 0;

    while (reader->Read(&msg))
    {
        // SIGINT logic is more complex. For simplicity:
        track_id = 3; // Tüm SIGINT'leri aynı track'e atıyoruz.

        std::lock_guard<std::mutex> lock(mtx_); // LOCK
        auto &t = track_buffer_[track_id];

        // Update
        t.has_sigint = true;
        t.sigint = msg;

        // Record timestamp
        t.last_update_ms = msg.header().timestamp();

        // Run fusion
        Fuse(track_id);
    }

    // Acknowledgement (sent after the loop finishes)
    ack->set_ok(true);
    std::ostringstream oss;
    oss << "SIGINT stream processed " << track_id << " successfully.";
    ack->set_message(oss.str());

    return grpc::Status::OK;
}