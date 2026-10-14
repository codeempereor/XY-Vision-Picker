# -*- coding: utf-8 -*-
# 串口回环测试: 验证 K210 UART1 (IO6/IO7) 接线是否正常
# 用法: 将 IO6 与 IO7 杜邦线短接后运行, 屏幕应每 200ms 打印一次 "OK"
#       或将 IO6 接 USB-TTL 的 RX, 在电脑串口助手上应收到 hello 帧
import time
from fpioa_manager import fm
from machine import UART

fm.register(6, fm.fpioa.UART1_TX, force=True)
fm.register(7, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, 115200, 8, None, 0, timeout=1000, read_buf_len=64)

while True:
    uart.write('hello\n')          # 发出测试帧
    time.sleep_ms(100)
    if uart.any():
        data = uart.read()
        print('OK rx:', data)      # 短接 IO6<->IO7 时这里应收到 hello
    time.sleep_ms(100)
