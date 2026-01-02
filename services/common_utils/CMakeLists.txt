cmake_minimum_required(VERSION 3.15)

# Library for shared utilities across sensor services
add_library(common_utils
    config.cpp
    geo_utils.cpp
    physics.cpp
)

target_include_directories(common_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link math library for physics calculations
target_link_libraries(common_utils
    PRIVATE
        m
)
