# STM32_CoreXY_Motion — 两轴视觉抓取运动控制工程

> 目标板：STM32F407ZET6（GEC-M4，板载 TJA1050 CAN / CH340G 串口）
> 工具链：Keil MDK5 5.43（AC6 V6.24，已编译通过） / STM32CubeIDE / Makefile+arm-gcc
> 中间件：HAL 1.28.3（本地固件库）+ FreeRTOS Kernel V11.x（源码已内嵌）
> 配套文档：`01_Docs/02_开发文档/`、`02_CubeMX配置清单.md`（历史交付）
>
> **重要**：本工程 `Core/Src/main.c` 为完整手写初始化（无 CubeMX USER CODE 段），
> **切勿在本目录执行 CubeMX Generate Code**（会整段覆盖 main.c）。
> 需要改配置时：改 `.ioc` → 在 CubeMX 中仅查看/核对（本机需先安装 FW_F4 固件包），
> 或直接改 `App/app_config.h` 与 `Core/Src/main.c` 后重新编译。

## 1. 功能

| 功能 | 实现 |
|---|---|
| 运动学 | CoreXY：`m1 = x+y, m2 = x-y`，节拍中断逐 tick 反解 |
| 插补 | Bresenham 直线插补（双轴同时到位） |
| 加减速 | 梯形加减速（加速/匀速/减速），动态改 TIM6 ARR |
| 脉冲输出 | TIM6 节拍中断 + GPIO STEP/DIR（等效 PWM 方案，直线性更好） |
| 视觉接口 | USART3 收 K210 帧 `$cx,cy,flag\n`，仿射像素→mm |
| 舵机 | TIM1_CH1 PWM 50Hz，MG90S 开合（独立 5V 供电！） |
| 限位 | PC0~PC3 EXTI 高优先级中断，触发立即停脉冲 |
| 通信 | CAN1 板载 TJA1050：0x181 位置 / 0x281 命令 / 0x381 错误 |
| 任务 | 5 个 FreeRTOS 任务（Motion 最高优先级） |

## 2. 目录结构

```
STM32_CoreXY_Motion/
├── CoreXY_Motion.ioc      # CubeMX 工程（完整配置：引脚/时钟168MHz/NVIC已核对）
├── Core/
│   ├── Inc/main.h
│   ├── Inc/stm32f4xx_hal_conf.h   # HAL 配置（HSE=8MHz，启用所需模块）
│   ├── Inc/FreeRTOSConfig.h       # FreeRTOS 配置（1kHz 节拍，heap_4）
│   ├── Src/main.c         # 完整 HAL 初始化 + App_Init + 调度器启动
│   ├── Src/stm32f4xx_it.c # 中断服务程序（IRQHandler → HAL → App 回调）
│   └── Startup/           # startup_stm32f407xx.s + STM32F407ZETx_FLASH.ld + CoreXY_Motion.sct
├── MDK-ARM/               # Keil MDK5 工程（AC6 编译通过，产物见第 4 节）
│   └── CoreXY_Motion.uvprojx
├── Drivers/               # 来自 STM32Cube FW_F4 V1.28.3（已内嵌）
│   ├── CMSIS/             # 内核头 + Device F4
│   └── STM32F4xx_HAL_Driver/  # HAL 源码（含 Inc/Src）
├── Middlewares/
│   └── Third_Party/FreeRTOS/  # FreeRTOS Kernel V11.x（include/portable/MemMang）
└── App/                   # 应用层（引脚/协议/运动参数改 app_config.h）
    ├── app.h              # 汇总头
    ├── app_config.h       # 引脚/机械参数/协议（改这里）
    ├── corexy.c/h         # CoreXY 运动学
    ├── interp.c/h         # Bresenham 插补
    ├── profile.c/h        # 梯形加减速
    ├── stepper.c/h        # TIM6 节拍 + 脉冲输出（HAL 回调）
    ├── limit.c/h          # 限位 EXTI
    ├── servo.c/h          # 舵机
    ├── comm_uart.c/h      # USART3 K210 帧解析
    ├── comm_can.c/h       # CAN1
    ├── key.c/h            # 板载按键（默认屏蔽，见 app_config.h）
    └── motion.c/h         # 运动任务 + 状态机 + 抓取流程
```

> 备份：`../STM32_backup_20261009/` 为集成前的原始工程（勿改动）。

## 3. 接线（GEC-M4）

| 信号 | 引脚 | 接向 |
|---|---|---|
| X STEP | PA0 | HW-434 扩展板 X 口 STP |
| Y STEP | PA1 | HW-434 扩展板 Y 口 STP |
| X DIR | PB0 | HW-434 X 口 DIR |
| Y DIR | PB1 | HW-434 Y 口 DIR |
| 舵机 PWM | PA8 | MG90S 信号线（舵机独立 5V/2A 供电） |
| K210 UART | PB10(TX)/PB11(RX) | K210 UART RX/TX（共地！） |
| CAN | PA11/PA12 | 板载 TJA1050 → CANH/CANL |
| 限位 X- X+ Y- Y+ | PC0 PC1 PC2 PC3 | 微动开关一端 3.3V、一端引脚（按下=高） |
| LED1 | PC13 | 心跳，低电平点亮 |
| 调试串口 | PA9/PA10 | 板载 CH340G（115200 printf） |

## 4. 编译与集成

本工程为**完整可编译结构**（HAL/CMSIS/FreeRTOS/启动文件/链接脚本已内嵌），不再依赖 CubeMX 生成。
三种编译路径任选：

**A. STM32CubeIDE（推荐，免费）**
1. 安装 STM32CubeIDE，新建空 F407 工程后，把本目录 `Core/`、`App/`、`Drivers/`、`Middlewares/` 拷入工程（或 File→Import 时勾选 Existing Project，选择本目录——需先生成 .project/.cproject）
2. 工程属性勾选 `Use MicroLIB`（printf 用）
3. 编译烧录（SWD/J-Link 或 CH340G 串口）

**B. Keil MDK5（AC6）— 已在本机编译通过 ✅**
1. 环境：Keil MDK 5.43 + STM32F4xx_DFP 2.14.0 器件包（`F:\MDK5\ARM\PACK\Keil\STM32F4xx_DFP\2.14.0`，本机已装）
2. 直接用现成工程：双击打开 `MDK-ARM/CoreXY_Motion.uvprojx`，F7 编译
3. 命令行编译：`F:\MDK5\UV4\UV4.exe -b MDK-ARM\CoreXY_Motion.uvprojx -j0 -o MDK-ARM\build_log.txt`
4. 产物：`MDK-ARM/CoreXY_Motion.axf`（ELF）/ `.hex`（烧录）/ `.map`（内存映射）
5. 烧录：ST-Link 或 J-Link（SWD），或串口 ISP；调试串口 CH340G 115200

> 本机 2026-10-09 验证结果：**0 Error / 0 Warning**，
> `Program Size: Code=50144 RO-data=1128 RW-data=52 ZI-data=28356`
> （Flash ≈51KB @512KB，RAM ≈28KB @128KB，余量充足）
>
> Keil 工程要点（已在 .uvprojx 内配好）：
> - AC6 编译器（V6.24），`-std=gnu99 -fno-common -mfloat-abi=hard -mfpu=fpv4-sp-d16`，Target FPU2（单精度）
> - 启动文件用 GCC 语法 `startup_stm32f407xx.s`（Keil AC6 的 armasm 汇编 `--fpu=FPv4-SP`），分散加载 `Core/Startup/CoreXY_Motion.sct`（512K/128K）
> - FreeRTOS 钩子 `vApplicationStackOverflowHook`/`vApplicationMallocFailedHook` 已在 main.c 实现
> - 中断中转 `stm32f4xx_it.c`（TIM6_DAC/EXTI0~3/USART3/CAN1_RX0）为手写补全

**C. Makefile + arm-none-eabi-gcc**
链接脚本用 `Core/Startup/STM32F407ZETx_FLASH.ld`（512K Flash / 128K RAM / 64K CCM），
启动文件用 `Core/Startup/startup_stm32f407xx.s`，FreeRTOS 用 GCC/ARM_CM4F 移植层。

> 本机（2026-10）Keil MDK5 5.43 + STM32F4xx_DFP 2.14.0 已装，**AC6 编译 0 错误 0 警告通过**；
> `.ioc` 已通过 CubeMX 6.12 解析校验（引脚/时钟/NVIC 与代码一致）。

## 5. 关键参数（app_config.h）

| 参数 | 默认值 | 说明 |
|---|---|---|
| STEPS_PER_MM | 100.0 | 1/16 细分理论值，**实测校准后改** |
| V_MAX_STEPS | 10000 | 100 mm/s |
| ACCEL_STEPS2 | 50000 | 500 mm/s² |
| V_START_STEPS | 500 | 起始节拍频率 |
| DUMP_X/DUMP_Y | 10/140 | 投放点（mm） |
| SERVO_CCR_MIN/MAX | 500/2500 | 0°~180° |

校准方法：手动发一段已知长度（如 `motion_move_mm` 走 100mm），数实际步数，反算 STEPS_PER_MM。

## 6. 协议

**K210 → STM32（USART3 115200）**：`$px,py,flag\n`（像素坐标，flag=1 目标存在）

**STM32 ↔ 上位机（CAN 500k）**：
| ID | 方向 | 载荷 |
|---|---|---|
| 0x181 | 上报 | pos_x(int16 LE,0.001mm) pos_y(int16 LE,0.001mm) status(uint8) |
| 0x281 | 下发 | cmd: 01回零 02手动 03自动 04停止；06手动移动 + x(mm int16 LE) + y(mm int16 LE) |
| 0x381 | 上报 | err(uint16 LE) |

## 7. 上电与联调

1. 上电：LED1 1Hz 闪烁，CH340G 串口打印 `[pos] x=... y=... status=0 mode=0`
2. 单轴标定：发 `$100,0,1\n` 观察 X 轴移动方向/距离，修正 DIR 逻辑与 STEPS_PER_MM
3. 限位测试：手动触发任一限位，立即停并打印错误
4. 回零：CAN 发 0x01，滑台朝 X-/Y- 移动至限位停，位置清零
5. 整链：auto 模式 + K210 发红色块坐标 → 移动→抓取→投放

## 8. 已知约束与待确认

- **KEY 按键默认屏蔽**（`KEY_DISABLE`）：GEC-M4 丝印未核实，确认后改 `app_config.h`
- ST7789 SPI TFT 未纳入本版（GEC-M4 的 SPI1 被 W25Q128/NRF 占用），显示任务用 USART1 打印代替；需要屏时用软件 SPI 或 PD/PE 空闲引脚
- A4988 细分以拨码为准（照片全 ON 疑似 1/16），实测步/mm 后更新参数
- 舵机必须独立 5V 供电，禁止从板载 5V 取电
