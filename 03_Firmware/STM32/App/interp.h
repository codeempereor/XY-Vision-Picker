#ifndef INTERP_H
#define INTERP_H

#include <stdint.h>

/* Bresenham 直线插补：在 X-Y 平面从 (0,0) 走到 (nx,ny)，
 * 由节拍中断逐 tick 驱动，保证两轴同时到位、轨迹为直线。 */
typedef struct {
    int32_t nx, ny;          /* 目标步数（带符号） */
    int32_t long_axis;       /* 长轴总步数（节拍数） */
    int32_t short_total;     /* 短轴总步数 */
    int32_t x_dir, y_dir;    /* 每步方向 */
    int32_t error;           /* 误差累加器 */
    int32_t remaining;       /* 长轴剩余步数 */
    int32_t x_pos, y_pos;    /* 已走步数 */
} Line_t;

void interp_start(Line_t *l, int32_t dx, int32_t dy);
/* 每节拍调用一次；输出本 tick 的 X/Y 步进(-1/0/1)；返回 1 表示插补完成 */
int  interp_tick(Line_t *l, int8_t *sx, int8_t *sy);
int  interp_done(const Line_t *l);

#endif
