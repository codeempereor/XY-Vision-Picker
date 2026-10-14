#include "stepper.h"
#include "corexy.h"
#include "interp.h"
#include "profile.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern TIM_HandleTypeDef htim6;   /* main.c 定义 */
extern SemaphoreHandle_t g_move_done_sem;   /* motion.c 创建：移动完成信号量 */

volatile uint8_t  g_estop = 0;
volatile uint8_t  g_move_finished = 0;
volatile uint32_t g_step_counter = 0;

static Line_t     s_line;
static Profile_t  s_prof;
static volatile uint8_t s_enabled = 0;

/* 位置跟踪：平台 X/Y 累计步数（带符号） */
static volatile int32_t s_x_steps = 0;
static volatile int32_t s_y_steps = 0;

void stepper_init(void)
{
    g_estop = 0;
    g_move_finished = 1;
    s_enabled = 0;
    s_x_steps = 0;
    s_y_steps = 0;
    HAL_GPIO_WritePin(X_STEP_PORT, X_STEP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Y_STEP_PORT, Y_STEP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X_DIR_PORT,  X_DIR_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Y_DIR_PORT,  Y_DIR_PIN,  GPIO_PIN_RESET);
}

void stepper_start_line(int32_t dx_steps, int32_t dy_steps)
{
    /* 清掉上一次可能残留的完成信号，避免本次瞬间"假完成" */
    xSemaphoreTake(g_move_done_sem, 0);
    interp_start(&s_line, dx_steps, dy_steps);
    profile_start(&s_prof, s_line.long_axis,
                  V_MAX_STEPS, ACCEL_STEPS2, V_START_STEPS, TIM6_TICK_CLK);
    g_estop = 0;
    g_move_finished = 0;
    g_step_counter = 0;
    s_enabled = 1;
    /* 初始节拍频率 = 起始速度 */
    {
        uint32_t arr = TIM6_TICK_CLK / V_START_STEPS - 1;
        if (arr < 2) arr = 2;
        __HAL_TIM_SET_AUTORELOAD(&htim6, arr);
        __HAL_TIM_SET_COUNTER(&htim6, 0);
    }
    HAL_TIM_Base_Start_IT(&htim6);
}

void stepper_stop(void)
{
    s_enabled = 0;
    HAL_TIM_Base_Stop_IT(&htim6);
    HAL_GPIO_WritePin(X_STEP_PORT, X_STEP_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Y_STEP_PORT, Y_STEP_PIN, GPIO_PIN_RESET);
    g_move_finished = 1;
    /* 唤醒等待移动完成的调用方（motion_move_mm），避免其空等超时 */
    xSemaphoreGive(g_move_done_sem);
}

uint8_t stepper_is_busy(void)
{
    return (uint8_t)s_enabled;
}

int32_t stepper_x_steps(void)
{
    return s_x_steps;
}

int32_t stepper_y_steps(void)
{
    return s_y_steps;
}

void stepper_x_reset(void)
{
    s_x_steps = 0;
}

void stepper_y_reset(void)
{
    s_y_steps = 0;
}

/* ================= 节拍处理（TIM6 更新中断 → HAL 回调） =================
 * CubeMX 生成的 stm32f4xx_it.c 中 TIM6_DAC_IRQHandler 会调用 HAL_TIM_IRQHandler，
 * 进而触发本回调；工程中无需自行定义 TIM6_IRQHandler。 */
void stepper_tick(void)
{
    int8_t sx, sy;
    int32_t dm1, dm2;

    /* 1. 结束上一轮脉冲（STEP 拉低） */
    X_STEP_PORT->BSRR = (uint32_t)X_STEP_PIN << 16;
    Y_STEP_PORT->BSRR = (uint32_t)Y_STEP_PIN << 16;

    /* 2. 急停或插补完成 → 停机 */
    if (g_estop || !s_enabled) {
        s_enabled = 0;
        HAL_TIM_Base_Stop_IT(&htim6);
        g_move_finished = 1;
        xSemaphoreGiveFromISR(g_move_done_sem, NULL);
        return;
    }

    /* 3. 插补一步 */
    if (interp_tick(&s_line, &sx, &sy)) {
        s_enabled = 0;
        HAL_TIM_Base_Stop_IT(&htim6);
        g_move_finished = 1;
        xSemaphoreGiveFromISR(g_move_done_sem, NULL);
        return;
    }

    /* 4. CoreXY 反解：m1 = x+y, m2 = x-y */
    corexy_inverse(sx, sy, &dm1, &dm2);
    if (dm1 != 0) {
        if (dm1 > 0)
            X_DIR_PORT->BSRR = X_DIR_PIN;                       /* DIR=1 */
        else
            X_DIR_PORT->BSRR = (uint32_t)X_DIR_PIN << 16;       /* DIR=0 */
        X_STEP_PORT->BSRR = X_STEP_PIN;                         /* STEP↑ */
    }
    if (dm2 != 0) {
        if (dm2 > 0)
            Y_DIR_PORT->BSRR = Y_DIR_PIN;
        else
            Y_DIR_PORT->BSRR = (uint32_t)Y_DIR_PIN << 16;
        Y_STEP_PORT->BSRR = Y_STEP_PIN;
    }

    /* 5. 位置跟踪（平台 X/Y 步数） */
    s_x_steps += sx;
    s_y_steps += sy;
    g_step_counter++;

    /* 6. 加减速：按当前速度更新节拍 ARR */
    {
        uint32_t arr = profile_arr(&s_prof);
        __HAL_TIM_SET_AUTORELOAD(&htim6, arr);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* HAL_TIM_IRQHandler 进入回调前已清除 UIF 标志，无需再次检查，直接执行节拍 */
    if (htim == &htim6) {
        stepper_tick();
    }
}
