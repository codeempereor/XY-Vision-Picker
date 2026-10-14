#include "motion.h"
#include "stepper.h"
#include "limit.h"
#include "servo.h"
#include "comm_can.h"
#include "comm_uart.h"
#include "key.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>

QueueHandle_t     g_motion_queue;
QueueHandle_t     g_can_cmd_queue;
SemaphoreHandle_t g_move_done_sem;
SemaphoreHandle_t g_home_sem;
volatile uint8_t  g_sys_status = ST_IDLE;
volatile uint8_t  g_auto_mode  = 0;

/* ---------- 运动执行原语 ---------- */

static int32_t mm_to_steps(float mm)
{
    return (int32_t)(mm * STEPS_PER_MM);
}

/* 移动到 (x_mm, y_mm)，阻塞等待到位或超时/急停；返回 0=成功 */
static int motion_move_mm(float x_mm, float y_mm)
{
    int32_t dx, dy;
    int32_t cur_x = stepper_x_steps();
    int32_t cur_y = stepper_y_steps();

    /* 软件行程保护 */
    if (x_mm < 0.0f || x_mm > X_TRAVEL_MM || y_mm < 0.0f || y_mm > Y_TRAVEL_MM) {
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0002);
        return -1;
    }

    dx = mm_to_steps(x_mm) - cur_x;
    dy = mm_to_steps(y_mm) - cur_y;

    stepper_start_line(dx, dy);
    if (xSemaphoreTake(g_move_done_sem, pdMS_TO_TICKS(MOVE_TIMEOUT_MS)) != pdPASS) {
        /* 超时诊断：高字节=counter低8位，bit2=TIM6 CEN，bit3=TIM6 UIE
         * 0x0003=完全没启动；0x0007=CEN已置位；0x000B=中断已使能；0x03NN=节拍在跑 */
        uint16_t diag = (uint16_t)(0x0003
                    | ((g_step_counter & 0xFF) << 8)
                    | ((TIM6->CR1 & TIM_CR1_CEN) ? 0x0004 : 0)
                    | ((TIM6->DIER & TIM_DIER_UIE) ? 0x0008 : 0));
        stepper_stop();
        g_sys_status = ST_ERROR;
        comm_can_send_err(diag);
        return -1;
    }
    if (g_estop) {
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0001);      /* 限位/急停 */
        return -1;
    }
    return 0;
}

static int motion_home_impl(void)
{
    g_sys_status = ST_HOME;
    g_estop = 0;
    limit_init();

    /* 双限位开关方案：X- = PA4(高触发)，Y- = PE3(低触发)。
     * 阶段1：X 负向找 X- 开关，EXTI4 上升沿触发即停（回调置 g_limit_xn 并给 g_home_sem）。
     * 阶段2：Y 负向找 Y- 开关，EXTI3 下降沿触发即停（回调置 g_limit_yn 并给 g_home_sem）。 */
    stepper_start_line(-X_HOME_STEPS, 0);
    if (xSemaphoreTake(g_home_sem, pdMS_TO_TICKS(MOVE_TIMEOUT_MS)) != pdPASS) {
        stepper_stop();
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0001);      /* 回零超时：X 未触发限位 */
        return -1;
    }
    stepper_stop();
    if (!g_limit_xn) {                  /* 理论不发生：仅 EXTI4 会置位 */
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0001);
        return -1;
    }
    stepper_x_reset();                  /* X 原点 = X- 开关触发位置 */

    stepper_start_line(0, -Y_HOME_STEPS);
    if (xSemaphoreTake(g_home_sem, pdMS_TO_TICKS(MOVE_TIMEOUT_MS)) != pdPASS) {
        stepper_stop();
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0003);      /* 回零超时：Y 未触发限位 */
        return -1;
    }
    stepper_stop();
    if (!g_limit_yn) {                  /* 理论不发生：仅 EXTI3 会置位 */
        g_sys_status = ST_ERROR;
        comm_can_send_err(0x0003);
        return -1;
    }
    stepper_y_reset();                  /* Y 原点 = Y- 开关触发位置 */

    g_sys_status = ST_IDLE;
    return 0;
}

/* ---------- 抓取流程 ---------- */

static void auto_pick(MotionCmd_t *cmd)
{
    g_sys_status = ST_AUTO;

    /* 1. 移到目标点 */
    if (motion_move_mm((float)cmd->x_mm, (float)cmd->y_mm) != 0)
        return;

    /* 2. 抓取 */
    g_sys_status = ST_GRIP;
    servo_grip_close();
    vTaskDelay(pdMS_TO_TICKS(300));

    /* 3. 移投放点 */
    if (motion_move_mm(DUMP_X_MM, DUMP_Y_MM) != 0)
        return;

    /* 4. 松爪 */
    servo_grip_open();
    vTaskDelay(pdMS_TO_TICKS(200));

    g_sys_status = ST_DONE;
}

/* ---------- 对外接口（按键/CAN 调用） ---------- */

void motion_toggle_mode(void)
{
    g_auto_mode ^= 1;
}

void motion_request_home(void)
{
    uint8_t cmd = CAN_CMD_HOME;
    xQueueSend(g_can_cmd_queue, &cmd, 0);
}

void motion_stop(void)
{
    stepper_stop();
    g_sys_status = ST_IDLE;
}

void motion_reset_pos(void)
{
    stepper_x_reset();
    stepper_y_reset();
}

/* ---------- 任务 ---------- */

void motion_init(void)
{
    g_motion_queue  = xQueueCreate(4, sizeof(MotionCmd_t));
    g_can_cmd_queue = xQueueCreate(4, sizeof(uint8_t));
    g_move_done_sem = xSemaphoreCreateBinary();
    g_home_sem      = xSemaphoreCreateBinary();
}

void vMotionTask(void *arg)
{
    MotionCmd_t cmd;
    uint8_t ccmd;

    (void)arg;
    for (;;) {
        /* 先处理 CAN 命令（高优先级），再处理坐标 */
        if (xQueueReceive(g_can_cmd_queue, &ccmd, 0) == pdPASS) {
            switch (ccmd) {
            case CAN_CMD_HOME:   motion_home_impl(); break;
            case CAN_CMD_STOP:   motion_stop(); break;
            case CAN_CMD_MANUAL: g_auto_mode = 0; break;
            case CAN_CMD_AUTO:   g_auto_mode = 1; break;
            default: break;
            }
            continue;
        }

        if (xQueueReceive(g_motion_queue, &cmd, portMAX_DELAY) == pdPASS) {
            /* 上位机手动移动（CAN MOVE）：任何模式下都执行 */
            if (cmd.src == SRC_CAN_MOVE) {
                g_sys_status = ST_MANUAL;
                motion_move_mm((float)cmd.x_mm, (float)cmd.y_mm);
                continue;
            }

            /* K210 视觉帧：仅自动模式生效 */
            if (!g_auto_mode)
                continue;

            /* 自动模式 + 目标存在(flag=1)：完整抓取流程 */
            if (cmd.flag)
                auto_pick(&cmd);
            /* 自动模式 + 无目标(flag=0)：保持待命，忽略，避免滑台回 (0,0) 原点 */
        }
    }
}

void vCommTask(void *arg)
{
    (void)arg;
    for (;;) {
        comm_uart_poll();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vKeyTask(void *arg)
{
    (void)arg;
    for (;;) {
        key_scan();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void vDisplayTask(void *arg)
{
    (void)arg;
    for (;;) {
        printf("[pos] x=%ld.%03ld y=%ld.%03ld status=%u mode=%u\r\n",
               (long)(stepper_x_steps() / (int32_t)STEPS_PER_MM),
               (long)((stepper_x_steps() * 10 % (int32_t)(STEPS_PER_MM * 10)) * 100 / 10),
               (long)(stepper_y_steps() / (int32_t)STEPS_PER_MM),
               (long)((stepper_y_steps() * 10 % (int32_t)(STEPS_PER_MM * 10)) * 100 / 10),
               g_sys_status, g_auto_mode);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void vHeartbeatTask(void *arg)
{
    static uint8_t first = 1;
    (void)arg;
    for (;;) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        if (first) {
            first = 0;
            comm_can_send_err(0xB001);   /* 启动诊断：调度器已运行、心跳任务首次执行 */
        }
        comm_can_send_pos();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
