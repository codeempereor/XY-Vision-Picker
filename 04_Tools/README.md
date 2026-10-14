# 工具软件

本项目开发、烧录、调试所需的工具软件索引。

| 工具 | 用途 | 位置 |
|---|---|---|
| Keil MDK-ARM | STM32 固件编译/烧录/调试 | 本机 `F:\MDK5`（工程见 `03_Firmware/STM32`，MDK-ARM 目标） |
| STM32CubeMX | CubeMX 配置生成工程 | 安装包在 D 盘资料包（`D:\YQ\Notes\04-Notes\软件基础资料包\SetupSTM32CubeMX-6.12.0-Win.exe`），配置清单见 `01_Docs/02_开发文档` |
| MaixPy IDE | K210 视觉开发 | 安装于本机 `E:\MaixPy IDE`（0.2.5），代码见 `03_Firmware/K210_Visual` |
| Qt Creator + MinGW | PC 上位机开发 | 工程见 `03_Firmware/PC_HMI/mypro00001` |
| ST-Link 驱动 | ST-LINK V2 调试器 | `D:\YQ\Notes\04-Notes\软件基础资料包\ST_Link\`（stlink_winusb_install.bat） |
| J-Link 驱动/软件 | J-Link 调试器 | `D:\YQ\Notes\04-Notes\软件基础资料包\Setup_JLink_V492\` |

## 子目录

- `烧录工具/`：固件烧录/下载工具（索引见该目录 README）
- `调试工具/`：调试与总线工具（含 PEAK 驱动安装包，见该目录 README）
