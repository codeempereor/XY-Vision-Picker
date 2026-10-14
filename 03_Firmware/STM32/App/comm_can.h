#ifndef COMM_CAN_H
#define COMM_CAN_H

#include "app_config.h"

/* 上位机通信：CAN1 (PA11/PA12) 500kbps，板载 TJA1050
 * 0x181 上报：pos_x(int16 LE, 0.001mm) pos_y(int16 LE) status(uint8)
 * 0x281 下发：cmd(0x01回零 0x02手动 0x03自动 0x04停止)；
 *            0x06 手动移动 + x(mm,int16 LE) + y(mm,int16 LE)
 * 0x381 错误：err(uint16 LE) */

void comm_can_init(void);
void comm_can_send_pos(void);
void comm_can_send_err(uint16_t err);

#endif
