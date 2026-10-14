# K210 视觉模块（Maix Bit + OV2640）

## 1. 文件说明

| 文件 | 用途 |
| --- | --- |
| `main.py` | 主程序：红色目标识别 → 输出 `$cx,cy,flag\n` 坐标帧（烧录/部署此文件） |
| `test_uart.py` | UART1 回环测试：验证 IO6/IO7 串口接线 |
| `calibrate.py` | 阈值标定辅助：打印目标色块 LAB 均值，用于调 `RED_THRESHOLD` |

## 2. 运行方式（三选一）

1. **MaixPy IDE**：连接 K210（需固件 ≥ v0.3.1，低于此版本先用 kflash_gui 刷最新 MaixPy 固件）→ 打开 `main.py` → 运行。
2. **SD 卡自动运行**：把 `main.py` 拷入 FAT32 格式 microSD 卡根目录，插入 Maix Bit，上电自动运行（推荐用于最终部署）。
3. **串口 REPL**：USB 串口终端粘贴运行。

## 3. 硬件接线

### 摄像头（ATK-OV2640 红版 18pin → Maix Bit 24P FPC 摄像头座）

> ⚠️ **关键**：OV2640 的 **D0~D7 数据线是 K210 芯片专用引脚**（BGA 封装 M 行球位，见 K210 数据手册引脚图），**只在 24P FPC 摄像头座上引出，不在排针上**，无法用杜邦线飞线。

**接法（推荐，零引脚配置）**：通过 24P FPC 转 2.54mm 排针转接板（或直接使用带 FPC 排线的 OV2640 模块）插入 Maix Bit 摄像头座：

| OV2640 信号 | Maix Bit 侧（默认，固件自动配置） |
| --- | --- |
| D0~D7 | 24P FPC 摄像头座（芯片专用 DVP_D0~D7，1.8V） |
| SDA (SCCB) | IO40（默认 DVP_SDA） |
| SCL (SCCB) | IO41（默认 DVP_SCL） |
| RST | IO42（默认 DVP_RST） |
| VSYNC | IO43（默认 DVP_VSYNC） |
| PWDN | IO44（默认 DVP_PWDN，**必须接 GND 或拉低**） |
| HREF | IO45（默认 DVP_HSYNC） |
| XCLK | IO46（默认 DVP_XCLK，模块自带晶振时可不接） |
| PCLK | IO47（默认 DVP_PCLK） |
| VCC / GND | 3.3V / GND |

> 若坚持飞线到排针：仅控制线可重映射（代码里 `fm.register` 改映射），数据线 D0~D7 仍必须走 FPC 座。

### 串口（K210 → STM32）

| K210 | 信号 | STM32 (USART3) |
| --- | --- | --- |
| IO6 | UART1_TX | PB11 (USART3_RX) |
| IO7 | UART1_RX | PB10 (USART3_TX)（可选，本程序不接收） |
| GND | 共地 | GND |

> ⚠️ IO4/IO5 是 UARTHS（板载 CH552 USB 串口桥，IDE/REPL 用），**不可用作 UART1**。波特率 115200-8-N-1。

## 4. 通信协议（与 STM32 固件对齐）

```
帧格式: $cx,cy,flag\n
cx/cy : 目标像素中心坐标 (QVGA 320x240 坐标系, cx∈[0,320), cy∈[0,240))
flag  : 1=目标存在  0=无目标(保活帧)
```

- STM32 侧解析：`App/comm_uart.c` `sscanf(s_buf, "$%d,%d,%d", &px, &py, &flag)`，`flag==1` 时像素坐标经仿射变换转 mm 后进入运动队列。
- 发送周期：约 20Hz（`FRAME_PERIOD_MS=50`），防止 STM32 队列积压。
- 像素→mm 仿射：固件 `app_config.h` 的 `AFFINE_DEFAULT`（默认 {1,0,0,0,1,0}），联调前按实际安装位置标定。

## 5. 联调步骤

1. **IDE 验证图像**：MaixPy IDE 连接后运行 `main.py`，右侧应实时显示画面并框出红色目标。
2. **阈值标定**：跑 `calibrate.py`，把目标放画面中心，读取打印的 L/A/B 值，回填 `main.py` 的 `RED_THRESHOLD`。
3. **串口验证**：跑 `test_uart.py` 或直接看 `main.py` 串口输出，确认 `$cx,cy,1` 帧格式正确（PC 串口助手接 IO6 观察）。
4. **联调**：K210 IO6 → STM32 PB11（共地），STM32 串口/CAN 观察运动队列是否收到坐标并执行抓取。

## 6. 常见问题

| 现象 | 原因 / 处理 |
| --- | --- |
| IDE 连不上 | 固件版本 < 0.3.1，用 kflash_gui 刷最新 MaixPy 固件 |
| 画面全黑/花屏 | 检查 FPC 排线方向与 PWDN 是否拉低；`sensor.set_vflip/hmirror` 按安装方向调整 |
| 找不到目标 | 阈值不对（用 calibrate.py 标定）；pixels_threshold 过大；环境光不足 |
| 误检多 | 提高 PIXELS_THRESHOLD/AREA_THRESHOLD；调整光照避免反光 |
| 串口无输出 | 检查 IO6 是否误接 IO4/5；波特率 115200；是否共地 |
