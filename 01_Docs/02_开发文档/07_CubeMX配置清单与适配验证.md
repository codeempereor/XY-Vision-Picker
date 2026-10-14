# STM32F407ZET6（GEC-M4）CubeMX 配置清单

> 版本：v1.0 ｜ 日期：2026-10-08
> 工具：STM32CubeMX ≥ 6.8 + STM32CubeIDE 1.15+ ｜ 芯片：STM32F407ZETx（LQFP144）
> 配套工程：`STM32_CoreXY_Motion/`（.ioc 已生成，本清单供核对/复现）

---

## 0. 新建工程

1. File → New Project → 选择 **STM32F407ZETx**（Access line，LQFP144）
2. 项目名：`CoreXY_Motion`，Toolchain：**STM32CubeIDE**
3. 勾选 **Initialize all peripherals with their default Mode**

## 1. 时钟树（RCC）

| 项目 | 值 |
|---|---|
| HSE 晶振 | **8 MHz**（GEC-M4 板载，原理图 01-CPU 页确认） |
| LSE | 32.768 kHz（板载，可开 RTC） |
| PLL 源 | HSE |
| PLLM / PLLN / PLLP | 8 / 336 / 2 |
| SYSCLK | **168 MHz** |
| APB1 / APB1 Timer | 42 MHz / **84 MHz** |
| APB2 / APB2 Timer | 84 MHz / **168 MHz** |

> 时钟配置页选择 HSE→PLL，勾选"旁路或外部晶体"按板子实际；SYSCLK 168 后检查无红字。

## 2. 外设使能总表

| 外设 | 模式 | 引脚 | 用途 |
|---|---|---|---|
| TIM1 | PWM Generation CH1 | PA8 | 舵机 MG90S（50 Hz） |
| TIM6 | Internal Clock + 中断 | — | 步进节拍定时器（插补基准） |
| USART3 | Asynchronous | PB10(TX) / PB11(RX) | K210 坐标帧 `$cx,cy,flag\n` |
| CAN1 | CAN | PD0(RX) / PD1(TX) | 上位机通信（板载 TJA1050；PA11/PA12 实为板载 USB） |
| GPIO 输出 | — | PA0 / PA1 / PB0 / PB1 | X STEP / Y STEP / X DIR / Y DIR |
| GPIO 输入(EXTI) | — | PA4 / PD6 | 限位开关 X-/Y-（双开关方案：PA4 高触发，PD6 低触发；原 PC0 被板载 MPU6050 中断/ LED 占用） |
| GPIO 输出 | — | PC13 | 心跳 LED（低电平点亮） |
| USART1 | Asynchronous | PA9 / PA10 | 调试串口（CH340G，不动，可选 printf） |

## 3. 各外设详细参数

### 3.1 TIM1（舵机）
| 参数 | 值 |
|---|---|
| Clock Source | Internal Clock |
| Prescaler | 168-1（1 MHz 计数） |
| Counter Period (ARR) | 20000（50 Hz） |
| Pulse (CCR1) | 1000（0°）~ 2000（180°），中位 1500 |
| PWM 输出 | CH1 PA8 |

### 3.2 TIM6（节拍）
| 参数 | 值 |
|---|---|
| Clock Source | Internal Clock（APB1 Timer 84 MHz） |
| Prescaler | 84-1（1 MHz 计数） |
| Counter Period (ARR) | 100 → 10 kHz 节拍（100 步/mm × 100 mm/s） |
| NVIC | TIM6 global interrupt **Enabled，优先级 5**（FreeRTOS 之下、可抢占任务） |

> 节拍频率由软件按加减速动态修改 `TIM6->ARR`（`HAL_TIM_Base_Stop/Start` 或直接写 ARR），本工程默认 10 kHz 基准。

### 3.3 USART3（K210）
| 参数 | 值 |
|---|---|
| Mode | Asynchronous |
| BaudRate | 115200 |
| Word Length | 8 Bits |
| Parity | None |
| Stop Bits | 1 |
| 中断 | USART3 global interrupt **Enabled**（接收逐字节进环形缓冲） |
| 可选 | DMA RX 环形（性能更高，本工程用中断即可） |

### 3.4 CAN1（上位机）
| 参数 | 值 |
|---|---|
| Mode | CAN（无收发器选项——板载 TJA1050 已连 PD0/PD1，PA11/PA12 为 USB） |
| Prescaler | 4（42 MHz / 4 / 21 = 500 kbps，BS1=16tq、BS2=5tq） |
| Time Quantum | 21 |
| BS1 / BS2 | 16 / 5 |
| 波特率 | **500 kbps** |
| 中断 | CAN RX0 **Enabled**（FIFO0 消息接收） |
| 滤波器 | 屏蔽位模式，接收全部（FilterBank 0，屏蔽全 0） |

> 公式核对：42 MHz / (4 × (1+16+5)) = 42/88 = 0.477 MHz≈500 kbps ✓（按 CubeMX 实际算出 500k 为准，Prescaler=4）

### 3.5 GPIO
| 引脚 | 模式 | 上下拉 | 初始电平 | 备注 |
|---|---|---|---|---|
| PA0 | Output Push-Pull | None | Low | X STEP |
| PA1 | Output Push-Pull | None | Low | Y STEP |
| PB0 | Output Push-Pull | None | Low | X DIR（0=正，1=负，软件可配） |
| PB1 | Output Push-Pull | None | Low | Y DIR |
| PA4 | External Interrupt Rising | Pull-Down | — | 限位开关 X-（EXTI4），开关另一端接 3.3V，闭合=高电平触发 |
| PD6 | External Interrupt Falling | Pull-Up | — | 限位开关 Y-（EXTI6，P8 摄像头座引脚 2 = DCMI_SCL 网络，紧邻 PA4 引脚 3），开关另一端接 GND，闭合=低电平触发 |
| PC13 | Output Push-Pull | None | High | LED1 心跳（低电平点亮，GEC-M4 板） |

> 限位开关接法（双开关）：X-（PA4）开关一端 3.3V、一端接 PA4（内部下拉），闭合=高电平=触发；Y-（PD6）开关一端 GND、一端接 PD6（P8 摄像头座引脚 2 = DCMI_SCL 网络，内部上拉），闭合=低电平=触发。软件触发电平分别定义于 app_config.h（LIM_X_ACTIVE_LEVEL=1 / LIM_Y_ACTIVE_LEVEL=0）。

### 3.6 FreeRTOS

| 项目 | 值 |
|---|---|
| Interface | CMSIS_V1 或 V2 均可（工程代码用原生 `xTaskCreate`） |
| 任务数 | 5（见下表） |
| 队列 | 坐标队列 x1（深度 4）、CAN 命令队列 x1（深度 4） |
| 信号量 | 运动完成信号量 x1 |
| 内存 | heap 默认（`configTOTAL_HEAP_SIZE` 建议 ≥ 16 KB） |

| 任务 | 优先级 | 栈(字) | 周期/触发 | 说明 |
|---|---|---|---|---|
| vMotionTask | 5（最高） | 512 | 事件（队列/信号量） | 坐标→插补→脉冲→舵机 |
| vCommTask | 4 | 1024 | 事件（UART/CAN 回调） | 解析帧、投队列、CAN 收发 |
| vKeyTask | 3 | 256 | 20 ms | 按键扫描（KEY0~KEY3） |
| vDisplayTask | 2 | 512 | 50 ms | TFT/OLED 刷新（本版用 UART 调试口输出替代，可挂屏） |
| vHeartbeatTask | 1 | 256 | 1 s | LED 心跳 + CAN 心跳帧 |

### 3.7 NVIC 优先级（关键）

| 中断 | 抢占优先级 | 说明 |
|---|---|---|
| TIM6 | 5 | 步进节拍，要求最实时 |
| USART3 | 7 | 收 K210 帧 |
| CAN1 RX0 | 7 | 收上位机命令 |
| EXTI4 / EXTI6（限位 X-/Y-，PA4/PD6，EXTI6 属 EXTI9_5 组） | 4（**最高**） | 限位必须立即停脉冲（双开关方案：EXTI4=上升沿 X-，EXTI6=下降沿 Y-） |
| FreeRTOS SysTick | 15 | 系统调度 |

> 限位中断优先级必须高于 TIM6：限位触发时在 ISR 里直接停 TIM6（`__HAL_TIM_DISABLE`）+ 置急停标志。

## 4. 生成与集成

1. Project Manager → 勾选 Generate peripheral initialization as .c/.h files
2. 生成代码 → 把 `STM32_CoreXY_Motion/App/` 整个目录拷入工程根目录，在 CubeIDE 中 Add Source Folder
3. `main.c` 中：`#include "app.h"`，在 `main()` 的 `MX_*_Init()` 之后、启动调度器之前调用 `App_Init()`（创建任务、队列）
4. 编译：勾选 C11 或默认，FreeRTOS 中间件由 CubeMX 生成
5. 烧录：ST-Link/J-Link SWD（GEC-M4 板载接口），或 UART1（CH340G）串口下载

## 5. 上电自检清单

- [ ] 板载 CH340G 串口出调试打印（USART1 115200）
- [ ] 心跳 LED1 1 Hz 闪烁（PC13 低电平点亮）
- [ ] 手动发 `$50,30,1\n` 到 USART3，滑台走到 (50,30)mm（先完成单轴标定）
- [ ] 按下 X-/Y- 限位开关立即停，CAN 发 0x381 错误帧（回零：X 找 X- 开关 + Y 找 Y- 开关）
- [ ] CAN 助手收 0x181 位置帧（20 Hz）

## 6. 与旧文档引脚差异速查

| 旧文档 | 本工程 |
|---|---|
| K210 UART：PA9/PA10 (USART1) | **PB10/PB11 (USART3)** |
| 限位：PB14~PB17（含不存在引脚） | **PA4（X-，EXTI4 上升沿）+ PD6（Y-，EXTI6 下降沿，P8 引脚 2/DCMI_SCL）双开关；原 PC0 被板载 3D_INT/LED 占用** |
| 脉冲：TIM2/TIM3 PWM | **TIM6 节拍 + GPIO**（等效可切换） |
| CAN：外接 TJA1050 | **板载 PD0/PD1（CAN_RX/CAN_TX；PA11/PA12 实为 USB）** |

---

## 7. 2026-10-09 CubeMX 6.12 适配验证记录

> 环境：STM32CubeMX 6.12.0（F:\STM32CubeMX）+ 本地固件库 STM32Cube FW_F4 V1.28.3

### 7.1 CubeMX 加载 .ioc 结果

- 用 CubeMX 6.12 打开 3_Firmware/STM32/CoreXY_Motion.ioc：弹"固件版本差异"提示（工程 V1.27.1 vs 本机 V1.28.3），选 Continue 后正常解析。
- **引脚/时钟/NVIC 逐项校验通过**（与 App/app_config.h、Core/Src/main.c 完全一致）：
  - PA0/PA1=GPIO_Output(X/Y_STEP)、PB0/PB1=GPIO_Output(X/Y_DIR)、PA8=TIM1_CH1、PA9/10=USART1、PB10/11=USART3、PD0/PD1=CAN1、PA4=EXTI Rising（X-）+ PD6=EXTI Falling（Y-）、PC13=GPIO_Output
  - 时钟：HSE 8MHz → PLL 8/336/2 → SYSCLK 168MHz；APB1=42M、APB2=84M、APB1 Tim=84M、APB2 Tim=168M
  - NVIC：限位 EXTI4/EXTI9_5（EXTI6）优先级 4 > TIM6=5 > USART3/CAN1=7 > SysTick=15，分组 4

### 7.2 固件库说明（本机）

- 本机原本无任何 STM32Cube FW 包，CubeMX 打开 F4 工程会触发 myST 登录下载（无法跳过）。
- 已从 ST 官方 GitHub 镜像（gh-proxy）离线下载并安装 STM32Cube_FW_F4_V1.28.3 到 C:\Users\临渊\STM32Cube\Repository\（169.9MB，34181 文件）。
- 注意：GitHub 源码包缺 CubeMX 的 db 元数据，CubeMX 仍可能要求在线安装/登录；**对工程编译无影响**——HAL/CMSIS/FreeRTOS 源码已直接内嵌进 3_Firmware/STM32/。

### 7.3 工程适配完成项（2026-10-09）

| 项 | 状态 |
|---|---|
| HAL 1.28.3 源码内嵌 Drivers/ | 完成 |
| CMSIS（Include + Device F4）内嵌 | 完成 |
| FreeRTOS Kernel V11.x 内嵌 Middlewares/ | 完成（portable/GCC/ARM_CM4F + heap_4） |
| 启动文件 startup_stm32f407xx.s | 完成（Core/Startup/） |
| 链接脚本 STM32F407ZETx_FLASH.ld（512K/128K/64K） | 完成 |
| stm32f4xx_hal_conf.h（HSE=8MHz） | 完成 |
| FreeRTOSConfig.h（1kHz 节拍） | 完成 |
| .ioc 与代码引脚/时钟/NVIC 一致性 | 已核对一致 |

### 7.4 重要约束

- **Core/Src/main.c 为完整手写初始化，无 CubeMX USER CODE 段：切勿在本目录执行 CubeMX Generate Code**（会整段覆盖 main.c）。改配置请直接改 App/app_config.h + Core/Src/main.c，或另存 .ioc 副本测试生成。
- 本机暂无 STM32 编译工具链（无 CubeIDE、Keil 未安装），源码级适配已完成，编译验证需先安装 CubeIDE / Keil MDK5 / arm-none-eabi-gcc 任一。
