#include "key.h"
#include "motion.h"
#include "stepper.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef KEY_DISABLE

static uint8_t s_last[4];
static uint8_t s_cur[4];

static uint8_t key_read(uint8_t idx)
{
    switch (idx) {
    case 0: return HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN) == GPIO_PIN_RESET;
    case 1: return HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET;
    case 2: return HAL_GPIO_ReadPin(KEY2_PORT, KEY2_PIN) == GPIO_PIN_RESET;
    default:return HAL_GPIO_ReadPin(KEY3_PORT, KEY3_PIN) == GPIO_PIN_RESET;
    }
}

void key_init(void)
{
    int i;
    for (i = 0; i < 4; i++) {
        s_last[i] = 0;
        s_cur[i] = key_read((uint8_t)i);
    }
}

void key_scan(void)
{
    uint8_t i, pressed;

    for (i = 0; i < 4; i++) {
        s_cur[i] = key_read(i);                 /* 读当前 */
        pressed = (s_last[i] == 1) && (s_cur[i] == 0);   /* 松手沿触发 */
        s_last[i] = s_cur[i];
        if (!pressed)
            continue;

        switch (i) {
        case 0: motion_toggle_mode(); break;
        case 1: motion_request_home(); break;
        case 2: motion_stop(); break;
        default: motion_reset_pos(); break;
        }
    }
}

#else

void key_init(void) {}
void key_scan(void) {}

#endif
