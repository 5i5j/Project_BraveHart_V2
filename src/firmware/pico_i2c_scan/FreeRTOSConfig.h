#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 包含必要的基础头文件，确保类型定义在宏判断前生效 */
#include <stdint.h>
#include <stddef.h>

/* ----------------------------------------------------------------
 * 1. 核心调度与时间设置 (Core Scheduling)
 * ---------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1   // 开启抢占
#define configUSE_TIME_SLICING                  1   // 开启时间片轮转
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      125000000 // Pico 默认频率
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // 1ms 心跳
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 512 ) // 增加栈空间给 micro-ROS
#define configMAX_TASK_NAME_LEN                 16
#define configIDLE_SHOULD_YIELD                 1

/* 针对新版 FreeRTOS 的位宽设置 (Fix for "Missing definition" error) */
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS

/* ----------------------------------------------------------------
 * 2. SMP 双核设置 (SMP Settings - RP2040 专属)
 * ---------------------------------------------------------------- */
#define configNUMBER_OF_CORES                   2   // 启用双核
#define configTICK_CORE                         0   // 核心 0 驱动时钟
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_CORE_AFFINITY                 1   // 允许核绑定（IMU 锁定 Core 1）
#define configUSE_PASSIVE_IDLE_HOOK             0   // Fix for SMP idle error

/* ----------------------------------------------------------------
 * 3. 内存管理 (Memory Management - micro-ROS 必需)
 * ---------------------------------------------------------------- */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 128 * 1024 ) ) // 128KB 堆
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configQUEUE_REGISTRY_SIZE               8

/* ----------------------------------------------------------------
 * 4. 事件组补丁 (Event Groups Patch - 解决 port.c 隐式声明报错)
 * ---------------------------------------------------------------- */
#define configUSE_EVENT_GROUPS                  1
#define INCLUDE_xEventGroupSetBitsFromISR       1
#define INCLUDE_xTimerPendFunctionCall          1

/* 强制在前向声明中加入函数原型，防止编译器在读取 port.c 时报错 */
#ifndef __ASSEMBLER__
#ifdef __cplusplus
extern "C" {
#endif
    struct EventGroupDef_t;
    typedef struct EventGroupDef_t * EventGroupHandle_t;
    long xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup, 
                                    const uint32_t uxBitsToSet, 
                                    long * pxHigherPriorityTaskWoken );
#ifdef __cplusplus
}
#endif
#endif

/* ----------------------------------------------------------------
 * 5. 钩子与调试 (Hooks & Debug)
 * ---------------------------------------------------------------- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2   // 开启高级栈检查
#define configUSE_MALLOC_FAILED_HOOK            1

/* 修正后的 Assert：在底层使用端口级关中断 */
#define configASSERT( x ) if( ( x ) == 0 ) { portDISABLE_INTERRUPTS(); for( ;; ); }

/* ----------------------------------------------------------------
 * 6. 软件定时器 (Software Timers)
 * ---------------------------------------------------------------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

/* ----------------------------------------------------------------
 * 7. 中断与 API 映射 (Interrupts & API Map)
 * ---------------------------------------------------------------- */
#define configKERNEL_INTERRUPT_PRIORITY          ( 3 << 6 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( 1 << 6 )

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/* Pico SDK 特定的中断处理函数名映射 */
#define vPortSVCHandler                         vPortSVCHandler
#define xPortPendSVHandler                      xPortPendSVHandler
#define xPortSysTickHandler                     xPortSysTickHandler

#endif /* FREERTOS_CONFIG_H */