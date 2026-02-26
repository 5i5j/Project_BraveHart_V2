# CMakeLists.txt for Pico Main Node
cmake_minimum_required(VERSION 3.13)

# Pull in PICO SDK (引入 SDK 标准配置)
include(pico_sdk_import.cmake)

project(pico_node C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Define the executable (定义可执行文件)
add_executable(pico_node
    pico_main.cpp
)

# Add include directories for peer libraries (添加同级库的头文件路径)
target_include_directories(pico_node PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../pio_encoder
    ${CMAKE_CURRENT_SOURCE_DIR}/../BNO08x_Pico_Library/include
)

# Link the libraries (链接外部库与 SDK 组件)
# 注意：确保 pio_encoder 和 BNO 库文件夹内有自己的 CMakeLists 并在上层定义了库名
target_link_libraries(pico_node 
    pico_stdlib 
    hardware_pwm
    hardware_i2c
    hardware_pio
    hardware_timer
    pico_multicore # 开启双核支持 (Dual-core support)
)

# Enable USB serial output (启用 USB 串口输出，禁用 UART 串口)
pico_enable_stdio_usb(pico_node 1)
pico_enable_stdio_uart(pico_node 0)

# Create extra build artifacts (生成 .uf2 文件)
pico_add_extra_outputs(pico_node)