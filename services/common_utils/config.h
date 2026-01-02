#pragma once

#include <string>

namespace utils {

// Parse environment variable as double, return default if not set or invalid
double GetEnvDouble(const std::string& key, double default_val);

// Parse environment variable as string, return default if not set
std::string GetEnvString(const std::string& key, const std::string& default_val);

} // namespace utils
