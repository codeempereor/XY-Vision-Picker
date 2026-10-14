#include "comm_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

extern UART_HandleTypeDef huart3;
extern QueueHandle_t     g_motion_queue;

#define UART_BUF_MAX   64

static volatile uint8_t  s_rx_byte;
static volatile uint8_t  s_line_ready = 0;
static volatile uint16_t s_len = 0;
static volatile uint8_t  s_buf[UART_BUF_MAX];

static Affine_t s_affine = AFFINE_DEFAULT;   /* 标定后替换/读 Flash */

void comm_uart_init(void)
{
    s_line_ready = 0;
    s_len = 0;
    HAL_UART_Receive_IT(&huart3, (uint8_t *)&s_rx_byte, 1);
}

static void affine_apply(int32_t px, int32_t py, int32_t *mx, int32_t *my)
{
    *mx = (int32_t)(s_affine.a * (float)px + s_affine.b * (float)py + s_affine.c);
    *my = (int32_t)(s_affine.d * (float)px + s_affine.e * (float)py + s_affine.f);
}

void comm_uart_poll(void)
{
    MotionCmd_t cmd;
    int32_t px, py, flag;
    int32_t mx, my;

    if (!s_line_ready)
        return;
    s_line_ready = 0;
    s_len = 0;

    if (sscanf((const char *)s_buf, "$%d,%d,%d", &px, &py, &flag) == 3) {
        affine_apply(px, py, &mx, &my);
        cmd.x_mm = mx;
        cmd.y_mm = my;
        cmd.flag = (uint8_t)flag;
        cmd.src  = SRC_K210;

        /* 只保留最新一帧目标坐标：入队前清空旧帧，
         * 避免运动任务在抓取流程执行期间消费到过期坐标 */
        MotionCmd_t drop;
        while (xQueueReceive(g_motion_queue, &drop, 0) == pdPASS) {
        }
        xQueueSend(g_motion_queue, &cmd, 0);
    }
}

/* ================= USART3 接收中断 ================= */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != &huart3)
        return;

    if (s_len < UART_BUF_MAX - 1)
        s_buf[s_len++] = s_rx_byte;
    if (s_rx_byte == '\n' || s_len >= UART_BUF_MAX - 1)
        s_line_ready = 1;

    HAL_UART_Receive_IT(&huart3, (uint8_t *)&s_rx_byte, 1);
}
