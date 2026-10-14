#ifndef SERVO_H
#define SERVO_H

#include "app_config.h"

/* 舵机 MG90S：TIM1_CH1 (PA8)，1MHz 计数，ARR=20000 → 50Hz。
 * 角度 0..180° 映射到 CCR 500..2500 (0.5ms..2.5ms)。 */

void servo_init(void);
void servo_set_angle(float deg);
void servo_grip_open(void);
void servo_grip_close(void);

#endif
