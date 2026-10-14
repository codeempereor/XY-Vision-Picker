#ifndef APP_H
#define APP_H

#include "app_config.h"
#include "corexy.h"
#include "interp.h"
#include "profile.h"
#include "stepper.h"
#include "limit.h"
#include "servo.h"
#include "comm_uart.h"
#include "comm_can.h"
#include "key.h"
#include "motion.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* 应用层总初始化：创建队列/信号量/任务，启动外设。
 * 在 main() 中所有 MX_*_Init() 之后、调度器启动之前调用。 */
void App_Init(void);

#endif
