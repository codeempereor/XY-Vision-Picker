#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>

/* 梯形加减速：按长轴位置计算当前节拍频率，输出定时器 ARR。
 * 分段：加速(0..n_acc) / 匀速(n_acc..total-n_dec) / 减速。 */
typedef struct {
    int32_t total;      /* 长轴总步数 */
    int32_t vmax;       /* 最大步/s */
    int32_t accel;      /* 步/s^2 */
    int32_t vstart;     /* 起始步/s */
    int32_t n_acc;      /* 加速段步数 */
    int32_t n_dec;      /* 减速段步数 */
    int32_t count;      /* 已走长轴步数 */
    uint32_t tick_clk;  /* 定时器计数时钟 Hz */
} Profile_t;

void     profile_start(Profile_t *p, int32_t total, int32_t vmax,
                       int32_t accel, int32_t vstart, uint32_t tick_clk);
/* 返回当前速度对应的定时器 ARR（内部 count++，每节拍调用一次） */
uint32_t profile_arr(Profile_t *p);

#endif
