#ifndef UTILS_H
#define UTILS_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "sh2_hal.h"

// Define default I2C pins for Pico (I2C1)
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

// Initialize I2C and GPIOs
void initI2C(i2c_inst_t *i2c, uint rst_pin, uint int_pin, bool pullup);

#endif