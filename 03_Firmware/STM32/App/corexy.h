#ifndef CORE_XY_H
#define CORE_XY_H

#include <stdint.h>

/* CoreXY 运动学
 * 电机1(M1)、电机2(M2) 与平台 X/Y 位移关系：
 *   m1 = x + y      （X 正向：两电机同向；Y 正向：M1 正、M2 反）
 *   m2 = x - y
 */
void corexy_inverse(int32_t dx, int32_t dy, int32_t *dm1, int32_t *dm2);
void corexy_forward(int32_t dm1, int32_t dm2, int32_t *dx, int32_t *dy);

#endif
