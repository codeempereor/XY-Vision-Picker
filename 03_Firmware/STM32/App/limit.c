#include "limit.h"
#include "stepper.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern TIM_HandleTypeDef htim6;
extern SemaphoreHandle_t g_home_sem;   /* 回零完成信号量（motion.c 创建） */

/* 限位（双开关方案）：X- = PA4(EXTI4，高触发)，Y- = PD6(EXTI6，低触发，P8 引脚2/DCMI_SCL)。
 * 触发方向已由引脚 EXTI 边沿配置过滤（X 上升沿 / Y 下降沿），
 * 回调中按引脚直接置位对应标志，无需再判断电平。 */

volatile uint8_t g_limit_xn = 0;
volatile uint8_t g_limit_xp = 0;
volatile uint8_t g_limit_yn = 0;
volatile uint8_t g_limit_yp = 0;

void limit_init(void)
{
    g_limit_xn = g_limit_xp = g_limit_yn = g_limit_yp = 0;
}

uint8_t limit_xn(void) { return g_limit_xn; }
uint8_t limit_xp(void) { return g_limit_xp; }
uint8_t limit_yn(void) { return g_limit_yn; }
uint8_t limit_yp(void) { return g_limit_yp; }

/* ================= EXTI 回调（优先级最高） ================= */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t hpw = pdFALSE;

    if (GPIO_Pin == LIM_XN_PIN) g_limit_xn = 1;
    if (GPIO_Pin == LIM_XP_PIN) g_limit_xp = 1;
    if (GPIO_Pin == LIM_YN_PIN) g_limit_yn = 1;
    if (GPIO_Pin == LIM_YP_PIN) g_limit_yp = 1;

    /* 立即停脉冲 */
    g_estop = 1;
    HAL_TIM_Base_Stop_IT(&htim6);
    X_STEP_PORT->BSRR = (uint32_t)X_STEP_PIN << 16;
    Y_STEP_PORT->BSRR = (uint32_t)Y_STEP_PIN << 16;

    if (g_home_sem != NULL)
        xSemaphoreGiveFromISR(g_home_sem, &hpw);
    portYIELD_FROM_ISR(hpw);
}
