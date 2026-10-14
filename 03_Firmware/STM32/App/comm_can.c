#include "comm_can.h"
#include "stepper.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern CAN_HandleTypeDef hcan1;
extern QueueHandle_t    g_can_cmd_queue;
extern QueueHandle_t    g_motion_queue;
extern volatile uint8_t g_sys_status;

volatile uint32_t g_can_diag = 0;   /* 诊断字：1=进入 2=filter完 3=start完 4=notif完 5=完成；0x200=start失败 0x400=notif失败 0x800=强制退出初始化 */

void comm_can_init(void)
{
    CAN_FilterTypeDef filt;

    g_can_diag = 1;

    filt.FilterIdHigh         = 0x0000;
    filt.FilterIdLow          = 0x0000;
    filt.FilterMaskIdHigh     = 0x0000;          /* 屏蔽位全 0 → 接收全部 */
    filt.FilterMaskIdLow      = 0x0000;
    filt.FilterFIFOAssignment = CAN_RX_FIFO0;
    filt.FilterBank           = 0;
    filt.FilterMode           = CAN_FILTERMODE_IDMASK;
    filt.FilterScale          = CAN_FILTERSCALE_32BIT;
    filt.FilterActivation     = ENABLE;
    HAL_CAN_ConfigFilter(&hcan1, &filt);

    g_can_diag = 2;
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        g_can_diag |= 0x200;
        /* 绕过 HAL State 检查：强制请求离开初始化模式 */
        hcan1.State = HAL_CAN_STATE_READY;
        hcan1.ErrorCode = HAL_CAN_ERROR_NONE;
        if (HAL_CAN_Start(&hcan1) != HAL_OK) {
            g_can_diag |= 0x800;
            /* 直接硬件操作：清 INRQ 请求退出初始化，等待 INAK 清零 */
            CLEAR_BIT(hcan1.Instance->MCR, CAN_MCR_INRQ);
            {
                uint32_t t = 0;
                while ((hcan1.Instance->MSR & CAN_MSR_INAK) != 0U && t < 100000U)
                    t++;
                hcan1.State = HAL_CAN_STATE_LISTENING;
            }
        }
    }

    g_can_diag = 3;
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        g_can_diag |= 0x400;
        /* 直接使能接收 FIFO0 消息挂起中断 */
        SET_BIT(hcan1.Instance->IER, CAN_IER_FMPIE0);
        hcan1.State = HAL_CAN_STATE_LISTENING;
    }

    g_can_diag = 4;
}

void comm_can_send_pos(void)
{
    CAN_TxHeaderTypeDef h;
    uint8_t d[8];
    uint32_t mailbox;
    int16_t x, y;

    h.ExtId = 0;
    h.StdId = CAN_ID_POS;
    h.IDE   = CAN_ID_STD;
    h.RTR   = CAN_RTR_DATA;
    h.DLC   = 8;
    h.TransmitGlobalTime = DISABLE;

    /* 位置单位：mm×10（0.1mm 精度，int16 上限 3276.7mm > 200mm 行程） */
    x = (int16_t)(stepper_x_steps() * 100 / (int32_t)STEPS_PER_MM);
    y = (int16_t)(stepper_y_steps() * 100 / (int32_t)STEPS_PER_MM);
    d[0] = (uint8_t)(x & 0xFF);
    d[1] = (uint8_t)((x >> 8) & 0xFF);
    d[2] = (uint8_t)(y & 0xFF);
    d[3] = (uint8_t)((y >> 8) & 0xFF);
    d[4] = g_sys_status;
    d[5] = d[6] = d[7] = 0;

    HAL_CAN_AddTxMessage(&hcan1, &h, d, &mailbox);
}

void comm_can_send_err(uint16_t err)
{
    CAN_TxHeaderTypeDef h;
    uint8_t d[8];
    uint32_t mailbox;

    h.ExtId = 0;
    h.StdId = CAN_ID_ERR;
    h.IDE   = CAN_ID_STD;
    h.RTR   = CAN_RTR_DATA;
    h.DLC   = 8;
    h.TransmitGlobalTime = DISABLE;
    d[0] = (uint8_t)(err & 0xFF);
    d[1] = (uint8_t)((err >> 8) & 0xFF);
    d[2] = d[3] = d[4] = d[5] = d[6] = d[7] = 0;

    HAL_CAN_AddTxMessage(&hcan1, &h, d, &mailbox);
}

/* ================= CAN 接收中断 ================= */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef h;
    uint8_t d[8];
    uint32_t cmd;

    if (hcan != &hcan1)
        return;

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &h, d);
    if (h.IDE != CAN_ID_STD)
        return;

    if (h.StdId == CAN_ID_CMD) {
        cmd = d[0];
        if (cmd == CAN_CMD_MOVE) {
            MotionCmd_t mc;
            mc.x_mm = (int16_t)(d[1] | ((uint16_t)d[2] << 8));
            mc.y_mm = (int16_t)(d[3] | ((uint16_t)d[4] << 8));
            mc.flag = 0;
            mc.src  = SRC_CAN_MOVE;
            xQueueSendFromISR(g_motion_queue, &mc, NULL);
        } else {
            xQueueSendFromISR(g_can_cmd_queue, &cmd, NULL);
        }
    }
}
