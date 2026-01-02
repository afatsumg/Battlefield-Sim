#pragma once

namespace geo_utils {

// Calculate bearing (in degrees 0-360) from point 1 to point 2
double BearingDegrees(double lat1, double lon1, double lat2, double lon2);

// Calculate haversine distance between two points in meters
double CalculateHaversine(double lat1, double lon1, double lat2, double lon2);

} // namespace geo_utils
