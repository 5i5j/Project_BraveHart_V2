/**
 * @file uros_transport.c
 * @brief Custom micro-ROS transport layer for Raspberry Pi Pico with FreeRTOS.
 * @origin Based on micro_ros_raspberrypi_pico_sdk/pico_uart_transport.c
 * @modifications 
 * - Integrated FreeRTOS vTaskDelay for non-blocking wait.
 * - Removed redundant stdio_init.
 */

#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"

/* FreeRTOS headers for task control */
#include "FreeRTOS.h"
#include "task.h"

#include <uxr/client/profile/transport/custom/custom_transport.h>

/**
 * @brief Micro-ROS internal delay function.
 * Changed to use FreeRTOS delay to avoid blocking the CPU.
 */
void usleep(uint64_t us) {
    if (us < 1000) {
        sleep_us(us); // Use hardware sleep for very short delays
    } else {
        vTaskDelay(pdMS_TO_TICKS(us / 1000));
    }
}

/**
 * @brief Clock implementation for ROS 2 time synchronization.
 */
int clock_gettime(clockid_t unused, struct timespec *tp) {
    uint64_t m = time_us_64();
    tp->tv_sec = m / 1000000;
    tp->tv_nsec = (m % 1000000) * 1000;
    return 0;
}

bool picoserial_open(struct uxrCustomTransport * transport) {
    /* stdio_init_all is handled in main.cpp for clarity */
    return true;
}

bool picoserial_close(struct uxrCustomTransport * transport) {
    return true;
}

/**
 * @brief Write bytes to the serial port.
 */
size_t picoserial_write(struct uxrCustomTransport * transport, uint8_t *buf, size_t len, uint8_t *errcode) {
    for (size_t i = 0; i < len; i++) {
        /* Standard Pico putchar to USB CDC */
        if (buf[i] != putchar(buf[i])) {
            *errcode = 1;
            return i;
        }
    }
    return len;
}

/**
 * @brief Read bytes from the serial port with a timeout.
 * Optimized for FreeRTOS: yields the CPU when no data is available.
 */
size_t picoserial_read(struct uxrCustomTransport * transport, uint8_t *buf, size_t len, int timeout, uint8_t *errcode) {
    uint64_t start_time_us = time_us_64();
    size_t bytes_read = 0;

    while (bytes_read < len) {
        /* Check if the total timeout (in milliseconds) has expired */
        if ((time_us_64() - start_time_us) > (uint64_t)timeout * 1000) {
            break; 
        }

        /* Try to get a character from USB CDC without waiting */
        int c = getchar_timeout_us(0); 
        
        if (c != PICO_ERROR_TIMEOUT) {
            buf[bytes_read++] = (uint8_t)c;
        } else {
            /**
             * CRITICAL CHANGE: Instead of sleep_us(10) which is a busy-wait,
             * vTaskDelay(1) tells the FreeRTOS scheduler to run other tasks 
             * on Core 0 while waiting for next USB data packet.
             */
            vTaskDelay(1); 
        }
    }

    *errcode = (bytes_read < len) ? 1 : 0;
    return bytes_read;
}