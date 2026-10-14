# 08. Keil MDK5 编译适配记录（2026-10-09）

> 目标：用本机 Keil MDK5 将 STM32 工程完整编译跑通，产出可烧录 hex。
> 状态：**编译通过（0 Error / 0 Warning）**，hex 已生成。

## 1. 环境确认

| 项 | 值 |
|---|---|
| Keil μVision | 5.43.1（F:\MDK5，MDK Professional 完整版，无 32KB 限制） |
| 编译器 | AC6 / armclang V6.24（F:\MDK5\ARM\ARMCLANG） |
| 器件包 | Keil.STM32F4xx_DFP 2.14.0（244MB，keil.com 官方，解压至 F:\MDK5\ARM\PACK\Keil\STM32F4xx_DFP\2.14.0） |
| 工程文件 | `03_Firmware/STM32/MDK-ARM/CoreXY_Motion.uvprojx` |

## 2. 工程配置要点（uvprojx 内已配好）

- **器件**：STM32F407ZET6，Target FPU2（单精度浮点），`Cpu` 含 `IRAM(0x20000000,0x20000) IROM(0x08000000,0x80000)`
- **C 编译**：AC6，`-std=gnu99 -fno-common -mfloat-abi=hard -mfpu=fpv4-sp-d16`；宏 `USE_HAL_DRIVER, STM32F407xx`
- **汇编**：armasm，`--fpu=FPv4-SP`（启动文件 GCC 语法，armasm 可汇编）
- **分散加载**：`Core/Startup/CoreXY_Motion.sct`（512K Flash / 128K RAM）
- **源码组**：Core（main.c + stm32f4xx_it.c + system_stm32f4xx.c）、App（10 模块）、HAL（15 个必需 .c）、FreeRTOS（内核 7 文件 + GCC/ARM_CM4F port.c + heap_4）、Startup
- **链接**：`--diag_suppress=6314,6329,L6312W`

## 3. 编译问题修正记录（483 → 0）

| # | 问题 | 根因 | 修正 |
|---|---|---|---|
| 1 | 483 个 `unknown type name '__IO'/'uint32_t'` | 手写 `stm32f4xx_hal_conf.h` 缺少标准模块头包含链（类型链：模块头→hal_def.h→stm32f4xx.h→CMSIS） | 补全 hal_conf.h 尾部 Includes 段（rcc/gpio/dma/cortex/can/tim/uart/pwr/flash 等 14 个头） |
| 2 | `QueueHandle_t/SemaphoreHandle_t` 未定义 | App/motion.h 未包含 FreeRTOS 头 | motion.h 加 `FreeRTOS.h/queue.h/semphr.h` |
| 3 | `HAL_TIM_MspPostInit` 冲突/隐式声明 | CubeMX 私有辅助函数在 HAL 头无声明，先调用后定义 | main.c 顶部加前置声明 |
| 4 | port.c: `not configured to enable hardware floating point` | AC6 未传浮点编译选项 | C 编译加 `-mfloat-abi=hard -mfpu=fpv4-sp-d16` |
| 5 | stream_buffer.c: `INCLUDE_xTaskGetCurrentTaskHandle must be 1` | FreeRTOSConfig 该宏为 0 而内核 V11 编译 stream_buffer.c 需要 | 置 1 |
| 6 | `L6366E: attributes are not compatible` | 汇编对象（softfp）与 C 对象（hardfp）ABI 属性冲突 | Asm 加 `--fpu=FPv4-SP`；Target Cpu 由 FPU3（双精度）改 FPU2（单精度） |
| 7 | `L6218E: vApplicationStackOverflowHook/MallocFailedHook 未定义` | config 启用钩子但无实现 | main.c 实现两钩子（故障时 LED 快闪死循环） |
| 8 | 3 个 `-Wformat` 警告 | `sscanf "$%ld"` 与 `int32_t*` 类型不匹配 | 改 `%d` |

## 4. 编译结果

```
*** Using Compiler 'V6.24', folder: 'F:\MDK5\ARM\ARMCLANG\Bin'
Program Size: Code=50144 RO-data=1128 RW-data=52 ZI-data=28356
FromELF: creating hex file...
".\CoreXY_Motion.axf" - 0 Error(s), 0 Warning(s).
```

| 产物 | 路径 | 说明 |
|---|---|---|
| CoreXY_Motion.axf | MDK-ARM/ | ELF，Keil 调试用 |
| **CoreXY_Motion.hex** | MDK-ARM/ | **Intel HEX，烧录用** |
| CoreXY_Motion.map | MDK-ARM/ | 内存映射（代码/数据分布） |

资源占用：Flash ≈51KB / 512KB；RAM ≈28KB / 128KB（余量充足）。

## 5. 复现命令

```bat
F:\MDK5\UV4\UV4.exe -b <工程>\MDK-ARM\CoreXY_Motion.uvprojx -j0 -o <工程>\MDK-ARM\build_log.txt
```

或 Keil GUI：双击 `CoreXY_Motion.uvprojx` → 编译（F7）。

## 6. 烧录

- SWD：ST-Link / J-Link（Target 已配置 Flash 算法 STM32F407ZETx_512K）
- 串口 ISP：CH340G + FlyMcu（需 BOOT0=1 上电）

## 7. 注意事项

- `main.c` 无 USER CODE 段，**勿在工程目录执行 CubeMX Generate**（整段覆盖）
- 本工程为源码内嵌结构（HAL/CMSIS/FreeRTOS/启动文件/链接脚本全部随工程），不依赖 CubeMX 再生成
- 启动文件为 GCC 语法版，Keil AC6 的 armasm（`--fpu=FPv4-SP`）可直接汇编；若换回标准 Keil 启动文件（DFP 内），同样适用
