/*
 * FreeRTOS Kernel Configuration for BraveHart V2 (RP2040)
 * Optimized for: Dual-Core SMP, micro-ROS, and BNO085 IMU (50Hz)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ----------------------------------------------------------------      
 * Scheduler Settings
 * ---------------------------------------------------------------- */
#define configUSE_PREEMPTION                    1   // Enable preemption to ensure sensor tasks interrupt low-priority ones
#define configUSE_TIME_SLICING                  1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      125000000 // Default 125MHz
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // 1ms tick, suitable for 50Hz sampling
#define configMAX_PRIORITIES                    32
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 512 ) // Increase min stack size to prevent micro-ROS overflow
#define configMAX_TASK_NAME_LEN                 16
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1   // Required for micro-ROS
#define configUSE_RECURSIVE_MUTEXES             1   // Required for I2C locking
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TASK_NOTIFICATIONS            1

/* ----------------------------------------------------------------
 * SMP (Symmetric Multi-Processing) Settings
 * ---------------------------------------------------------------- */
#define configNUMBER_OF_CORES                   2   // Enable dual-core parallelism
#define configTICK_CORE                         0   // Core 0 handles system tick
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_CORE_AFFINITY                 1   // Enable core affinity to lock IMU to Core 1

/* ----------------------------------------------------------------
 * Memory Allocation
 * ---------------------------------------------------------------- */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1   // micro-ROS requires dynamic memory (Heap 4)
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 128 * 1024 ) ) // Reserve 128KB for micro-ROS

/* ----------------------------------------------------------------
 * Hook Functions & Debugging
 * ---------------------------------------------------------------- */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2   // Enable advanced stack overflow checking
#define configUSE_MALLOC_FAILED_HOOK            1
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* ----------------------------------------------------------------
 * Software Timer Settings
 * ---------------------------------------------------------------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024

/* ----------------------------------------------------------------
 * Interrupt Settings
 * ---------------------------------------------------------------- */
#define configKERNEL_INTERRUPT_PRIORITY          ( 3 << 6 )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( 1 << 6 )

/* ----------------------------------------------------------------
 * Included API Functions
 * ---------------------------------------------------------------- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTimerStart                     1
#define INCLUDE_xTimerStop                      1

/* ----------------------------------------------------------------
 * RP2040 Specific Interrupt Handlers
 * ---------------------------------------------------------------- */
// Note: For SMP, these mappings ensure FreeRTOS handles M0+ exceptions
#define vPortSVCHandler                         vPortSVCHandler
#define xPortPendSVHandler                      xPortPendSVHandler
#define xPortSysTickHandler                     xPortSysTickHandler

#endif /* FREERTOS_CONFIG_H */