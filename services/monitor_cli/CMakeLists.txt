# services/monitor_cli/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(monitor_cli CXX)

set(TARGET monitor_cli)

# Compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES src/*.cpp)

add_executable(${TARGET} ${SOURCES})

# Include generated proto headers and local sources
target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Find and link required packages
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

target_link_libraries(${TARGET}
    PRIVATE
        project_protos
        gRPC::grpc++
        protobuf::libprotobuf
)

if(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic)
endif()