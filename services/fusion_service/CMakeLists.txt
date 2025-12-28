cmake_minimum_required(VERSION 3.15)
project(fusion_service CXX) 

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.hpp)

add_executable(fusion_service ${SOURCES} ${HEADERS})

target_include_directories(fusion_service
    PRIVATE
        ${CMAKE_SOURCE_DIR}/generated       
        ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenCV REQUIRED)

find_package(Threads REQUIRED)

target_link_libraries(fusion_service
    PRIVATE
        project_protos 

        gRPC::grpc++
        protobuf::libprotobuf
        
        yaml-cpp
        opencv_core

    Threads::Threads
    ${UTF8_RANGE_LIB}
)