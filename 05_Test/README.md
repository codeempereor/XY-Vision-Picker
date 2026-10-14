# 测试

## 三位一体协议联调测试（无硬件仿真）

`test_three_in_one.py`：复刻 K210 → STM32 → CAN → 上位机整条协议链路的纯逻辑仿真测试，覆盖 7 个场景：

1. 自动模式 + K210 目标帧 → 解析/决策/行程保护/CAN 编码/上位机显示
2. 自动模式无目标帧 → 忽略
3. 手动模式收到 K210 帧 → 忽略
4. 手动模式 + CAN MOVE → 执行移动 + 状态上报
5. 越界保护（行程保护拦截 + 错误状态）
6. Qt 上位机 sendMove 字节流与固件解析逐字节核对
7. 帧格式完整性（乱码/缺前缀丢弃）

运行方式：

```bash
python 05_Test/test_three_in_one.py
```

对应真实源码：K210 `03_Firmware/K210_Visual/main.py`；STM32 `App/comm_uart.c`、`App/motion.c`、`App/comm_can.c`；上位机 `03_Firmware/PC_HMI/mypro00001/mainwindow.cpp`。

## 子目录

- `单元测试/`：模块级测试（详见该目录 README）
- `联调记录/`：实机联调过程与结果（详见该目录 README）
