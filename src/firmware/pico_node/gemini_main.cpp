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

// -------------------- 扫频参数定义 --------------------
uint32_t sweep_freqs[] = {50, 100, 200, 400}; // 使用 BNO085 支持的标准 ODR，避免混叠
#define FREQ_COUNT (sizeof(sweep_freqs) / sizeof(sweep_freqs[0]))
#define SAMPLES_PER_TEST 500  // 每个频率采集500个样本（节省内存并提高效率）

// -------------------- 统计记录定义 --------------------
volatile int32_t jitter_array[SAMPLES_PER_TEST];
volatile uint32_t current_idx = 0;
volatile bool recording_enabled = false;
volatile uint32_t current_target_period_us = 10000; // 动态目标周期
volatile bool config_update_requested = false;     // 配置更新请求标志

// -------------------- 全局标志位 --------------------
volatile bool data_ready_flag = false;
volatile uint32_t last_arrival_time_us = 0;

// -------------------- 中断服务函数 (ISR) --------------------
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
            
            if (recording_enabled && current_idx < SAMPLES_PER_TEST && last_arrival_time_us != 0) {
                uint32_t dt = now - last_arrival_time_us;
                // 使用当前的动态周期计算 jitter
                int32_t jitter = (int32_t)dt - current_target_period_us;
                jitter_array[current_idx++] = jitter;
            }
            last_arrival_time_us = now;
        }
    }
}

// -------------------- Core1: 处理传感器通信 --------------------
void core1_entry()
{
    i2c_init(I2C_INST, 400 * 1000); 
    gpio_set_function(6, GPIO_FUNC_I2C);
    gpio_set_function(7, GPIO_FUNC_I2C);
    gpio_pull_up(6);
    gpio_pull_up(7);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(200);
    gpio_put(PIN_RST, 1);
    sleep_ms(1000);

    gpio_init(PIN_INT);
    gpio_set_dir(PIN_INT, GPIO_IN);
    gpio_pull_up(PIN_INT);
    gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    sh2_Hal_t* hal = get_sh2_pico_hal();
    if (sh2_open(hal, NULL, NULL) != SH2_OK) return;
    sh2_setSensorCallback(sensorHandler, NULL);

    while (true)
    {
        // 检查是否有来自 Core 0 的频率更新请求
        if (config_update_requested) {
            sh2_SensorConfig_t config = {};
            config.reportInterval_us = current_target_period_us;
            sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &config);
            config_update_requested = false; // 更新完成
        }

        // 持续轮询服务，不论 flag 状态，确保最高响应速度
        if (data_ready_flag) {
            data_ready_flag = false;
        }
        sh2_service();
        tight_loop_contents();
    }
}

// -------------------- Core0: 扫频逻辑与数据分析 --------------------
int main()
{
    stdio_init_all();
    sleep_ms(5000); // 给串口助手留出连接时间

    printf("\n=== BNO085 FULL SPECTRUM JITTER SWEEP TEST ===\n");
    printf("Total Frequency Points: %d\n", FREQ_COUNT);
    
    multicore_launch_core1(core1_entry);

    for (uint i = 0; i < FREQ_COUNT; i++) {
        uint32_t freq = sweep_freqs[i];
        current_target_period_us = 1000000 / freq;

        printf("\n>>> TESTING FREQUENCY: %d Hz (Target: %u us)\n", freq, current_target_period_us);
        
        // 1. 请求 Core 1 更新配置
        config_update_requested = true;
        while(config_update_requested) { tight_loop_contents(); } // 等待配置生效

        // 2. 热身稳定 (Warm up)
        printf("Wait 3s for stabilization...\n");
        sleep_ms(3000);

        // 3. 静默采集 (Silent Collection)
        current_idx = 0;
        last_arrival_time_us = 0; // 重置时间基准
        recording_enabled = true;
        
        printf("Sampling...");
        while (current_idx < SAMPLES_PER_TEST) {
            tight_loop_contents(); // 此处 Core 0 不进行任何 printf
        }
        recording_enabled = false;
        printf(" Done.\n");

        // 4. 数据倾倒 (Data Dump to Pi 4B)
        // 打印 CSV 头部，方便 Pi 4B 直接解析
        printf("---DATA_START:%dHZ---\n", freq);
        printf("index,jitter_us\n");
        
        int64_t sum_j = 0;
        int32_t max_j = -100000, min_j = 100000;

        for (uint32_t j = 0; j < SAMPLES_PER_TEST; j++) {
            int32_t val = jitter_array[j];
            printf("%u,%ld\n", j, val);
            
            sum_j += val;
            if (val > max_j) max_j = val;
            if (val < min_j) min_j = val;
        }

        // 5. 打印该频率的简报
        printf("---SUMMARY:%dHZ---\n", freq);
        printf("Avg:%2f,Max:%ld,Min:%ld\n", (float)sum_j / SAMPLES_PER_TEST, max_j, min_j);
        printf("---DATA_END---\n");

        sleep_ms(1000); // 频率切换间隔
    }

    printf("\n=== SWEEP TEST COMPLETE ===\n");
    while (true) { tight_loop_contents(); }
}