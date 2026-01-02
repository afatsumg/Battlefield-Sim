#include "fusion_service.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <map>
#include <numeric>

#include "geo_utils.h"
#include "utils/logging.h"

namespace
{
    constexpr double EARTH_RADIUS = 6371000.0;
}

// Kalman filter implementation moved to separate module: kalman_filter.{h,cpp}

// ==================== Fusion Service Implementation ====================

FusionServiceImpl::FusionServiceImpl()
{
    running_ = true;
    std::cout << "[FUSION] Starting Background Fusion Thread (Dynamic origin)..." << std::endl;
    fusion_thread_ = std::thread(&FusionServiceImpl::FusionLoop, this);
}

FusionServiceImpl::~FusionServiceImpl()
{
    running_ = false;
    if (fusion_thread_.joinable())
        fusion_thread_.join();
}

grpc::Status FusionServiceImpl::StreamUAV(
    grpc::ServerContext *context, grpc::ServerReader<sensors::UAVTelemetry> *reader, fusion::FusionAck *ack)
{
    sensors::UAVTelemetry msg;
    while (reader->Read(&msg))
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        queue_.push_back({(uint64_t)msg.header().timestamp(),
                          "UAV",
                          msg.uav_id(),
                          msg.position().lat(), msg.position().lon(), msg.position().alt(),
                          msg.uav_id()});
    }
    return grpc::Status::OK;
}

grpc::Status FusionServiceImpl::StreamRadar(
    grpc::ServerContext *context, grpc::ServerReader<sensors::RadarDetection> *reader, fusion::FusionAck *ack)
{
    sensors::RadarDetection msg;
    while (reader->Read(&msg))
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        // The radar client already calculates the target GPS coordinates;
        // use them directly. If the client provided per-message origin
        // instead, that should be stored per-sensor (not done here).
        queue_.push_back({(uint64_t)msg.header().timestamp(),
                          "RADAR",
                          msg.header().sensor_id(),
                          msg.radar_lat(),
                          msg.radar_lon(),
                          msg.radar_alt(),
                          ""});
    }
    return grpc::Status::OK;
}

grpc::Status FusionServiceImpl::StreamSigint(
    grpc::ServerContext *context, grpc::ServerReader<sensors::SigintHit> *reader, fusion::FusionAck *ack)
{
    sensors::SigintHit msg;
    while (reader->Read(&msg))
    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        queue_.push_back({(uint64_t)msg.header().timestamp(), "SIGINT", msg.header().sensor_id(), 0.0, 0.0, 0.0, ""});
    }
    return grpc::Status::OK;
}


void FusionServiceImpl::FusionLoop()
{
    uint64_t last_fusion_time = 0;
    double raw_uav_lat = 0.0, raw_uav_lon = 0.0, raw_uav_alt = 0.0;

    const std::string report_path = "/workspace/shared/logs/results.csv";
    const std::string header = "ts,f_lat,f_lon,uav_lat,uav_lon,error_m,sources";

    {
        std::ofstream ofs(report_path, std::ios::trunc); // ios::trunc removes old content
        if (ofs.is_open())
        {
            ofs << header << "\n";
            ofs.close();
            std::cout << "[FUSION] Log file initialized: " << report_path << std::endl;
        }
        else
        {
            std::cerr << "[ERROR] Could not initialize log file!" << std::endl;
        }
    }

    // Map to store each sensor's Sigma values coming from Docker
    // In real systems this data comes from a "Sensor Registry" service.
    std::map<std::string, double> sensor_sigma_map;

    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::deque<SensorMeasurement> batch;
        {
            std::lock_guard<std::mutex> lock(queue_mtx_);
            if (queue_.empty())
                continue;
            batch.swap(queue_);
        }

        uint32_t track_id = 1;
        uint64_t current_batch_ts = batch.back().timestamp;
        std::vector<std::string> active_sources;

        KalmanFilter &kf = kf_map_[track_id];
        double dt = (last_fusion_time == 0) ? 0.1 : (current_batch_ts - last_fusion_time) / 1000.0;
        if (dt <= 0 || dt > 1.0)
            dt = 0.1;

        kf.Predict(dt);
        last_fusion_time = current_batch_ts;

        for (const auto &m : batch)
        {
            if (m.sensor_type == "UAV")
            {
                raw_uav_lat = m.lat;
                raw_uav_lon = m.lon;
                raw_uav_alt = m.alt;
                continue;
            }

            if (std::abs(m.lat) < 1.0)
                continue;

            // --- DYNAMIC R MATRIX CALCULATION ---
            // We use RADAR_RANGE_SIGMA from docker-compose per sensor.
            // If the radar client does not send sigma info inside the message,
            // we can assign a default based on sensor_id here.

            double sigma = 30.0; // Default
            if (m.sensor_id == "TPS-77-LONG-RANGE")
            {
                sigma = 50.0;
            }
            else if (m.sensor_id == "AN-MPQ-53-PATRIOT")
            {
                sigma = 5.0;
            }

            // Kalman's R matrix is the variance: R = sigma^2
            double base_R = std::pow(sigma, 2);

            double pred_lat, pred_lon, v_lat, v_lon;
            kf.GetState(pred_lat, pred_lon, v_lat, v_lon);
            double innovation = geo_utils::CalculateHaversine(m.lat, m.lon, pred_lat, pred_lon);

            // Gating: Filter very large deviations (outliers)
            double adaptive_R = base_R;
            if (innovation > 1000.0)
            {
                // If the measurement is very distant, increase R to desensitize the filter (Outlier Rejection)
                adaptive_R = base_R * std::pow(innovation / 500.0, 2);
            }

            kf.Update(m.lat, m.lon, adaptive_R);

            if (std::find(active_sources.begin(), active_sources.end(), m.sensor_id) == active_sources.end())
                active_sources.push_back(m.sensor_id);
        }

        double f_lat, f_lon, f_v_lat, f_v_lon;
        kf.GetState(f_lat, f_lon, f_v_lat, f_v_lon);

        // --- VALIDATION & LOGGING ---
        if (active_sources.empty() || (f_lat == 0.0 && f_lon == 0.0))
        {
            continue;
        }

        double error_m = (raw_uav_lat != 0.0) ? geo_utils::CalculateHaversine(f_lat, f_lon, raw_uav_lat, raw_uav_lon) : 0.0;

        // Send to Monitor service
        {
            std::lock_guard<std::mutex> lock(mtx_);
            fusion::FusedTrack &ft = fused_tracks_[track_id];
            ft.set_track_id(track_id);
            ft.mutable_position()->set_lat(f_lat);
            ft.mutable_position()->set_lon(f_lon);
            ft.mutable_position()->set_alt(raw_uav_alt != 0 ? raw_uav_alt : 1250.0);
            ft.set_confidence(0.95);
            ft.clear_source_sensors();
            for (const auto &s : active_sources)
                ft.add_source_sensors(s);
        }

        // CSV Logging
        std::stringstream ss;
        ss << current_batch_ts << "," << std::fixed << std::setprecision(6) << f_lat << "," << f_lon << ","
           << raw_uav_lat << "," << raw_uav_lon << "," << std::fixed << std::setprecision(2) << error_m << ",";
        for (size_t i = 0; i < active_sources.size(); ++i)
            ss << active_sources[i] << (i < active_sources.size() - 1 ? ";" : "");

        utils::LogToCSV(report_path, ss.str(), mtx_);
    }
}



void FusionServiceImpl::StartTimeoutThread(int duration_sec)
{
    if (duration_sec <= 0)
        return;

    std::thread([this, duration_sec]()
                {
                    std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
                    std::cout << "[FUSION] Simulation duration reached. Shutting down..." << std::endl;
                    this->running_ = false;                               // Stop the fusion loop
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait a moment for writing final logs
                    std::exit(0);                                         // For stopping the container
                })
        .detach();
}