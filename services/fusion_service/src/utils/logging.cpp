#include "utils/logging.h"
#include <fstream>
#include <iostream>

namespace utils {

void LogToCSV(const std::string &path, const std::string &line, std::mutex &mtx)
{
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream ofs(path, std::ios::app);
    if (ofs.is_open())
    {
        ofs << line << "\n";
    }
    else
    {
        std::cerr << "[ERROR] Could not write log file: " << path << std::endl;
    }
}

} // namespace utils
