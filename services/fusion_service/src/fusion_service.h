#pragma once

#include <grpcpp/grpcpp.h>
#include <mutex>
#include <thread>
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <chrono>
#include <opencv2/core.hpp>
#include "kalman_filter.h"

#include "fusion/fusion.grpc.pb.h"
#include "sensors/uav.pb.h"
#include "sensors/radar.pb.h"
#include "sensors/sigint.pb.h"
#include "common/geo.pb.h"

// Raw sensor measurement structure
struct SensorMeasurement
{
    uint64_t timestamp;
    std::string sensor_type;
    std::string sensor_id;
    double lat;
    double lon;
    double alt;
    std::string extras;
};

// Fusion service class
class FusionServiceImpl final : public fusion::FusionService::Service
{
public:
    FusionServiceImpl();
    ~FusionServiceImpl();

    // ============================================================
    //  IMPORTANT: These members are public so main.cpp's MonitorService
    //  can access the fused tracks and mutex.
    // ============================================================
    std::unordered_map<uint32_t, fusion::FusedTrack> fused_tracks_;
    std::mutex mtx_;
    // ============================================================

    grpc::Status StreamUAV(grpc::ServerContext *context, grpc::ServerReader<sensors::UAVTelemetry> *reader, fusion::FusionAck *ack) override;
    grpc::Status StreamRadar(grpc::ServerContext *context, grpc::ServerReader<sensors::RadarDetection> *reader, fusion::FusionAck *ack) override;
    grpc::Status StreamSigint(grpc::ServerContext *context, grpc::ServerReader<sensors::SigintHit> *reader, fusion::FusionAck *ack) override;

private:
    // Arka plan thread'i ve kuyruk yönetimi
    std::thread fusion_thread_;
    bool running_ = true;
    std::deque<SensorMeasurement> queue_;
    std::mutex queue_mtx_;

    void FusionLoop();

    // Kalman ve Yardımcı Veriler
    std::unordered_map<uint32_t, KalmanFilter> kf_map_;
    std::map<uint64_t, common::GeoPoint> ground_truth_buffer_;
    std::unordered_map<std::string, uint32_t> ext_to_int_id_;
    uint32_t next_id_ = 1;
    common::GeoPoint radar_position_;

    // Helper metodlar
    uint32_t ResolveId(const std::string &ext_id);
    common::GeoPoint PolarToGeo(double range, double bearing, const common::GeoPoint &origin);
    double CalculateHaversine(double lat1, double lon1, double lat2, double lon2);
    void LogToCSV(const std::string &filename, const std::string &header, const std::string &line);
};