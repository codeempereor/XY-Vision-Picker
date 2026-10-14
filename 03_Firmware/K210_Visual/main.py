# -*- coding: utf-8 -*-
# ============================================================================
# XY_Vision_Picker - K210 视觉模块主程序
# 板卡 : Sipeed Maix Bit (K210) + OV2640 摄像头(24P FPC 接入 DVP 座)
# 功能 : 识别画面中的红色目标, 输出目标像素中心坐标帧到 UART1
# 协议 : $cx,cy,flag\n   (与 STM32 固件 comm_uart.c 的 "%d,%d,%d" 解析一致)
#         flag=1 目标存在 / flag=0 无目标(保活帧)
# 波特率: 115200-8-N-1
# 接线 : K210 IO16(UART1_TX) -> STM32 PB11(USART3_RX)
#         K210 IO17(UART1_RX) -> STM32 PB10(USART3_TX)   [可选, 本程序不接收]
#         两端共地 (GND <-> GND)
# ============================================================================
import sensor
import image
import time
from fpioa_manager import fm
from machine import UART

# ---------------------------------------------------------------------------
# 1. OV2640 摄像头初始化 (必须最先做, 且不要 fm.register 摄像头占用脚)
#    Maix Bit 上 DVP 摄像头固定占用 IO2~IO15 (XCLK/SCCB/VSYNC/HREF/D0~D7/PCLK),
#    force=True 抢这些脚会直接导致 sensor.reset() -> OSError: Reset Failed
# ---------------------------------------------------------------------------
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)      # 320x240
sensor.set_vflip(0)                    # 画面上下翻转, 依安装方向改 0/1
sensor.set_hmirror(0)                  # 画面左右镜像, 依安装方向改 0/1
sensor.skip_frames(time=1000)          # 跳过前 1s 让自动曝光收敛
sensor.set_auto_gain(1)                # 自动增益(颜色识别建议开启)
sensor.set_auto_whitebal(1)            # 自动白平衡

# ---------------------------------------------------------------------------
# 2. UART1: IO16=TX / IO17=RX  (这两个是排针空闲脚, 不跟摄像头/板载CH552冲突)
#    注意: TX/RX 交叉接, Maix.TX -> STM32.RX, Maix.RX -> STM32.TX
# ---------------------------------------------------------------------------
fm.register(16, fm.fpioa.UART1_TX, force=True)
fm.register(17, fm.fpioa.UART1_RX, force=True)
uart = UART(UART.UART1, 115200, 8, None, 0, timeout=1000, read_buf_len=64)

# ---------------------------------------------------------------------------
# 3. 红色目标 LAB 阈值 (L:0-100, A:-127~127, B:-127~127)
#    在 MaixPy IDE 中: 工具 -> 机器视觉 -> 阈值编辑器 对着实物调
#    典型红色: (30, 100, 15, 127, -10, 127)
# ---------------------------------------------------------------------------
RED_THRESHOLD = (30, 100, 15, 127, -10, 127)
PIXELS_THRESHOLD = 60      # 最小像素数, 过滤噪点
AREA_THRESHOLD = 60        # 最小面积
FRAME_PERIOD_MS = 50       # 坐标帧发送周期(20Hz), 避免 STM32 队列积压

last_send = time.ticks_ms()

while True:
    img = sensor.snapshot()
    blobs = img.find_blobs([RED_THRESHOLD],
                           pixels_threshold=PIXELS_THRESHOLD,
                           area_threshold=AREA_THRESHOLD,
                           merge=True)                 # 相近色块合并
    if blobs:
        # 取面积最大的色块作为目标
        blob = max(blobs, key=lambda b: b.pixels())
        cx, cy = blob.cx(), blob.cy()
        img.draw_rectangle(blob.rect(), color=(0, 255, 0))
        img.draw_cross(cx, cy, color=(0, 255, 0))
        frame = '$%d,%d,1\n' % (cx, cy)
    else:
        frame = '$0,0,0\n'                             # 无目标保活帧

    now = time.ticks_ms()
    if time.ticks_diff(now, last_send) >= FRAME_PERIOD_MS:
        uart.write(frame)
        last_send = now
