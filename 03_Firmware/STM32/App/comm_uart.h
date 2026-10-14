#ifndef COMM_UART_H
#define COMM_UART_H

#include "app_config.h"

/* K210 通信：USART3 (PB10/PB11) 115200-8-N-1
 * 收帧格式：$cx,cy,flag\n （像素坐标，flag=1 识别到目标）
 * 解析后做仿射变换（像素→mm），投递到运动队列。 */

void comm_uart_init(void);
void comm_uart_poll(void);   /* vCommTask 中调用：行就绪时解析 */

#endif
