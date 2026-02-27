/**
 * File: pico_main.cpp
 * Architecture: Dual-core for non-blocking sensor fusion
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "quadrature_encoder.h" // From your pio_encoder dir

// --- Binary Protocol Structure ---
#pragma pack(push, 1)
struct TelemetryPacket{
    uint8_t head[2] = {0xAA, 0x55};
    unint32_t timestamp;
    float roll, pitch, yaw;
    int32_t l_ticks, r_ticks;
    unint8_t checksum;
};
#pragma pack(pop)

// 传感器数据共享结构（Shared Sensor Data
struct{
    float roll, pitch, yaw;
    int32_t l_ticks, r_ticks;
    bool imu_updated;
} shared_data = {0, 0, 0, 0, 0, false};

static mutex_t data_mutex;

// Mutex for data safety (双核共享数据互斥锁)
static mutex_t sensor_mutex;

// Global sensor data (全局传感器数据结构)
struct {
    float yaw;
    float distance_cm;
    int32_t left_ticks;
    int32_t right_ticks;
} robot_data;

/**
 * Core 1 Task: Ultrasonic Measurement (从核心任务：超声波测距)
 * Running independently to prevent blocking Core 0
 */
void core1_entry() {
    while (1) {
        // Trigger Ultrasonic and measure pulse (执行超声波测距)
        float dist = measure_ultrasonic_blocking(); // This takes ~20ms

        mutex_enter_blocking(&sensor_mutex);
        robot_data.distance_cm = dist;
        mutex_exit(&sensor_mutex);

        sleep_ms(50); // Sample ultrasonic at 20Hz (超声波采样率为20Hz)
    }
}

/**
 * Core 0 Task: IMU, Encoders, and Serial Comm (主核心任务：IMU、编码器与通信)
 */
int main() {
    stdio_init_all();
    mutex_init(&sensor_mutex);

    // 1. Initialize BNO085 (初始化 IMU)
    // 2. Initialize PIO Encoders (初始化 PIO 正交编码器)
    
    // Launch Core 1 (启动第二核)
    multicore_launch_core1(core1_entry);

    while (1) {
        // A. Read BNO085 (Fast, Non-blocking) (高速读取姿态)
        // B. Read PIO Encoder (Hardware-level, Fast) (读取硬件级编码器数据)
        
        // C. Send packaged binary data to Pi 3B (向树莓派发送二进制数据包)
        printf("DATA_START,%.2f,%d,%d,%.2f,DATA_END\n", 
               robot_data.yaw, 
               robot_data.left_ticks, 
               robot_data.right_ticks, 
               robot_data.distance_cm);

        sleep_ms(20); // Maintain 50Hz Loop (保持50Hz控制循环)
    }
}