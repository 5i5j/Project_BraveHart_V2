# src/firmware/pico_i2c_scan/CMakeLists.txt

# 1. 定义可执行文件
add_executable(i2c_scan 
    main.cpp
)

# 2. 包含路径 (使用相对路径更安全)
target_include_directories(i2c_scan PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 3. 链接基础硬件库
target_link_libraries(i2c_scan 
    pico_stdlib 
    hardware_i2c 
)

# 4. 辅助功能配置
# 启用 USB 输出，这样你可以通过 MacBook 的终端看到 I2C 扫描结果
pico_enable_stdio_usb(i2c_scan 1)
pico_enable_stdio_uart(i2c_scan 0)

# 5. 生成 .uf2 文件 (用于拖拽烧录)
pico_add_extra_outputs(i2c_scan)