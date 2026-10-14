/* ============================================================
 * CoreXY_Motion - STM32F407ZET6 (GEC-M4) 主程序
 * 平台：STM32CubeIDE + HAL + FreeRTOS（CMSIS_V1/V2 均可）
 * 功能：两轴 X-Y 视觉抓取系统运动控制
 *   1) TIM6 节拍中断 + Bresenham 插补 + CoreXY 反解 + 梯形加减速
 *   2) USART3 收 K210 坐标帧 ($cx,cy,flag\n)
 *   3) CAN1 板载 TJA1050 与上位机通信 (0x181/0x281/0x381)
 *   4) TIM1_CH1 PWM 驱动 MG90S 舵机夹爪
 *   5) PC0~PC3 限位急停（最高优先级中断）
 * 说明：本文件为完整用户代码；由 CubeMX 生成的 stm32f4xx_it.c /
 *       FreeRTOS 中间件需保留，HAL 回调在 App 层实现，无重复定义。
 * ============================================================ */
#include "main.h"
#include "app.h"
#include <stdio.h>

/* HAL_TIM_MspPostInit：TIM1 输出引脚(PA8)复用初始化，
 * CubeMX 生成物本应位于 stm32f4xx_hal_msp.c，此处前置声明+定义于本文件 */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
CAN_HandleTypeDef hcan1;

/* ---------------- 时钟：HSI 16MHz → SYSCLK 168MHz ----------------
 * 说明：原方案用 HSE 8MHz 外部晶振（PLLM=8/PLLN=336/PLLP=2），
 *       现场出现 HSE 起振失败 → Error_Handler（LED 快闪、程序不运行）。
 *       本版改回 HSI 内部 RC（16MHz：PLLM=8→2MHz，PLLN=168→336MHz，
 *       PLLP=2→168MHz），不依赖外部晶振，可靠性最高。 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState       = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM       = 8;
    osc.PLL.PLLN       = 168;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 7;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        Error_Handler();

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;      /* 42 MHz，定时器 84 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV2;      /* 84 MHz，定时器 168 MHz */
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK)
        Error_Handler();
}

/* ---------------- TIM1：舵机 PWM 50Hz（PA8 CH1） ---------------- */
static void MX_TIM1_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};

    __HAL_RCC_TIM1_CLK_ENABLE();

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 168 - 1;          /* 1 MHz 计数 */
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 20000;               /* 50 Hz */
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
        Error_Handler();

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 1000;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();

    HAL_TIM_MspPostInit(&htim1);
}

/* ---------------- TIM6：步进节拍定时器（插补基准） ---------------- */
static void MX_TIM6_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 84 - 1;           /* 1 MHz 计数 */
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 100;                 /* 默认 10 kHz，软件按加减速修改 */
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
        Error_Handler();

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 7, 0);   /* ≥ configMAX_SYSCALL_INTERRUPT_PRIORITY(7)，允许在 ISR 中调用 FreeRTOS API */
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/* ---------------- USART1：调试串口（CH340G） ---------------- */
static void MX_USART1_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
}

/* ---------------- USART3：K210 坐标帧 ---------------- */
static void MX_USART3_Init(void)
{
    __HAL_RCC_USART3_CLK_ENABLE();

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
        Error_Handler();

    HAL_NVIC_SetPriority(USART3_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

/* ---------------- CAN1：上位机（板载 TJA1050） ---------------- */
static void MX_CAN1_Init(void)
{
    CAN_FilterTypeDef filt = {0};

    __HAL_RCC_CAN1_CLK_ENABLE();

    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 4;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_16TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;   /* 42MHz/(4×(1+16+4))=500kbps，与上位机一致 */
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK)
        Error_Handler();

    filt.FilterIdHigh = 0x0000;
    filt.FilterIdLow = 0x0000;
    filt.FilterMaskIdHigh = 0x0000;
    filt.FilterMaskIdLow = 0x0000;
    filt.FilterFIFOAssignment = CAN_RX_FIFO0;
    filt.FilterBank = 0;
    filt.FilterMode = CAN_FILTERMODE_IDMASK;
    filt.FilterScale = CAN_FILTERSCALE_32BIT;
    filt.FilterActivation = ENABLE;
    if (HAL_CAN_ConfigFilter(&hcan1, &filt) != HAL_OK)
        Error_Handler();

    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

/* ---------------- GPIO：STEP/DIR/限位/LED/舵机引脚 ---------------- */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* X/Y STEP 输出，初始低 */
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Pin = X_STEP_PIN | Y_STEP_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(GPIOA, X_STEP_PIN | Y_STEP_PIN, GPIO_PIN_RESET);

    /* X/Y DIR 输出 */
    gpio.Pin = X_DIR_PIN | Y_DIR_PIN;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, X_DIR_PIN | Y_DIR_PIN, GPIO_PIN_RESET);

    /* 限位（双开关方案）：
     * X-：PA4(EXTI4) 上升沿触发 + 下拉，开关另一端接 VDD3V3，闭合拉高=高触发；
     * Y-：PD6(EXTI6) 下降沿触发 + 上拉（P8 摄像头座引脚 2 = DCMI_SCL 网络，
     *     紧邻 PA4 引脚 3；EXTI6 属 EXTI9_5 中断组），开关另一端接 GND，闭合拉低=低触发）。
     * PC1..PC3 预留未接线不初始化 */
    gpio.Pin = LIM_XN_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(EXTI4_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

    gpio.Pin = LIM_YN_PIN;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOD, &gpio);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    /* 心跳 LED1：PC13 低电平点亮 */
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/* 舵机引脚的 GPIO 复用（PA8 = TIM1_CH1） */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    if (htim->Instance == TIM1) {
        gpio.Pin = GPIO_PIN_8;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF1_TIM1;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

/* USART1(PA9/PA10 CH340G) 与 USART3(PB10/PB11 K210) 引脚复用 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio);
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
    GPIO_InitTypeDef gpio = {0};
    if (hcan->Instance == CAN1) {
        __HAL_RCC_CAN1_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        /* CAN_RX/CAN_TX 板载网络连 PD0/PD1（CAN1 复用，AF9）；
         * 注意：PA11/PA12 是板载 USB（OTG_FS_DM/DP），不是 CAN */
        gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOD, &gpio);
    }
}

/* USART1 为 CH340G 板载连接，仅需时钟（引脚在板上已固定） */

/* ---------------- printf 重定向到 USART1（CubeIDE 勾选 Use MicroLIB） ---------------- */
int fputc(int ch, FILE *f)
{
    (void)f;
    uint8_t b = (uint8_t)ch;
    HAL_UART_Transmit(&huart1, &b, 1, 10);
    return ch;
}

/* ---------------- 应用初始化：队列/信号量/任务 ---------------- */
void App_Init(void)
{
    BaseType_t ok;

    motion_init();
    stepper_init();
    limit_init();
    servo_init();
    comm_uart_init();
    comm_can_init();
    key_init();

    ok  = xTaskCreate(vMotionTask, "Motion",   1024, NULL, 5, NULL);
    ok &= xTaskCreate(vCommTask,  "Comm",     1024, NULL, 4, NULL);
    ok &= xTaskCreate(vKeyTask,   "Key",       256, NULL, 3, NULL);
    ok &= xTaskCreate(vDisplayTask, "Display", 1024, NULL, 2, NULL);
    ok &= xTaskCreate(vHeartbeatTask, "Heart", 512, NULL, 1, NULL);
    if (!ok)
        Error_Handler();

    /* 启动诊断：App_Init 全部完成 + 任务创建成功（CAN 已初始化，可发帧）
     * 上位机错误码若显示 0xB000 → 程序活着且 CAN 通；0xB001 → 调度器已启动 */
    extern void comm_can_send_err(uint16_t err);
    comm_can_send_err(0xB000);
}

/* ---------------- 主函数 ---------------- */
int main(void)
{
    /* ===== LED 阶段指示（PC13 低电平点亮；定位启动死点用） =====
     * 入口 亮    = 程序已进入 main（GPIOC 时钟手动使能）
     * 阶段1 灭    = SystemClock 完成（MX_GPIO_Init 后置灭）
     * 阶段2 亮    = TIM1+TIM6 初始化完成
     * 阶段3 灭    = USART1+USART3+CAN1 初始化完成
     * 阶段4 亮    = App_Init 完成（0xB000 诊断帧已发出）
     * 心跳慢闪    = 调度器已运行
     * 若死在 SystemClock（Error_Handler 触发），LED 会快速闪（100ms 周期）。
     * 定位完成后删除本段指示。 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);   /* 入口：亮 */

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();                                          /* 阶段1：灭 */

    MX_TIM1_Init();
    MX_TIM6_Init();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);   /* 阶段2：亮 */

    MX_USART1_Init();
    MX_USART3_Init();
    MX_CAN1_Init();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);     /* 阶段3：灭 */

    App_Init();
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);   /* 阶段4：亮 */

    vTaskStartScheduler();

    for (;;)
        ;
}

/* ---------------- 错误处理 ---------------- */
void Error_Handler(void)
{
    __disable_irq();
    for (;;) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

/* ---------------- FreeRTOS 钩子（config 中已启用） ---------------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    __disable_irq();
    for (;;) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(50);
    }
}

void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    for (;;) {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(200);
    }
}
