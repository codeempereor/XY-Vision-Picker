#ifndef KEY_H
#define KEY_H

#include "app_config.h"

/* 板载按键 KEY0..KEY3（上拉，按下低电平），20ms 轮询消抖。
 * KEY0：手动/自动模式切换
 * KEY1：回零
 * KEY2：停止
 * KEY3：坐标清零
 * 若引脚与主功能冲突，可在 app_config.h 修改或定义 KEY_DISABLE 屏蔽 */

void key_init(void);
void key_scan(void);   /* vKeyTask 中 20ms 调用 */

#endif
