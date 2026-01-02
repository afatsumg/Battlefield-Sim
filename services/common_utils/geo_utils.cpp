#include "geo_utils.h"
#include <cmath>

namespace {
    constexpr double EARTH_RADIUS = 6371000.0;
}

namespace geo_utils {

double CalculateHaversine(double lat1, double lon1, double lat2, double lon2)
{
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = pow(sin(dLat / 2), 2) + pow(sin(dLon / 2), 2) * cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0);
    return EARTH_RADIUS * 2 * asin(sqrt(a));
}

double BearingDegrees(double lat1, double lon1, double lat2, double lon2)
{
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double lam1 = lon1 * M_PI / 180.0;
    double lam2 = lon2 * M_PI / 180.0;
    double y = sin(lam2 - lam1) * cos(phi2);
    double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(lam2 - lam1);
    double bearing = atan2(y, x) * 180.0 / M_PI;
    return (bearing < 0) ? bearing + 360.0 : bearing;
}

} // namespace geo_utils
