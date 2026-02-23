#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

static bool as5600_read_u8(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *value) {
    int wret = i2c_write_blocking(i2c, addr, &reg, 1, true);
    if (wret != 1) {
        return false;
    }

    int rret = i2c_read_blocking(i2c, addr, value, 1, false);
    return rret == 1;
}

static bool as5600_read_u16(i2c_inst_t *i2c, uint8_t addr, uint8_t reg_msb, uint16_t *value) {
    uint8_t bytes[2] = {0};
    int wret = i2c_write_blocking(i2c, addr, &reg_msb, 1, true);
    if (wret != 1) {
        return false;
    }

    int rret = i2c_read_blocking(i2c, addr, bytes, 2, false);
    if (rret != 2) {
        return false;
    }

    *value = ((uint16_t)bytes[0] << 8) | bytes[1];
    return true;
}

static const char *as5600_magnet_judge(uint8_t status) {
    bool mh = (status & (1u << 3)) != 0;
    bool ml = (status & (1u << 4)) != 0;
    bool md = (status & (1u << 5)) != 0;

    if (md && !mh && !ml) {
        return "GOOD";
    }
    if (ml) {
        return "BAD_TOO_FAR";
    }
    if (mh) {
        return "BAD_TOO_CLOSE";
    }
    return "BAD_NO_MAGNET";
}

static bool read_as5600_magnet_diag(i2c_inst_t *i2c, uint8_t addr, uint8_t *status, uint8_t *agc, uint16_t *magnitude) {
    const uint8_t REG_STATUS = 0x0B;
    const uint8_t REG_AGC = 0x1A;
    const uint8_t REG_MAGNITUDE = 0x1B;

    bool ok_status = as5600_read_u8(i2c, addr, REG_STATUS, status);
    bool ok_agc = as5600_read_u8(i2c, addr, REG_AGC, agc);
    bool ok_mag = as5600_read_u16(i2c, addr, REG_MAGNITUDE, magnitude);
    return ok_status && ok_agc && ok_mag;
}

static bool read_as5600_raw_angle(i2c_inst_t *i2c, uint8_t addr, uint16_t *raw_angle) {
    const uint8_t REG_RAW_ANGLE = 0x0C;
    uint16_t value = 0;
    bool ok = as5600_read_u16(i2c, addr, REG_RAW_ANGLE, &value);
    if (!ok) {
        return false;
    }

    *raw_angle = value & 0x0FFF;
    return true;
}

static bool i2c_probe(i2c_inst_t *i2c, uint8_t addr) {
    uint8_t data = 0;
    int ret = i2c_read_blocking(i2c, addr, &data, 1, false);
    return ret == 1;
}

static bool tca_select_channel(i2c_inst_t *i2c, uint8_t tca_addr, uint8_t ch) {
    uint8_t reg = (uint8_t)(1u << ch);
    int ret = i2c_write_blocking(i2c, tca_addr, &reg, 1, false);
    return ret == 1;
}

int main() {
    stdio_init_all();

    i2c_init(i2c0, 100 * 1000);
    const uint SDA_PIN = 4;
    const uint SCL_PIN = 5;
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    const uint8_t TCA_ADDR = 0x70;
    const uint8_t AS5600_ADDR = 0x36;
    const uint LED_PIN = 25;
    int active_ch = -1;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    printf("\n--- TCA9548A + AS5600 Realtime Monitor (i2c0 GP4/GP5) ---\n");

    while (true) {
        bool tca_ok = i2c_probe(i2c0, TCA_ADDR);
        if (!tca_ok) {
            printf("[WARN] ⚠️  TCA9548A (0x70) not found\n");
            gpio_put(LED_PIN, 0);
            active_ch = -1;
            sleep_ms(300);
            continue;
        }

        gpio_put(LED_PIN, 1);

        bool as5600_ok = false;
        if (active_ch >= 0 && tca_select_channel(i2c0, TCA_ADDR, (uint8_t)active_ch)) {
            sleep_ms(1);
            as5600_ok = i2c_probe(i2c0, AS5600_ADDR);
        }

        if (!as5600_ok) {
            active_ch = -1;
            for (uint8_t ch = 0; ch < 8; ++ch) {
                bool selected = tca_select_channel(i2c0, TCA_ADDR, ch);
                if (!selected) {
                    continue;
                }

                sleep_ms(1);
                if (i2c_probe(i2c0, AS5600_ADDR)) {
                    active_ch = (int)ch;
                    printf("[INFO] AS5600 detected on CH%d\n", active_ch);
                    as5600_ok = true;
                    break;
                }
            }
        }

        if (!as5600_ok || active_ch < 0) {
            printf("[INFO] no AS5600 on CH0..CH7\n");
            sleep_ms(300);
            continue;
        }

        if (!tca_select_channel(i2c0, TCA_ADDR, (uint8_t)active_ch)) {
            printf("[WARN] CH%d select failed\n", active_ch);
            sleep_ms(300);
            continue;
        }

        uint8_t status = 0;
        uint8_t agc = 0;
        uint16_t magnitude = 0;
         uint16_t raw_angle = 0;
        bool diag_ok = read_as5600_magnet_diag(i2c0, AS5600_ADDR, &status, &agc, &magnitude);
         bool angle_ok = read_as5600_raw_angle(i2c0, AS5600_ADDR, &raw_angle);
         if (!diag_ok || !angle_ok) {
            printf("[WARN] CH%d AS5600 register read failed\n", active_ch);
            sleep_ms(120);
            continue;
        }

         const char *judge = as5600_magnet_judge(status);
         float angle_deg = ((float)raw_angle * 360.0f) / 4096.0f;
         printf("[CH%d] STATUS=0x%02X AGC=%u MAG=%u RAW=%u DEG=%.2f => %s\n",
             active_ch, status, agc, magnitude, raw_angle, angle_deg, judge);
         printf("AS5600_RT ch=%d raw=%u deg=%.2f status=0x%02X agc=%u mag=%u judge=%s\n",
             active_ch, raw_angle, angle_deg, status, agc, magnitude, judge);
        sleep_ms(100);
    }
}