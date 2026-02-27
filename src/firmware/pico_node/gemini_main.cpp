#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_pico_hal.h"

// -------------------- 硬件定义 --------------------
#define PIN_RST 10
#define PIN_INT 11
#define I2C_INST i2c1
#define TARGET_PERIOD_US 20000 

// -------------------- 统计记录定义 --------------------
#define LOG_SIZE 1000
volatile int32_t jitter_array[LOG_SIZE];
volatile uint32_t current_idx = 0;
volatile bool recording_enabled = false;

// -------------------- 全局标志位 --------------------
volatile bool data_ready_flag = false;
volatile uint32_t last_arrival_time_us = 0;

// -------------------- 中断服务函数 (ISR) --------------------
// 当 BNO085 拉低 INT 引脚时触发
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == PIN_INT) {
        data_ready_flag = true; 
    }
}

// -------------------- 传感器回调 --------------------
void sensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    sh2_SensorValue_t value;
    if (sh2_decodeSensorEvent(&value, event) == SH2_OK)
    {
        if (value.sensorId == SH2_GAME_ROTATION_VECTOR)
        {
            uint32_t now = to_us_since_boot(get_absolute_time());
            
            if (recording_enabled && current_idx < LOG_SIZE && last_arrival_time_us != 0) {
                uint32_t dt = now - last_arrival_time_us;
                int32_t jitter = (int32_t)dt - TARGET_PERIOD_US;
                jitter_array[current_idx++] = jitter;
            }

            last_arrival_time_us = now;
        }
    }
}

// -------------------- Core1: 精准驱动 --------------------
void core1_entry()
{
    // 💡 优化 1：升级 I2C 到 400kHz (Fast Mode)
    i2c_init(i2c1, 400 * 1000); 
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_set_function(7, GPIO_FUNC_I2C);
    gpio_pull_up(6);
    gpio_pull_up(7);

    // 物理复位逻辑保持不变...
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(200);
    gpio_put(PIN_RST, 1);
    sleep_ms(1000);

    // 💡 优化 2：配置 GPIO 中断 (INT 引脚)
    gpio_init(PIN_INT);
    gpio_set_dir(PIN_INT, GPIO_IN);
    gpio_pull_up(PIN_INT);
    // 监听下降沿 (Falling Edge)
    gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    sh2_Hal_t* hal = get_sh2_pico_hal();
    if (sh2_open(hal, NULL, NULL) != SH2_OK) return;

    sh2_setSensorCallback(sensorHandler, NULL);
    sh2_SensorConfig_t config = {};
    config.reportInterval_us = TARGET_PERIOD_US;
    while (sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &config) != SH2_OK) {
        sleep_ms(200);
    }

    while (true)
    {
        // 💡 只有当 INT 中断触发时，才调用 sh2_service
        if (data_ready_flag) {
            data_ready_flag = false;
            sh2_service();
        }
        // 保持极短的休眠或 tight_loop 避免 Core 1 过热
        tight_loop_contents();
    }
}


// main 函数逻辑保持你的统计输出版本即可...
// -------------------- Core0: Logic & Analysis --------------------
int main()
{
    stdio_init_all();
    sleep_ms(3000);

    printf("\n=== PHASE 1: SILENT JITTER DISTRIBUTION TEST ===\n");
    
    multicore_launch_core1(core1_entry);
    
    // Step 1: Warm up (Wait for sensor to stabilize)
    printf("Warming up sensor for 5 seconds...\n");
    sleep_ms(5000);

    // Step 2: Start Silent Recording
    printf("Recording started... (Core 0 will be silent for ~12 seconds)\n");
    current_idx = 0;
    recording_enabled = true;

    // Monitor progress silently (minimal CPU impact)
    while (current_idx < LOG_SIZE)
    {
        tight_loop_contents();
    }
    
    recording_enabled = false;
    printf("Recording finished! Analyzing results...\n\n");

    // Step 3: Analysis & CSV Output
    int32_t max_j = -100000;
    int32_t min_j = 100000;
    int64_t sum_j = 0;

    printf("index,jitter_us\n");
    for (uint32_t i = 0; i < current_idx; i++)
    {
        int32_t val = jitter_array[i];
        printf("%lu,%ld\n", i, val);
        
        if (val > max_j) max_j = val;
        if (val < min_j) min_j = val;
        sum_j += val;
    }

    printf("\n--- SUMMARY ---\n");
    printf("Total Samples: %lu\n", current_idx);
    printf("Max Jitter:    %ld us\n", max_j);
    printf("Min Jitter:    %ld us\n", min_j);
    printf("Avg Jitter:    %.2f us\n", (float)sum_j / current_idx);
    printf("----------------\n");

    while (true) { tight_loop_contents(); }
}