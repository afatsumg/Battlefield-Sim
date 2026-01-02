#include "physics.h"
#include "geo_utils.h"
#include <cmath>

namespace physics {

double CalculateAspectRCS(double uav_lat, double uav_lon, double uav_heading,
                          double radar_lat, double radar_lon)
{
    // Bearing from radar to UAV
    double bearing_to_uav = geo_utils::BearingDegrees(radar_lat, radar_lon, uav_lat, uav_lon);

    // Aspect Angle: Difference between where UAV points and where Radar is
    double alpha_rad = std::abs(uav_heading - bearing_to_uav) * M_PI / 180.0;

    // Small UAV RCS Model: Frontal (0.1 m^2) vs Side (2.0 m^2)
    double rcs_min = 0.1;
    double rcs_max = 2.0;
    return rcs_min * std::pow(std::cos(alpha_rad), 2) + rcs_max * std::pow(std::sin(alpha_rad), 2);
}

double CalculateSignalStrength(double rcs, double range)
{
    return rcs / std::pow(range, 4);
}

} // namespace physics
