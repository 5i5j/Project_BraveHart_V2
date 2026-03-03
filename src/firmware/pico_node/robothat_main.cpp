#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_pico_hal.h"

// -------------------- Hardware Definitions (Robot HAT v4 Schema) --------------------
#define PIN_RST 3       // Connected to Pico GP3
#define PIN_INT 2       // Connected to Pico GP2
#define I2C_INST i2c0   // GP4/GP5 belongs to i2c0
#define PIN_SDA 4       // Robot HAT I2C SDA
#define PIN_SCL 5       // Robot HAT I2C SCL

// -------------------- Sweep Parameters --------------------
uint32_t sweep_freqs[] = {50, 100, 200, 400}; 
#define FREQ_COUNT (sizeof(sweep_freqs) / sizeof(sweep_freqs[0]))
#define SAMPLES_PER_TEST 500  

// -------------------- Statistics --------------------
volatile int32_t jitter_array[SAMPLES_PER_TEST];
volatile uint32_t current_idx = 0;
volatile bool recording_enabled = false;
volatile uint32_t current_target_period_us = 10000; 
volatile bool config_update_requested = false;     

// -------------------- Global Flags --------------------
volatile bool data_ready_flag = false;
volatile uint32_t last_arrival_time_us = 0;

// -------------------- ISR --------------------
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == PIN_INT) {
        data_ready_flag = true; 
    }
}

// -------------------- Sensor Callback --------------------
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
                int32_t jitter = (int32_t)dt - current_target_period_us;
                jitter_array[current_idx++] = jitter;
            }
            last_arrival_time_us = now;
        }
    }
}

// -------------------- Core1: Sensor Communication (I2C Master) --------------------
void core1_entry()
{
    // Initialize I2C0 for Robot HAT Bus
    i2c_init(I2C_INST, 400 * 1000); 
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    // Reset Sequence
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(200);
    gpio_put(PIN_RST, 1);
    sleep_ms(1000);

    // Interrupt Setup
    gpio_init(PIN_INT);
    gpio_set_dir(PIN_INT, GPIO_IN);
    gpio_pull_up(PIN_INT);
    gpio_set_irq_enabled_with_callback(PIN_INT, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    sh2_Hal_t* hal = get_sh2_pico_hal();
    if (sh2_open(hal, NULL, NULL) != SH2_OK) return;
    sh2_setSensorCallback(sensorHandler, NULL);

    while (true)
    {
        if (config_update_requested) {
            sh2_SensorConfig_t config = {};
            config.reportInterval_us = current_target_period_us;
            sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &config);
            config_update_requested = false; 
        }

        if (data_ready_flag) {
            data_ready_flag = false;
        }
        sh2_service();
        tight_loop_contents();
    }
}

// -------------------- Core0: Logic and Analysis --------------------
int main()
{
    stdio_init_all();
    sleep_ms(5000); 

    printf("\n=== ROBOT HAT v4: BNO085 JITTER SWEEP ===\n");
    
    multicore_launch_core1(core1_entry);

    for (uint i = 0; i < FREQ_COUNT; i++) {
        uint32_t freq = sweep_freqs[i];
        current_target_period_us = 1000000 / freq;

        printf("\n>>> TARGET FREQ: %d Hz\n", freq);
        
        config_update_requested = true;
        while(config_update_requested) { tight_loop_contents(); } 

        sleep_ms(3000); // Stabilization

        current_idx = 0;
        last_arrival_time_us = 0; 
        recording_enabled = true;
        
        while (current_idx < SAMPLES_PER_TEST) {
            tight_loop_contents(); 
        }
        recording_enabled = false;

        // Data Output
        printf("---DATA_START:%dHZ---\n", freq);
        int64_t sum_j = 0;
        int32_t max_j = -100000, min_j = 100000;

        for (uint32_t j = 0; j < SAMPLES_PER_TEST; j++) {
            int32_t val = jitter_array[j];
            sum_j += val;
            if (val > max_j) max_j = val;
            if (val < min_j) min_j = val;
        }

        printf("Avg Jitter: %2f us | Max: %ld us | Min: %ld us\n", (float)sum_j / SAMPLES_PER_TEST, max_j, min_j);
        printf("---DATA_END---\n");
    }

    while (true) { tight_loop_contents(); }
}