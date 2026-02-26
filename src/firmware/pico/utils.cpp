#include "utils.h"

void initI2C(i2c_inst_t *i2c, uint rst_pin, uint int_pin, bool pullup) {
    // Initialize I2C at 400kHz
    i2c_init(i2c, 400 * 1000);
    
    // Set GPIO function to I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    if (pullup) {
        gpio_pull_up(I2C_SDA_PIN);
        gpio_pull_up(I2C_SCL_PIN);
    }

    // Initialize Reset Pin
    gpio_init(rst_pin);
    gpio_set_dir(rst_pin, GPIO_OUT);
    gpio_put(rst_pin, 1); // Default high

    // Initialize Interrupt Pin
    gpio_init(int_pin);
    gpio_set_dir(int_pin, GPIO_IN);
    gpio_pull_up(int_pin);
}