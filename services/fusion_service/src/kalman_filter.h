#pragma once

#include <opencv2/core.hpp>

// Simple 2D Kalman filter keeping state as [lat, lon, v_lat, v_lon].
// This class is separated from fusion_service to keep responsibilities clear.
class KalmanFilter {
public:
    KalmanFilter();
    void Initialize(double lat, double lon);
    void Predict(double dt_seconds);
    void Update(double meas_lat, double meas_lon, double noise_scale);
    void GetState(double &lat, double &lon, double &v_lat, double &v_lon) const;
    double GetCovarianceTrace() const;

private:
    cv::Mat state_, P_, Q_, R_;
    bool initialized_ = false;
};
