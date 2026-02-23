# src/firmware/pico_i2c_scan/CMakeLists.txt

# 1. Define executable
add_executable(i2c_scan 
    main.cpp
)

# 2. Link libraries (Ordered by importance)
target_link_libraries(i2c_scan 
    pico_stdlib 
    hardware_i2c
    hardware_gpio
    pico_micro_ros            # Link micro-ROS library (Target name defined in pico_micro_ros.cmake)
    FreeRTOS-Kernel           # need this to use vTaskStartScheduler()
)

# 3. Include directories
target_include_directories(i2c_scan PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 4. Auxiliary configuration
pico_enable_stdio_usb(i2c_scan 1)
pico_enable_stdio_uart(i2c_scan 0)

# 5. Generate firmware artifacts
pico_add_extra_outputs(i2c_scan)