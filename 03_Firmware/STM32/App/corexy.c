#include "corexy.h"

void corexy_inverse(int32_t dx, int32_t dy, int32_t *dm1, int32_t *dm2)
{
    *dm1 = dx + dy;
    *dm2 = dx - dy;
}

void corexy_forward(int32_t dm1, int32_t dm2, int32_t *dx, int32_t *dy)
{
    *dx = (dm1 + dm2) / 2;
    *dy = (dm1 - dm2) / 2;
}
