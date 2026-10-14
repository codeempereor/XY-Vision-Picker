#ifndef LIMIT_H
#define LIMIT_H

#include "app_config.h"

/* 限位开关（双开关方案）：X- = PA4(EXTI4，高触发)，Y- = PE3(EXTI3，低触发)。
 * 触发后立即停 TIM6 脉冲并置位标志（优先级最高中断）。
 * 触发方向由引脚 EXTI 边沿配置过滤（X 上升沿 / Y 下降沿），回调按引脚置位。 */

void limit_init(void);
uint8_t limit_xn(void);
uint8_t limit_xp(void);
uint8_t limit_yn(void);
uint8_t limit_yp(void);

extern volatile uint8_t g_limit_xn;
extern volatile uint8_t g_limit_xp;
extern volatile uint8_t g_limit_yn;
extern volatile uint8_t g_limit_yp;

#endif
