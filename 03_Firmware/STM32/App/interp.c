#include "interp.h"

void interp_start(Line_t *l, int32_t dx, int32_t dy)
{
    int32_t ax, ay;
    l->nx = dx;
    l->ny = dy;
    l->x_dir = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    l->y_dir = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    ax = (dx < 0) ? -dx : dx;
    ay = (dy < 0) ? -dy : dy;
    l->long_axis   = (ax > ay) ? ax : ay;
    l->short_total = (ax > ay) ? ay : ax;
    l->error = 0;
    l->remaining = l->long_axis;
    l->x_pos = 0;
    l->y_pos = 0;
}

int interp_tick(Line_t *l, int8_t *sx, int8_t *sy)
{
    *sx = 0;
    *sy = 0;
    if (l->remaining <= 0)
        return 1;

    if (l->nx != 0) {                 /* X 为主轴（含 nx==ny 情况） */
        *sx = (int8_t)l->x_dir;
        l->error += l->short_total;
        if (l->error * 2 >= l->long_axis) {
            *sy = (int8_t)l->y_dir;
            l->error -= l->long_axis;
        }
    } else {                          /* 纯 Y 运动 */
        *sy = (int8_t)l->y_dir;
    }

    l->x_pos += *sx;
    l->y_pos += *sy;
    l->remaining--;
    return (l->remaining <= 0) ? 1 : 0;
}

int interp_done(const Line_t *l)
{
    return (l->remaining <= 0);
}
