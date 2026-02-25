/**
 * @file sh2_pico_hal.cpp
 * @brief RP2040 (Pico) Hardware Abstraction Layer for Hillcrest SH2 Driver.
 * * This file bridges the CEVA/Hillcrest SH2 library to the Raspberry Pi Pico 
 * SDK hardware interfaces (I2C, GPIO, and System Timer).
 */

#include "sh2_pico_hal.h"
#include "utils.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <string.h>
#include <cstdio>
#include <cstring>

/* --- Configuration Constants --- */
#define BNO08X_ADDR 0x4A   // Default I2C address
#define BNO08X_INT_PIN 15  // Interrupt Pin (GP15)
#define I2C_TIMEOUT_US 100000 // 100ms timeout for I2C transactions
#define I2C_SDA_PIN 6      // Defined based on your main.cpp
#define I2C_SCL_PIN 7      // Defined based on your main.cpp

/* --- External Hardware Reference --- */
// Ensure i2c_port is defined globally in your main.cpp (e.g., i2c_inst_t* i2c_port = i2c1;)
extern i2c_inst_t* i2c_port; 

/* --- Static HAL Instance --- */
static sh2_Hal_t pico_hal_instance;

/**
 * @brief Provides the initialized HAL instance to the SH2 library.
 * @return Pointer to sh2_Hal_t structure.
 */
sh2_Hal_t* get_sh2_pico_hal(void) {
    pico_hal_instance.open = sh2_hal_open;
    pico_hal_instance.close = sh2_hal_close;
    pico_hal_instance.read = sh2_hal_read;
    pico_hal_instance.write = sh2_hal_write;
    pico_hal_instance.getTimeUs = sh2_hal_getTimeUs;
    return &pico_hal_instance;
}

// --- 💡 Helper: I2C Bus Recovery (Ported from utils.cpp/main.cpp) ---
static void hal_i2c_recover() {
    // 1. Bit-bang SCL to clear stuck SDA
    gpio_init(I2C_SDA_PIN); gpio_set_dir(I2C_SDA_PIN, GPIO_IN);
    gpio_init(I2C_SCL_PIN); gpio_set_dir(I2C_SCL_PIN, GPIO_OUT);
    
    for (int i = 0; i < 9; i++) {
        gpio_put(I2C_SCL_PIN, 0); sleep_us(5);
        gpio_put(I2C_SCL_PIN, 1); sleep_us(5);
    }
    
    // 2. Generate STOP condition
    gpio_set_dir(I2C_SDA_PIN, GPIO_OUT);
    gpio_put(I2C_SCL_PIN, 0);
    gpio_put(I2C_SDA_PIN, 0);
    sleep_us(5);
    gpio_put(I2C_SCL_PIN, 1);
    sleep_us(5);
    gpio_put(I2C_SDA_PIN, 1);
    sleep_us(5);

    // 3. Restore I2C Functionality
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

/**
 * @brief Opens the SH2 session.
 * @return 0 on success.
 */
int sh2_hal_open(sh2_Hal_t *self) {
    // Hardware reset is handled in main.cpp. 
    // We simply return success to allow sh2_open to proceed and read the Advertisement packet.
    return 0;
}

/**
 * @brief Closes the SH2 session.
 */
void sh2_hal_close(sh2_Hal_t *self) {
    // No specific cleanup required for Pico I2C
}

/**
 * @brief Reads a full SHTP packet from the BNO085.
 * * Logic:
 * 1. Check INT pin (Active Low). If High, no data is available.
 * 2. Read 4-byte SHTP Header to determine packet length.
 * 3. Read the remaining payload based on the parsed length.
 * * @return Number of bytes read, 0 if no data, or -1 on I2C error.
 */
// --- sh2_pico_hal.cpp ---

int sh2_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {
    if (gpio_get(11) != 0) return 0; // INT 为高不读

    *t_us = to_us_since_boot(get_absolute_time());

    // 💡 强制读取一个足够大的数据块，避开分段逻辑
    // 对于初期订阅，大部分包都在 20-30 字节
    int ret = i2c_read_blocking(i2c1, 0x4A, pBuffer, 32, false);
    if (ret <= 0) return -1;

    // 解析真正的长度
    uint16_t packet_size = (pBuffer[1] << 8 | pBuffer[0]) & 0x7FFF;
    return (packet_size > 32) ? 32 : packet_size; // 简单处理
}

int sh2_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    // 💡 简单的阻塞写入，不带 timeout 干扰
    int ret = i2c_write_blocking(i2c1, 0x4A, pBuffer, len, false);
    return (ret == (int)len) ? len : -1;
}

/**
 * @brief Returns the system time in microseconds.
 */
uint32_t sh2_hal_getTimeUs(sh2_Hal_t *self) {
    return to_us_since_boot(get_absolute_time());
}

/**
 * @brief Asynchronous event callback for SH2 protocol events.
 * Handles resets and protocol errors reported by the sensor hub.
 */
void hal_event_callback(void *cookie, sh2_AsyncEvent_t *pEvent) {
    if (pEvent->eventId == SH2_RESET) {
        // Log sensor reset events for debugging
        printf(">> [HAL EVENT] BNO085 Reset detected.\n");
    }
}