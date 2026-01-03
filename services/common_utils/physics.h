#pragma once

namespace physics {

// Calculate Dynamic RCS based on aspect angle (heading difference)
// uav_lat/lon: UAV position
// uav_heading: UAV heading in degrees (0-360)
// radar_lat/lon: Radar position
// Returns RCS in m^2
double CalculateAspectRCS(double uav_lat, double uav_lon, double uav_heading,
                          double radar_lat, double radar_lon);

// Calculate signal strength based on RCS and range
// Returns signal_strength = rcs / range^4
double CalculateSignalStrength(double rcs, double range);

// === ADVANCED RADAR PHYSICS ===

// Calculate Doppler frequency shift
// range_rate: closure velocity (m/s)
// carrier_freq_hz: Radar carrier frequency in Hz (e.g., 3e9 for S-band)
// Returns: Doppler shift in Hz
double CalculateDopplerShift(double range_rate, double carrier_freq_hz);

// Calculate rain attenuation loss
// frequency_ghz: Radar frequency in GHz (3=S-band, 10=X-band)
// range_km: Distance to target in km
// rain_rate_mmh: Rainfall rate in mm/hour (0=no rain, 10=heavy)
// Returns: Two-way attenuation in dB (positive = loss)
double CalculateRainAttenuation(double frequency_ghz, double range_km, double rain_rate_mmh);


} // namespace physics
