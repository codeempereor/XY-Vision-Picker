#include "profile.h"
#include <math.h>

void profile_start(Profile_t *p, int32_t total, int32_t vmax,
                   int32_t accel, int32_t vstart, uint32_t tick_clk)
{
    int32_t n_acc_full;
    p->total   = total;
    p->vmax    = vmax;
    p->accel   = accel;
    p->vstart  = vstart;
    p->tick_clk = tick_clk;
    p->count   = 0;
    p->n_acc   = 0;
    p->n_dec   = 0;
    if (total <= 0)
        return;

    n_acc_full = (int32_t)((int64_t)vmax * vmax / accel / 2);   /* v^2/(2a) */
    if (n_acc_full <= 0)
        n_acc_full = 1;
    if (2 * n_acc_full >= total) {      /* 三角：无匀速段 */
        p->n_acc = total / 2;
        p->n_dec = total - p->n_acc;
    } else {
        p->n_acc = n_acc_full;
        p->n_dec = n_acc_full;
    }
}

uint32_t profile_arr(Profile_t *p)
{
    int32_t v;
    int32_t dv;

    if (p->total <= 0) {
        v = p->vstart;
    } else if (p->count < p->n_acc) {
        /* 线性加速：v = vstart + (vmax-vstart) * count / n_acc（纯整数，避免 ISR 浮点） */
        dv = (p->n_acc > 0) ? ((p->vmax - p->vstart) / p->n_acc) : 0;
        v = p->vstart + dv * p->count;
    } else if (p->count < p->total - p->n_dec) {
        v = p->vmax;
    } else {
        /* 线性减速：v = vmax - (vmax-vstart) * (n_dec-rem) / n_dec */
        int32_t rem = p->total - p->count;
        dv = (p->n_dec > 0) ? ((p->vmax - p->vstart) / p->n_dec) : 0;
        v = p->vmax - dv * (p->n_dec - rem);
    }

    if (v < p->vstart)
        v = p->vstart;
    if (v > p->vmax)
        v = p->vmax;

    p->count++;

    {
        uint32_t arr = (uint32_t)((uint64_t)p->tick_clk / (uint32_t)v) - 1;
        if (arr < 2)
            arr = 2;
        if (arr > 0xFFFFU)
            arr = 0xFFFFU;
        return arr;
    }
}
