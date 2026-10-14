#include "servo.h"

extern TIM_HandleTypeDef htim1;

void servo_init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    servo_set_angle(0.0f);
}

void servo_set_angle(float deg)
{
    uint32_t ccr;
    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 180.0f) deg = 180.0f;
    ccr = SERVO_CCR_MIN + (uint32_t)((float)(SERVO_CCR_MAX - SERVO_CCR_MIN) * deg / 180.0f);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
}

void servo_grip_open(void)
{
    servo_set_angle(0.0f);
}

void servo_grip_close(void)
{
    servo_set_angle(90.0f);
}
