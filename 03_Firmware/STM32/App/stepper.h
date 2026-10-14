#ifndef STEPPER_H
#define STEPPER_H

#include "app_config.h"

/* 步进脉冲引擎：TIM6 节拍中断驱动 Bresenham 插补 + CoreXY 反解 + 梯形加减速。
 * 每个节拍：拉低 STEP 结束上一脉冲 → 插补决策 → 按 CoreXY 反解发 M1/M2 脉冲 → 更新节拍 ARR */

void stepper_init(void);
void stepper_start_line(int32_t dx_steps, int32_t dy_steps);   /* 启动一段直线 */
void stepper_stop(void);                                       /* 急停（清脉冲） */
uint8_t stepper_is_busy(void);
int32_t stepper_x_steps(void);                                 /* 当前平台 X 步数 */
int32_t stepper_y_steps(void);
void    stepper_x_reset(void);                                 /* 位置清零（回零后设原点） */
void    stepper_y_reset(void);

extern volatile uint8_t g_estop;          /* 限位/急停标志，limit 模块置位 */
extern volatile uint8_t g_move_finished;  /* 插补完成标志 */
extern volatile uint32_t g_step_counter;  /* 已发节拍数（调试） */

/* HAL 周期回调入口：由 HAL_TIM_PeriodElapsedCallback 分发（本文件已实现回调） */
void stepper_tick(void);

#endif
