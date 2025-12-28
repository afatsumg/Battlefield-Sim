#include "kalman_filter.h"
#include <opencv2/core.hpp>

namespace {
    constexpr double DEFAULT_Q = 0.01; // Increased slightly to allow maneuverability
}

KalmanFilter::KalmanFilter()
{
    state_ = cv::Mat::zeros(4, 1, CV_64F);
    P_ = cv::Mat::eye(4, 4, CV_64F) * 100.0;
    Q_ = cv::Mat::eye(4, 4, CV_64F) * DEFAULT_Q;
    R_ = cv::Mat::eye(2, 2, CV_64F) * 0.1;
}

void KalmanFilter::Initialize(double lat, double lon)
{
    if (initialized_)
        return;
    state_.at<double>(0) = lat;
    state_.at<double>(1) = lon;
    state_.at<double>(2) = 0; // v_lat
    state_.at<double>(3) = 0; // v_lon
    initialized_ = true;
}

void KalmanFilter::Predict(double dt)
{
    if (!initialized_)
        return;

    cv::Mat F = cv::Mat::eye(4, 4, CV_64F);
    F.at<double>(0, 2) = dt;
    F.at<double>(1, 3) = dt;

    state_ = F * state_;
    P_ = F * P_ * F.t() + Q_;
}

void KalmanFilter::Update(double meas_lat, double meas_lon, double noise_scale)
{
    if (!initialized_)
    {
        Initialize(meas_lat, meas_lon);
        return;
    }

    cv::Mat H = cv::Mat::zeros(2, 4, CV_64F);
    H.at<double>(0, 0) = 1.0;
    H.at<double>(1, 1) = 1.0;

    cv::Mat R_curr = R_ * noise_scale;
    cv::Mat z = (cv::Mat_<double>(2, 1) << meas_lat, meas_lon);
    cv::Mat y = z - H * state_;
    cv::Mat S = H * P_ * H.t() + R_curr;
    cv::Mat K = P_ * H.t() * S.inv();

    state_ = state_ + K * y;
    P_ = (cv::Mat::eye(4, 4, CV_64F) - K * H) * P_;
}

void KalmanFilter::GetState(double &lat, double &lon, double &v_lat, double &v_lon) const
{
    lat = state_.at<double>(0);
    lon = state_.at<double>(1);
    v_lat = state_.at<double>(2);
    v_lon = state_.at<double>(3);
}

double KalmanFilter::GetCovarianceTrace() const
{
    return cv::trace(P_)[0];
}
