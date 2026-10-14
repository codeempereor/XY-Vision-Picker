/*
 * FreeRTOSConfig.h — CoreXY_Motion (STM32F407ZET6 @168MHz)
 * 说明：手工工程用的 FreeRTOS 配置（等效 CubeMX 生成），
 *       内核 V11.x，portable: GCC/ARM_CM4F，heap_4。
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 提供 __NVIC_PRIO_BITS、SystemCoreClock、uint32_t 等定义（等效 CubeMX 生成） */
#include "stm32f4xx.h"

/*-----------------------------------------------------------
 * Application specific definitions
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                     1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
#define configTICK_RATE_HZ                       ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                     ( 7 )
#define configMINIMAL_STACK_SIZE                 ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                    ( ( size_t ) ( 24 * 1024 ) )
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              0
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_TASK_NOTIFICATIONS             1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TIMERS                         0
#define configCHECK_FOR_STACK_OVERFLOW           1
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_NEWLIB_REENTRANT               0

/* Co-routine definitions (unused) */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )

/* Software timer definitions (unused) */
#define configTIMER_TASK_PRIORITY                ( 2 )
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             ( 256 )

/* Set the following definitions to 1 to include the API function */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetIdleTaskHandle           0

/* Cortex-M protection: 0 = 关, 1 = 默认 */
#define configENABLE_MPU                         0
#define configENABLE_FPU                         1

/* Assert */
#define configASSERT( x )                        if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* 优先级分组相关：本工程 NVIC 分组4（0~15 抢占优先级） */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 7
#define configKERNEL_INTERRUPT_PRIORITY          ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS) )
#define configMAX_API_CALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS) )

/* Map the Newlib reentrancy struct (unused) */
#define configUSE_NEWLIB_REENTRANT               0

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
   standard names, so the startup vector table (DCD SVC_Handler / PendSV_Handler
   / SysTick_Handler) resolves to the FreeRTOS port implementations. */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
