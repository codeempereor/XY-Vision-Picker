# -*- coding: utf-8 -*-
# 阈值标定辅助: 将目标放在画面中心, 观察打印出的 LAB 值, 用于调整 RED_THRESHOLD
# 在 MaixPy IDE 串口终端 / 或 USB 串口 REPL 中查看输出
import sensor
import image

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=1000)

# 用一个宽泛红色阈值抓取最大色块, 打印其平均 LAB
ROUGH_RED = (20, 100, 0, 127, -20, 127)

while True:
    img = sensor.snapshot()
    blobs = img.find_blobs([ROUGH_RED], pixels_threshold=30, area_threshold=30, merge=True)
    if blobs:
        b = max(blobs, key=lambda x: x.pixels())
        stat = img.get_statistics(roi=b.rect())   # 色块区域的统计
        print('LAB: L=%d A=%d B=%d  cx=%d cy=%d' %
              (stat.l_mean(), stat.a_mean(), stat.b_mean(), b.cx(), b.cy()))
    else:
        print('no blob, 请把红色目标对准镜头')
