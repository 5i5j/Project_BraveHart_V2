# 1. 准确定位项目根目录
set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# 2. 定义各模块的基准路径
set(BNO_LIB_DIR "${PROJ_ROOT}/src/firmware/BNO08x_Pico_Library")
set(SH2_DIR "${BNO_LIB_DIR}/sh2")
set(FREERTOS_KERNEL_PATH "${PROJ_ROOT}/src/firmware/FreeRTOS-Kernel")

# 3. 收集 IMU 驱动源文件
set(IMU_DRV_SOURCES
    ${SH2_DIR}/src/sh2.c
    ${SH2_DIR}/src/sh2_SensorValue.c
    ${SH2_DIR}/src/sh2_util.c
    ${SH2_DIR}/src/shtp.c
    ${SH2_DIR}/src/sh2_pico_hal.cpp
    ${PROJ_ROOT}/src/firmware/pico/utils.cpp
)

# 4. 收集 FreeRTOS 源文件
set(FREERTOS_SOURCES
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_KERNEL_PATH}/stream_buffer.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/port.c
)

# 5. 创建目标
add_executable(bno_test 
    chatgpt_main.cpp 
    ${IMU_DRV_SOURCES}
    ${FREERTOS_SOURCES}
)

# 6. 配置包含路径
target_include_directories(bno_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SH2_DIR}/include
    ${SH2_DIR}/src
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include
    ${PROJ_ROOT}/src/firmware/pico
)

# 💡 关键补充：为 FreeRTOS 设置必要的宏定义 (Ensure FreeRTOS can see your config)
# 如果 main.cpp 目录下有 FreeRTOSConfig.h，必须让编译器找到它
target_compile_definitions(bno_test PRIVATE
    # 有些库可能需要这个宏来区分环境
    PICO_RP2040=1
)

# 7. 链接 Pico SDK 库
target_link_libraries(bno_test 
    pico_stdlib
    pico_multicore
    hardware_i2c
    hardware_gpio
    hardware_irq
    hardware_exception
)

# 8. 💡 串口配置 (这里是重中之重)
pico_enable_stdio_usb(bno_test 1)
pico_enable_stdio_uart(bno_test 0)

# 9. 💡 额外保险：生成多种格式，确保可以被 picotool 识别
pico_add_extra_outputs(bno_test)