/* ============================================================
 * stm32f4xx_it.c - 中断服务程序（等效 CubeMX 生成文件）
 * 功能：各外设 IRQHandler → HAL_*_IRQHandler → App 层回调
 *       （HAL 回调在 App/stepper.c、limit.c、comm_uart.c、comm_can.c 实现）
 * 说明：本文件为手写工程补全，需随工程一起编译。
 * ============================================================ */
#include "main.h"
#include "stm32f4xx_hal.h"

/* 外设句柄（定义于 Core/Src/main.c） */
extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart3;
extern CAN_HandleTypeDef hcan1;

/* ---------------- TIM6 步进节拍中断 ---------------- */
void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}

/* ---------------- 限位 EXTI4（PA4 = X-，双开关方案） ---------------- */
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

/* ---------------- 限位 EXTI6（PD6 = Y-，P8 引脚2/DCMI_SCL；EXTI6 属 EXTI9_5 组） ---------------- */
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
}

/* ---------------- USART3 K210 接收 ---------------- */
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
}

/* ---------------- CAN1 FIFO0 接收 ---------------- */
void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}
