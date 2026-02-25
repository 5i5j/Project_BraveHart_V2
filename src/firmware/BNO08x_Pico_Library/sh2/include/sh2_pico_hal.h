#ifndef SH2_PICO_HAL_H
#define SH2_PICO_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "sh2.h"
#include "sh2_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取专为 Pico 适配的 SH2 HAL 实例
 * @return sh2_Hal_t* 指向初始化好的 HAL 结构体
 */
sh2_Hal_t* get_sh2_pico_hal(void);

// 声明你在 .cpp 里实现的函数，供 sh2_open 使用
int sh2_hal_open(sh2_Hal_t *self);
void sh2_hal_close(sh2_Hal_t *self);
int sh2_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
int sh2_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
uint32_t sh2_hal_getTimeUs(sh2_Hal_t *self);

// 协议要求的异步事件回调声明
void hal_event_callback(void *cookie, sh2_AsyncEvent_t *pEvent);

#ifdef __cplusplus
}
#endif

#endif // SH2_PICO_HAL_H