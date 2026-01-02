#pragma once

#include <string>
#include <mutex>

namespace utils {

void LogToCSV(const std::string &path, const std::string &line, std::mutex &mtx);

} // namespace utils
