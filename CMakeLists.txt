cmake_minimum_required(VERSION 3.15)
project(DistributedBattlefieldSim CXX)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
# Prefer config-mode gRPC if available, but allow a fallback when the
# distribution doesn't ship a gRPCConfig.cmake.
find_package(gRPC CONFIG QUIET)

set(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto)
set(GEN_DIR ${CMAKE_SOURCE_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_DIR})

# Tüm proto dosyalarını bul
file(GLOB_RECURSE PROTO_FILES ${PROTO_DIR}/*.proto)

# Önceden üretilmiş dosyalar var mı kontrol et
file(GLOB_RECURSE PROTO_GENERATED_SRCS
    ${GEN_DIR}/*.pb.cc
    ${GEN_DIR}/*.grpc.pb.cc
)

if(PROTO_GENERATED_SRCS)
    message(STATUS "Using existing generated proto sources in ${GEN_DIR}")
    add_library(project_protos STATIC ${PROTO_GENERATED_SRCS})
else()
    message(STATUS "Generating proto files...")

    # Try several common binary names for the gRPC C++ protoc plugin.
    find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin protoc-gen-grpc grpc_cpp_plugin-1)

    # If plugin is missing but the project already contains generated gRPC sources
    # in `${GEN_DIR}`, allow configuration to continue (use pre-generated files).
    if(NOT GRPC_CPP_PLUGIN)
        file(GLOB_RECURSE EXISTING_GRPC_SRCS
            ${GEN_DIR}/*.grpc.pb.cc
            ${GEN_DIR}/*.grpc.pb.h
        )

        if(EXISTING_GRPC_SRCS)
            message(WARNING "grpc_cpp_plugin not found, but found generated gRPC sources in ${GEN_DIR}; skipping plugin requirement.")
        else()
            message(FATAL_ERROR
                "grpc_cpp_plugin not found. Install the gRPC C++ plugin or pre-generate the gRPC sources.\n"
                "On Debian/Ubuntu you may be able to install a package that provides the plugin, or build gRPC from source.\n"
                "Example apt commands to try:\n"
                "  apt-get update; apt-get install -y build-essential cmake pkg-config \"libgrpc++-dev\" \"protobuf-compiler\"\n"
                "If your distro provides a `protobuf-compiler-grpc` or similar package that installs `grpc_cpp_plugin`, install it.\n"
                "As an alternative, run `protoc --cpp_out=generated --grpc_out=generated --plugin=protoc-gen-grpc=/path/to/grpc_cpp_plugin your.proto` on a host
                and copy the generated files into the `generated/` directory so the build can proceed.")
        endif()
    endif()

    set(GENERATED_FILES "")

    foreach(_proto ${PROTO_FILES})
        file(RELATIVE_PATH REL_PATH ${PROTO_DIR} ${_proto})
        get_filename_component(DIR ${REL_PATH} DIRECTORY)
        get_filename_component(BASE ${REL_PATH} NAME_WE)

        file(MAKE_DIRECTORY ${GEN_DIR}/${DIR})

        set(OUT_PB_H   ${GEN_DIR}/${DIR}/${BASE}.pb.h)
        set(OUT_PB_CC  ${GEN_DIR}/${DIR}/${BASE}.pb.cc)
        set(OUT_GRPC_H ${GEN_DIR}/${DIR}/${BASE}.grpc.pb.h)
        set(OUT_GRPC_CC ${GEN_DIR}/${DIR}/${BASE}.grpc.pb.cc)

        add_custom_command(
            OUTPUT ${OUT_PB_H} ${OUT_PB_CC} ${OUT_GRPC_H} ${OUT_GRPC_CC}
            # If `GRPC_CPP_PLUGIN` is defined, pass it to protoc via --plugin.
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                --proto_path=${PROTO_DIR}
                --cpp_out=${GEN_DIR}
                --grpc_out=${GEN_DIR}
                $<$<BOOL:${GRPC_CPP_PLUGIN}>:--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}>
                ${REL_PATH}
            WORKING_DIRECTORY ${PROTO_DIR}
            DEPENDS ${_proto}
            COMMENT "Generating C++ sources for ${REL_PATH}"
        )

        list(APPEND GENERATED_FILES
            ${OUT_PB_H} ${OUT_PB_CC} ${OUT_GRPC_H} ${OUT_GRPC_CC}
        )
    endforeach()

    add_library(project_protos STATIC ${GENERATED_FILES})
endif()

target_include_directories(project_protos PUBLIC ${GEN_DIR} ${PROTO_DIR})

# Ensure we have imported targets for gRPC and Protobuf. Some distros
# install headers/libs but do not provide a gRPCConfig.cmake; in that
# case create imported targets pointing to the system libraries so the
# rest of the project can link against `gRPC::grpc` and
# `protobuf::libprotobuf` as expected.
if(NOT TARGET gRPC::grpc)
    find_library(GRPC_LIB NAMES grpc libgrpc)
    find_library(GRPCPP_LIB NAMES grpc++ libgrpc++)
    find_path(GRPC_INCLUDE_DIR NAMES grpcpp/grpcpp.h grpcpp/server.h)

    if(GRPC_LIB AND GRPCPP_LIB)
        add_library(gRPC::grpc UNKNOWN IMPORTED)
        set_target_properties(gRPC::grpc PROPERTIES IMPORTED_LOCATION "${GRPC_LIB}")
        add_library(gRPC::grpc++ UNKNOWN IMPORTED)
        set_target_properties(gRPC::grpc++ PROPERTIES IMPORTED_LOCATION "${GRPCPP_LIB}")
        if(GRPC_INCLUDE_DIR)
            target_include_directories(gRPC::grpc++ INTERFACE ${GRPC_INCLUDE_DIR})
            target_include_directories(gRPC::grpc INTERFACE ${GRPC_INCLUDE_DIR})
        endif()
    else()
        message(FATAL_ERROR "gRPC not found: no gRPCConfig.cmake and could not locate grpc libraries on the system.")
    endif()
endif()

if(NOT TARGET protobuf::libprotobuf)
    find_library(PROTOBUF_LIB NAMES protobuf libprotobuf)
    find_path(PROTOBUF_INCLUDE_DIR NAMES google/protobuf/stubs/common.h)
    if(PROTOBUF_LIB)
        add_library(protobuf::libprotobuf UNKNOWN IMPORTED)
        set_target_properties(protobuf::libprotobuf PROPERTIES IMPORTED_LOCATION "${PROTOBUF_LIB}")
        if(PROTOBUF_INCLUDE_DIR)
            target_include_directories(protobuf::libprotobuf INTERFACE ${PROTOBUF_INCLUDE_DIR})
        endif()
    else()
        message(FATAL_ERROR "Protobuf library not found on the system.")
    endif()
endif()

target_link_libraries(project_protos PUBLIC gRPC::grpc protobuf::libprotobuf)

# Alt servisler
add_subdirectory(services/fusion_service)
add_subdirectory(services/sensor_uav)
add_subdirectory(services/sensor_radar)
add_subdirectory(services/sensor_sigint)
add_subdirectory(services/monitor_cli)
