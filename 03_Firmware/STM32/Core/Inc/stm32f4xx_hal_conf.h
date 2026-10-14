/**
  ******************************************************************************
  * @file    stm32f4xx_hal_conf.h
  * @brief   HAL 配置（GEC-M4 / STM32F407ZET6，HSE=8MHz）
  *          与 CubeMX 生成配置等效，供手工工程编译使用
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F4xx_HAL_CONF_H
#define STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_CAN_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
/* 本工程未用：ADC/DAC/I2C/SPI/USB/SDIO/DMA2D/FMC/ETH/RNG/CRYP/HASH 等默认关闭 */

/* ########################## Oscillator Values adaptation #################### */
#define HSE_VALUE          8000000U   /*!< GEC-M4 板 8MHz 外部晶振 */
#define HSE_STARTUP_TIMEOUT 100U      /*!< Time out for HSE start up */
#define HSI_VALUE          16000000U  /*!< Internal 16 MHz oscillator */
#define LSI_VALUE          32000U     /*!< LSI 32 kHz oscillator */
#define LSE_VALUE          32768U     /*!< LSE 32.768 kHz oscillator */
#define LSE_STARTUP_TIMEOUT 5000U     /*!< Time out for LSE start up */
#define EXTERNAL_CLOCK_VALUE 12288000U /*!< External clock (not used) */

/* ########################## System Configuration ######################### */
#define VDD_VALUE              3300U
#define TICK_INT_PRIORITY      5U      /*!< 与 NVIC 中断优先级分组一致（分组4：0~15） */
#define USE_RTOS               0U
#define PREFETCH_ENABLE        1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE      1U

#define USE_HAL_ADC_REGISTER_CALLBACKS 0U
#define USE_HAL_CAN_REGISTER_CALLBACKS 0U
#define USE_HAL_DMA_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS 0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_RCC_REGISTER_CALLBACKS 0U

/* ########################## Assert Selection ############################## */
#define assert_param(expr) ((void)0U)

/* ########################## Peripheral's enable ########################## */
#define USE_HAL_GPIO_EXTI_ENABLED 1U

/* ########################## Includes #################################### */
/* 与 CubeMX 生成模板一致：各模块头构成 HAL 类型/宏定义的完整包含链
 * （hal_rcc.h → hal_def.h → stm32f4xx.h → CMSIS 类型 __IO/uint32_t 等） */
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_dma_ex.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_pwr_ex.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#ifdef __cplusplus
}
#endif

#endif /* STM32F4xx_HAL_CONF_H */
