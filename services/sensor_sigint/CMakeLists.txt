cmake_minimum_required(VERSION 3.15)

set(TARGET sensor_sigint)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS include/*.h) 

add_executable(${TARGET} ${SOURCES} ${HEADERS})

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/generated # Proto headers
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

target_link_libraries(${TARGET}
    PRIVATE
        project_protos  
        gRPC::grpc++
        protobuf::libprotobuf
        ${UTF8_RANGE_LIB}
)