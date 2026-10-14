#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "stm32f4xx_hal.h"

/* ================= 引脚定义（GEC-M4 板） ================= */
#define X_STEP_PORT     GPIOA
#define X_STEP_PIN      GPIO_PIN_0
#define Y_STEP_PORT     GPIOA
#define Y_STEP_PIN      GPIO_PIN_1
#define X_DIR_PORT      GPIOB
#define X_DIR_PIN       GPIO_PIN_0
#define Y_DIR_PORT      GPIOB
#define Y_DIR_PIN       GPIO_PIN_1
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_PIN_13            /* 低电平点亮 */

/* 限位（双开关方案）：
 * - X-：PA4（EXTI4，上升沿触发 + 下拉），开关另一端接 VDD3V3，闭合拉高=高电平触发。
 *   说明：原计划接 PC0，但 GEC-M4 板上 PC0 已被板载外设占用
 *   （MPU6050 中断 3D_INT + 板载 LED GBC_LED，见原理图 01-CPU 页），故用 PA4。
 * - Y-：PD6（EXTI6，下降沿触发 + 上拉，P8 摄像头座引脚 2 = DCMI_SCL 网络，
 *   紧邻 PA4 引脚 3），开关另一端接 GND，闭合拉低=低电平触发。
 * - X 轴回零：负向移动找 X- 开关，触发即停并建立 X 原点；
 * - Y 轴回零：负向移动找 Y- 开关（由原软回零升级为硬回零），触发即停并建立 Y 原点；
 * - PC1..PC3 为 4 路方案预留定义，当前未接线、不初始化。
 * 触发电平：X=高触发（LIM_X_ACTIVE_LEVEL=1），Y=低触发（LIM_Y_ACTIVE_LEVEL=0）。 */
#define LIM_XN_PORT     GPIOA
#define LIM_XN_PIN      GPIO_PIN_4
#define LIM_XP_PORT     GPIOC
#define LIM_XP_PIN      GPIO_PIN_1
#define LIM_YN_PORT     GPIOD
#define LIM_YN_PIN      GPIO_PIN_6
#define LIM_YP_PORT     GPIOC
#define LIM_YP_PIN      GPIO_PIN_3

/* 板载按键 KEY0..KEY3（上拉，按下低；引脚按 GEC-M4 丝印配置）。
 * 默认屏蔽（KEY 引脚未核实、KEY3 默认 PA0 与 X_STEP 冲突）；
 * 确认丝印后修改引脚并取消下行注释启用。 */
#define KEY_DISABLE 1
#define KEY0_PORT       GPIOE
#define KEY0_PIN        GPIO_PIN_4
#define KEY1_PORT       GPIOE
#define KEY1_PIN        GPIO_PIN_3
#define KEY2_PORT       GPIOE
#define KEY2_PIN        GPIO_PIN_2
#define KEY3_PORT       GPIOA
#define KEY3_PIN        GPIO_PIN_0

/* ================= 机械参数（实测校准） ================= */
#define STEPS_PER_MM    100.0f      /* 1/16 细分：3200 微步 / 32mm 每圈 */
#define X_TRAVEL_MM     200.0f
#define Y_TRAVEL_MM     150.0f
#define DUMP_X_MM       10.0f       /* 投放点坐标（mm） */
#define DUMP_Y_MM       140.0f

/* ================= 运动参数 ================= */
#define V_MAX_STEPS     10000       /* 100 mm/s × 100 步/mm */
#define ACCEL_STEPS2    50000       /* 500 mm/s^2 × 100 */
#define V_START_STEPS   500         /* 起始节拍频率（步/s） */
#define TIM6_TICK_CLK   1000000U    /* TIM6 计数时钟 1 MHz（PSC=84-1） */
#define MOVE_TIMEOUT_MS 8000        /* 单段运动超时（防卡死） */

/* ================= 单/双限位回零步数 ================= */
#define X_HOME_STEPS     ((int32_t)((X_TRAVEL_MM + 20.0f) * STEPS_PER_MM))  /* X 负向找开关：行程+余量 */
#define Y_HOME_STEPS     ((int32_t)((Y_TRAVEL_MM + 20.0f) * STEPS_PER_MM))  /* Y 负向找开关：行程+余量 */

/* ================= 舵机（TIM1_CH1 PA8，1MHz 计数，ARR=20000） ================= */
#define SERVO_CCR_MIN   500         /* 0°   → 0.5ms */
#define SERVO_CCR_MAX   2500        /* 180° → 2.5ms */

/* ================= 限位触发电平：X=高触发(PA4)，Y=低触发(PD6) ================= */
#define LIM_X_ACTIVE_LEVEL 1
#define LIM_Y_ACTIVE_LEVEL 0

/* ================= 仿射标定系数（像素→mm） =================
 * mm_x = a*px + b*py + c
 * mm_y = d*px + e*py + f
 * 默认单位变换，标定后写入（可存 Flash 0x0800FC00 保留扇区）
 */
typedef struct {
    float a, b, c, d, e, f;
} Affine_t;
#define AFFINE_DEFAULT  {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}

/* ================= 运动命令（队列元素） ================= */
#define SRC_K210       0       /* 命令来源：K210 视觉坐标帧 */
#define SRC_CAN_MOVE   1       /* 命令来源：上位机 CAN 手动移动 */
typedef struct {
    int32_t x_mm;
    int32_t y_mm;
    uint8_t flag;       /* 1=抓取流程，0=仅移动 */
    uint8_t src;        /* 命令来源（见 SRC_*），用于区分 K210 帧与手动移动 */
} MotionCmd_t;

/* ================= CAN 帧 ID 与命令码 ================= */
#define CAN_ID_POS      0x181
#define CAN_ID_CMD      0x281
#define CAN_ID_ERR      0x381
#define CAN_CMD_HOME    0x01
#define CAN_CMD_MANUAL  0x02
#define CAN_CMD_AUTO    0x03
#define CAN_CMD_STOP    0x04
#define CAN_CMD_MOVE    0x06    /* 手动移动：d[1..2]=X(mm,int16 LE) d[3..4]=Y(mm,int16 LE) */

/* ================= 状态码 ================= */
#define ST_IDLE     0
#define ST_HOME     1
#define ST_MANUAL   2
#define ST_AUTO     3
#define ST_GRIP     4
#define ST_DONE     5
#define ST_ERROR    6

#endif
