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

double CalculateDopplerShift(double range_rate, double carrier_freq_hz)
{
    const double SPEED_OF_LIGHT = 299792458.0; // m/s
    
    // Doppler shift formula: Δf = 2 * v_radial * f_c / c
    // Factor of 2 because radar signal goes to target and back
    return 2.0 * range_rate * carrier_freq_hz / SPEED_OF_LIGHT;
}

double CalculateRainAttenuation(double frequency_ghz, double range_km, double rain_rate_mmh)
{
    // No attenuation if no rain
    if (rain_rate_mmh < 0.1) return 0.0;

    // ITU-R P.838 simplified rain attenuation model
    // A = k * R^alpha * distance
    // where R = rain rate (mm/h), coefficients vary by frequency band
    
    // S-band (2-4 GHz) coefficients
    double k = 0.0000075;   // coefficient
    double alpha = 0.63;    // exponent (frequency dependent, ~0.6-0.65 for S-band)
    
    // Attenuation per km (dB/km)
    double atten_per_km = k * std::pow(rain_rate_mmh, alpha);
    
    // Total two-way attenuation (signal goes down and reflects back)
    return 2.0 * atten_per_km * range_km;
}



} // namespace physics
