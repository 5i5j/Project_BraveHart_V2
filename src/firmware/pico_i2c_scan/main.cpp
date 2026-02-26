#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

// 包含官方库头文件 (Include Official Library Headers)
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_pico_hal.h" // 确保你的 HAL 文件已经更新了引脚定义

// --- 硬件定义 (Hardware Definitions) ---
#define PIN_RST 10
#define PIN_INT 11
#define I2C_INST i2c1
#define BNO_ADDR 0x4A
#define ENCODER_L_PIN 12
#define ENCODER_R_PIN 14


volatile uint32_t last_arrival_time_us = 0;
volatile uint32_t current_delta_us = 0;
uint32_t max_jitter_us = 0;

extern "C" void vApplicationMallocFailedHook(void) {
    panic("Malloc Failed");
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    panic("Stack Overflow: %s", pcTaskName);
}

// 全局姿态变量 (Global Pose Variables)
struct {
    float roll, pitch, yaw;
    bool updated;
} imu_pose = {0, 0, 0, false};

// --- 💡 关键：欧拉角转换函数 (Quaternion to Euler) ---
void update_euler(float i, float j, float k, float r) {
    float sqw = r * r;
    float sqx = i * i;
    float sqy = j * j;
    float sqz = k * k;

    imu_pose.roll  = atan2(2.0f * (i * j + k * r), (sqx - sqy - sqz + sqw)) * 57.29577f;
    imu_pose.pitch = asin(-2.0f * (i * k - j * r) / (sqx + sqy + sqz + sqw)) * 57.29577f;
    imu_pose.yaw   = atan2(2.0f * (j * k + i * r), (-sqx - sqy + sqz + sqw)) * 57.29577f;
    imu_pose.updated = true;
}

// --- 💡 关键：传感器事件回调 (Sensor Event Callback) ---
void sensorHandler(void *cookie, sh2_SensorEvent_t *event) {
    sh2_SensorValue_t value;
    if (sh2_decodeSensorEvent(&value, event) == SH2_OK) {
        if (value.sensorId == SH2_GAME_ROTATION_VECTOR) {
            uint32_t now = to_us_since_boot(get_absolute_time());
            if (last_arrival_time_us != 0) {
                current_delta_us = now - last_arrival_time_us;
            }
            last_arrival_time_us = now;
            
            // 更新欧拉角逻辑...
            update_euler(value.un.gameRotationVector.i, 
                         value.un.gameRotationVector.j, 
                         value.un.gameRotationVector.k, 
                         value.un.gameRotationVector.real);
        }
    }
}

// --- Core 1: 专用驱动进程 (Dedicated Driver Process) ---
void core1_entry() {
    // 1. 基础硬件初始化 (GP6, GP7, GP10, GP11)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_set_function(7, GPIO_FUNC_I2C);
    gpio_pull_up(6); gpio_pull_up(7);

    // 2. 物理复位
    gpio_init(PIN_RST); gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0); sleep_ms(200);
    gpio_put(PIN_RST, 1); sleep_ms(1000); 

    sh2_Hal_t* hal = get_sh2_pico_hal();
    if (sh2_open(hal, NULL, NULL) != SH2_OK) return;

    // 💡 3. 强制清空：直到 INT 引脚变高 (Idle) 之后再等一下
    printf(">> [C1] Force draining sensor buffer...\n");
    uint32_t drain_start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - drain_start < 2000) {
        sh2_service(); // 不断处理传感器发出的任何包
        if (gpio_get(PIN_INT) != 0) break; // 如果 INT 变高，说明暂时没数据了
        sleep_ms(2);
    }

    sh2_setSensorCallback(sensorHandler, NULL);
    sh2_SensorConfig_t config = {.reportInterval_us = 20000}; // 50Hz

    // 💡 4. 带重试的订阅 (The Bulletproof Subscription)
    printf(">> [C1] Attempting Subscription with retries...\n");
    int status = -1;
    int retry_count = 0;
    
    while (status != SH2_OK && retry_count < 10) {
        status = sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &config);
        if (status != SH2_OK) {
            printf(">> [C1] Config failed (%d), retrying %d/10...\n", status, retry_count + 1);
            // 失败了就再跑几次 service，把阻碍 Config 的那个包“挤”出来
            for(int j=0; j<20; j++) sh2_service(); 
            sleep_ms(200);
            retry_count++;
        }
    }

    if (status == SH2_OK) {
        printf(">> [C1] SUCCESS! Sensor active.\n");
    } else {
        printf(">> [C1] FATAL: All retries failed. Resetting...\n");
        // 如果 10 次都失败，通常意味着 I2C 频率还是太快或上拉不够
    }

    while (true) {
        sh2_service();
    }
}

// --- Core 0: 业务逻辑进程 (Business Logic Process) ---
int main() {
    stdio_init_all();
    sleep_ms(2000); // 等待串口连接 (Wait for USB Serial)

    printf("\n--- BNO085 STABLE STARTUP ---\n");

    // 启动 Core 1 处理硬件 (Launch Core 1)
    multicore_launch_core1(core1_entry);

    while (true) {
        if (imu_pose.updated) {
            // 计算偏离目标 20000us 的差值
            int32_t jitter = (int32_t)current_delta_us - 20000;
        
            printf(">> [POSE] R:%6.2f P:%6.2f Y:%6.2f | dt:%uus | jitter:%dus\n", 
               imu_pose.roll, imu_pose.pitch, imu_pose.yaw, 
               current_delta_us, jitter);
               
            imu_pose.updated = false;
        }
        sleep_ms(5); // 稍微快一点的检查频率
    }
}