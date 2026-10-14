# 烧录工具

| 工具 | 用途 | 位置 |
|---|---|---|
| STM32CubeProgrammer（备选） | STM32 固件烧录 | 随 CubeMX 套件安装；也可直接用 Keil 的 Flash Download 烧录 |
| kflash（备选） | K210 固件烧录 | MaixPy IDE 内置烧录；命令行工具按需安装 |

> 当前实际烧录方式：STM32 用 Keil + ST-Link（SWD）下载；K210 用 MaixPy IDE 连接开发板烧录 `main.py`。
