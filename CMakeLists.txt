cmake_minimum_required(VERSION 3.13)

# 1. Explicitly set SDK path
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/src/firmware/pico-sdk")
# Add micro-ROS SDK path (New for micro-ROS)
set(MICRO_ROS_PICO_SDK "${CMAKE_CURRENT_LIST_DIR}/src/firmware/micro_ros_raspberrypi_pico_sdk")

# 2. Include SDK import script
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

# 3. Cross-compilation setup
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(BraveHart_V2_Project C CXX ASM)

# 4. Initialize Pico SDK
pico_sdk_init()

# 5. Include micro-ROS (Essential for the "Cerebellum" Architecture)
include(${MICRO_ROS_PICO_SDK}/pico_micro_ros.cmake)

# 6. Add subdirectories
add_subdirectory(src/firmware/pico_i2c_scan)