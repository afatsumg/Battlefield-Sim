cmake_minimum_required(VERSION 3.15)
project(fusion_service CXX) 

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.hpp)

# Exclude config and physics from local sources (use common_utils instead)
list(FILTER SOURCES EXCLUDE REGEX "src/utils/(config|physics)\\.cpp$")

add_executable(fusion_service ${SOURCES} ${HEADERS})

target_include_directories(fusion_service
    PRIVATE
        ${CMAKE_SOURCE_DIR}/generated       
        ${CMAKE_SOURCE_DIR}/services/common_utils
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenCV REQUIRED)

find_package(Threads REQUIRED)

target_link_libraries(fusion_service
    PRIVATE
        common_utils
        project_protos 

        gRPC::grpc++
        protobuf::libprotobuf
        
        yaml-cpp
        opencv_core

    Threads::Threads
    ${UTF8_RANGE_LIB}
)