#ifndef MOTION_H
#define MOTION_H

#include "app_config.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* 运动控制任务（最高优先级 5）：
 * 接收坐标队列 → CoreXY 直线运动 → 到位后舵机抓取 → 投放点松爪。
 * 支持回零、手动/自动切换、急停。 */

void motion_init(void);            /* 创建队列/信号量（App_Init 调用） */
void vMotionTask(void *arg);
void vCommTask(void *arg);
void vKeyTask(void *arg);
void vDisplayTask(void *arg);
void vHeartbeatTask(void *arg);

void motion_request_home(void);
void motion_stop(void);
void motion_toggle_mode(void);
void motion_reset_pos(void);

extern QueueHandle_t     g_motion_queue;
extern QueueHandle_t     g_can_cmd_queue;
extern SemaphoreHandle_t g_move_done_sem;
extern SemaphoreHandle_t g_home_sem;
extern volatile uint8_t  g_sys_status;
extern volatile uint8_t  g_auto_mode;

#endif
