cmake_minimum_required(VERSION 3.13)

# 1. 显式设置 SDK 路径
# 使用 CMAKE_CURRENT_LIST_DIR 确保路径始终相对于当前文件
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/src/firmware/pico-sdk")

# 2. 引入 SDK 导入脚本
if (NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Pico SDK not found at ${PICO_SDK_PATH}. Please check your path.")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

# 3. 交叉编译防御设置
# 防止在编译器检查阶段尝试链接导致失败
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 4. Picotool 设置
# 尝试在系统中寻找 picotool，如果没找到再使用你指定的默认路径
find_program(PICOTOOL_EXECUTABLE NAMES picotool)
if(NOT PICOTOOL_EXECUTABLE)
    set(PICOTOOL_EXECUTABLE /usr/local/bin/picotool CACHE FILEPATH "Path to picotool" FORCE)
endif()

project(BraveHart_V2_Project C CXX ASM)

# 5. 初始化 SDK
pico_sdk_init()

# 6. 添加子项目
# 以后你可以在这里添加更多模块，如 src/firmware/pico_control
add_subdirectory(src/firmware/pico_i2c_scan)