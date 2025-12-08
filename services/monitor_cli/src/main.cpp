#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

#include "monitor_cli.h"

int main(int argc, char** argv) {
    std::string monitor_addr = "localhost:6005";
    if (argc > 1) monitor_addr = argv[1];

    std::cout << "Monitor CLI connecting to " << monitor_addr << "..." << std::endl;
    MonitorCLI cli(monitor_addr);
    cli.Run();

    return 0;
}