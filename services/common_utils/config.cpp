#include "config.h"
#include <cstdlib>

namespace utils {

double GetEnvDouble(const std::string& key, double default_val)
{
    const char* val = std::getenv(key.c_str());
    if (!val) return default_val;
    try {
        return std::stod(val);
    } catch (...) {
        return default_val;
    }
}

std::string GetEnvString(const std::string& key, const std::string& default_val)
{
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : default_val;
}

} // namespace utils
