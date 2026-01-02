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

} // namespace physics
