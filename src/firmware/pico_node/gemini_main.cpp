#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_pico_hal.h"

// -------------------- Hardware Defines --------------------
#define PIN_RST 10
#define PIN_INT 11
#define I2C_INST i2c1
#define BNO_ADDR 0x4A
#define TARGET_PERIOD_US 20000 

// -------------------- Jitter Distribution Config --------------------
#define LOG_SIZE 600
volatile int32_t jitter_array[LOG_SIZE];
volatile uint32_t current_idx = 0;
volatile bool recording_enabled = false;
volatile uint32_t last_arrival_time_us = 0;

// -------------------- Sensor Callback --------------------
void sensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    sh2_SensorValue_t value;

    if (sh2_decodeSensorEvent(&value, event) == SH2_OK)
    {
        if (value.sensorId == SH2_GAME_ROTATION_VECTOR)
        {
            uint32_t now = to_us_since_boot(get_absolute_time());

            if (last_arrival_time_us != 0)
            {
                uint32_t dt = now - last_arrival_time_us;
                int32_t jitter = (int32_t)dt - TARGET_PERIOD_US;

                // Only record if the "Silent Phase" is active
                if (recording_enabled && current_idx < LOG_SIZE)
                {
                    jitter_array[current_idx++] = jitter;
                }
            }
            last_arrival_time_us = now;
        }
    }
}

// -------------------- Core1: Driver Loop --------------------
void core1_entry()
{
    // Use your original 100kHz for stability first
    i2c_init(i2c1, 100 * 1000);
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

    sh2_Hal_t* hal = get_sh2_pico_hal();
    if (sh2_open(hal, NULL, NULL) != SH2_OK) return;

    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start < 2000)
    {
        sh2_service();
        sleep_ms(2);
    }

    sh2_setSensorCallback(sensorHandler, NULL);
    sh2_SensorConfig_t config = {};
    config.reportInterval_us = TARGET_PERIOD_US;

    while (sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &config) != SH2_OK)
    {
        sleep_ms(200);
    }

    while (true)
    {
        sh2_service();
    }
}

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