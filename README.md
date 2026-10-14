# XY-Vision-Picker

基于 STM32F407 + K210 的两轴 X-Y 视觉识别抓取系统。

## 项目简介

- **视觉识别**：K210 + OV2640 摄像头，颜色分割 + 连通域分析 + 质心提取
- **运动控制**：STM32F407ZET6（GEC-M4 开发板）+ FreeRTOS，CoreXY 双 42 步进电机滑台
- **抓取执行**：3D 打印多指舵机夹爪（MG90S）
- **人机交互**：ST7789V SPI 显示屏 + 4x4 矩阵键盘
- **通信协议**：K210 <-> STM32 用 UART（自定义帧协议），STM32 <-> 上位机用 CAN

## 目录结构

```
XY_Vision_Picker/
├── 01_Docs/                    # 项目文档
│   ├── 01_方案设计/            # 系统设计方案
│   ├── 02_开发文档/            # 各模块开发文档（含 CubeMX 配置清单、硬件接线指南）
│   ├── 02_开发日志/            # 开发记录、进度（6 份）
│   ├── 03_参考文献/            # 参考资料索引（原件在 06_Resources）
│   └── 04_演示材料/            # PPT 等演示交付物
├── 02_Hardware/                # 硬件资料
│   ├── 01_原理图/              # GEC-M4 / OV2640 / USB-CAN 原理图
│   ├── 02_数据手册/            # 数据手册索引（原件在 06_Resources）
│   ├── 03_BOM清单/             # BOM 清单
│   ├── 04_接线图/              # 滑台/驱动接线图与接线要点
│   └── 05_实物照片/            # 实物与联调实拍
├── 03_Firmware/                # 代码工程（monorepo）
│   ├── STM32/                  # 运动控制（Keil MDK-ARM + FreeRTOS）
│   │   ├── Core/
│   │   ├── Drivers/
│   │   ├── App/
│   │   └── Middlewares/
│   ├── K210/                   # 视觉识别（规划目录）
│   │   ├── c_sdk/             # C/C++ SDK 版本（预留升级）
│   │   ├── py/                 # MaixPy 脚本（预留）
│   │   ├── vision/             # 视觉算法/模型归档（预留）
│   │   └── calibration/        # 标定归档（预留）
│   ├── K210_Visual/            # 当前 K210 MaixPy 视觉代码（main.py 等）
│   └── PC_HMI/                 # 上位机（Qt + QCanBus）
│       ├── mypro00001/         # Qt 工程源码
│       ├── src/                # 源码归档区（预留）
│       └── qcustomplot/        # 绘图库（预留）
├── 04_Tools/                   # 工具软件索引与本地归档
│   ├── 烧录工具/
│   └── 调试工具/               # PEAK 驱动（本地归档，未入库）
├── 05_Test/                    # 测试
│   ├── test_three_in_one.py    # 三位一体协议仿真测试（无硬件）
│   ├── 单元测试/
│   └── 联调记录/
├── 06_Resources/               # 第三方参考资料（滑台/PCAN/STM32F407/OV2640）
└── README.md
```

## 三个工程

| 工程 | 路径 | 工具链 | 对外接口 |
|------|------|--------|----------|
| K210 视觉 | `03_Firmware/K210_Visual/` | MaixPy IDE | UART → STM32 |
| STM32 运动 | `03_Firmware/STM32/`（MDK-ARM 目标） | Keil MDK-ARM + ST-Link | UART↔K210, CAN↔PC |
| PC 上位机 | `03_Firmware/PC_HMI/mypro00001/` | Qt Creator + MinGW | CAN ↔ STM32 |

## 通信协议

- **K210 ↔ STM32**：UART 115200-8-N-1，文本帧 `$cx,cy,flag\n`
- **PC ↔ STM32**：CAN 500kbps，标准帧 ID 0x181 / 0x281 / 0x381
  - 0x181 上报：pos_x / pos_y（int16 小端，0.001mm）+ status（0空闲 1回零 2手动 3自动 4抓取 5完成 6错误）
  - 0x281 下发：cmd = 0x01 回零 / 0x02 手动 / 0x03 自动 / 0x04 停止；cmd = 0x06 手动移动 + X(mm,int16 小端) + Y(mm,int16 小端)
  - 0x381 上报：err（uint16 小端：0x0001 限位急停 / 0x0002 越界 / 0x0003 超时）

## K210 升级路径

当前用 MaixPy（Python），后续如需更高性能（YOLO 推理、帧率提升），迁移到 `K210/c_sdk/` 目录下用 Kendryte SDK C 开发，对外 UART 协议不变，STM32 侧零修改。

## 硬件清单

| 模块 | 型号 | 数量 |
|---|---|---|
| 运动主控 | STM32F407ZET6 开发板（GEC-M4） | 1 |
| 视觉协处理器 | K210 开发板（Maix Bit） | 1 |
| 摄像头 | 正点原子 ATK-OV2640 红版（YX64074，2×9 排针，飞线至 K210 DVP） | 1 |
| 运动平台 | CoreXY 十字滑台（含 2 个 42 步进电机） | 1 |
| 电机驱动 | HW-434 扩展板 + A4988 模块（MS1~MS3 拨码调细分） | 2 |
| 夹爪 | 3D 打印多指舵机夹爪（含 MG90S） | 1 |
| 显示屏 | ST7789V SPI TFT | 1 |
| 键盘 | 4x4 矩阵键盘 | 1 |
| 限位开关 | 卧式微动开关（X-：PA4 高触发 / Y-：PD6 低触发） | 2 |
| USB-CAN | USB 转 CAN 适配器（UTC-T01，CANH/CANL/GND + 120R 终端开关） | 1 |
| 电源 | 24V 主电源（驱动板 VMOT）+ 5V 舵机独立供电 | 各1 |
| 调试器 | ST-LINK V2（J-Link 备用） | 1 |

> 详细 BOM 见 `02_Hardware/03_BOM清单/`，接线见 `02_Hardware/04_接线图/` 与 `01_Docs/02_开发文档/10_硬件接线指南.docx`。
