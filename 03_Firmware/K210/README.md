# K210 视觉模块

## 当前代码（MaixPy）

视觉识别代码位于 `../K210_Visual/`（main.py / calibrate.py / test_uart.py / README.md）：

- `main.py`：主程序——OV2640 采集 → 颜色分割 → 连通域/质心 → UART 输出 `$cx,cy,flag\n`
- `calibrate.py`：坐标标定脚本
- `test_uart.py`：UART 通信自测

## 目录规划

| 目录 | 状态 | 说明 |
|---|---|---|
| `py/` | 预留 | MaixPy 脚本目录（当前代码以 K210_Visual/ 为实际路径，见上） |
| `c_sdk/` | 预留 | Kendryte C/C++ SDK 版本（升级路径，见 c_sdk/README.md） |
| `vision/` | 预留 | 视觉算法/模型文件归档位 |
| `calibration/` | 预留 | 标定参数/标定记录归档位 |

> 升级到 c_sdk 后对外 UART 协议不变，STM32 侧零修改（详见根 README「K210 升级路径」）。
