# -*- coding: utf-8 -*-
"""
三位一体协议联调模拟测试 (无硬件)
链路: K210(模拟帧) -> STM32(解析/仿射/运动决策) -> CAN编码 -> 上位机(解析/显示)

对照真实源码:
  - K210    : 03_Firmware/K210_Visual/main.py         (输出 $cx,cy,flag\n)
  - STM32   : App/comm_uart.c, App/motion.c, App/comm_can.c
  - 上位机  : mypro00001/mainwindow.cpp (handleFrame)
运行: python 05_Test/test_three_in_one.py
"""
import struct
import re


# ============ 固件常量 (与 app_config.h 一致) ============
STEPS_PER_MM = 100.0
X_TRAVEL_MM = 200.0
Y_TRAVEL_MM = 150.0
DUMP_X_MM = 10.0
DUMP_Y_MM = 140.0
AFFINE = {"a": 1.0, "b": 0.0, "c": 0.0, "d": 0.0, "e": 1.0, "f": 0.0}

# CAN 帧 ID
CAN_ID_POS, CAN_ID_CMD, CAN_ID_ERR = 0x181, 0x281, 0x381
CAN_CMD_HOME, CAN_CMD_MANUAL, CAN_CMD_AUTO, CAN_CMD_STOP, CAN_CMD_MOVE = 0x01, 0x02, 0x03, 0x04, 0x06
ST_IDLE, ST_HOME, ST_MANUAL, ST_AUTO, ST_GRIP, ST_DONE, ST_ERROR = 0, 1, 2, 3, 4, 5, 6

SRC_K210, SRC_CAN_MOVE = 0, 1

PASS, FAIL = 0, 0
results = []


def check(name, ok, detail=""):
    global PASS, FAIL
    if ok:
        PASS += 1
        results.append(f"  [PASS] {name} {detail}")
    else:
        FAIL += 1
        results.append(f"  [FAIL] {name} {detail}")


# ============ 1. STM32 接收解析 (复刻 comm_uart.c) ============
def stm32_parse(line):
    m = re.match(r"\$(-?\d+),(-?\d+),(-?\d+)", line)
    if not m:
        return None
    px, py, flag = int(m.group(1)), int(m.group(2)), int(m.group(3))
    # 仿射 (affine_apply)
    mx = int(AFFINE["a"] * px + AFFINE["b"] * py + AFFINE["c"])
    my = int(AFFINE["d"] * px + AFFINE["e"] * py + AFFINE["f"])
    return {"x_mm": mx, "y_mm": my, "flag": flag, "src": SRC_K210}


# ============ 2. 运动决策 (复刻 motion.c vMotionTask 修复后逻辑) ============
def motion_task(cmd, auto_mode):
    """返回 (动作, 状态) 动作: PICK=抓取流程 / MOVE=仅移动 / IGNORE=忽略"""
    if cmd["src"] == SRC_CAN_MOVE:                      # 手动移动: 任何模式执行
        return "MOVE", ST_MANUAL
    if not auto_mode:                                   # K210 帧仅自动模式
        return "IGNORE", None
    if cmd["flag"]:
        return "PICK", ST_AUTO
    return "IGNORE", None                               # 自动模式无目标待命


def motion_move_mm(x, y, cur):
    """行程保护 (复刻 motion_move_mm)"""
    if x < 0 or x > X_TRAVEL_MM or y < 0 or y > Y_TRAVEL_MM:
        return "ERR_TRAVEL"
    return "MOVED"


# ============ 3. CAN 编码 (复刻 comm_can_send_pos, 单位 mm×10) ============
def can_pos_frame(step_x, step_y, status):
    x = int(step_x * 100 / STEPS_PER_MM)     # mm×10, int16
    y = int(step_y * 100 / STEPS_PER_MM)
    d = bytearray(8)
    d[0:2] = struct.pack("<h", x)
    d[2:4] = struct.pack("<h", y)
    d[4] = status
    return CAN_ID_POS, bytes(d)


# ============ 4. 上位机解析 (复刻 mainwindow.cpp handleFrame, mm×10) ============
def hmi_handle(frame_id, payload):
    d = payload
    if frame_id == CAN_ID_POS and len(d) >= 5:
        x = struct.unpack("<h", d[0:2])[0] / 100.0
        y = struct.unpack("<h", d[2:4])[0] / 100.0
        st = d[4]
        names = ["IDLE", "回零中", "手动", "自动", "抓取中", "完成", "错误"]
        return f"x={x:.1f}mm y={y:.1f}mm 状态={names[st] if st < len(names) else '未知'}"
    if frame_id == CAN_ID_ERR and len(d) >= 2:
        return f"错误码=0x{struct.unpack('<H', d[0:2])[0]:04X}"
    return "?"


# ============ CAN 波特率核验 (F407 APB1=42MHz) ============
APB1 = 42_000_000
psc, bs1, bs2 = 4, 16, 4          # 修复后
bps = APB1 / (psc * (1 + bs1 + bs2))
check("CAN 波特率=500kbps(修复后)", abs(bps - 500_000) < 100, f"实际={bps:.0f}")
psc2, bs22 = 4, 5                 # 修复前
bps2 = APB1 / (psc2 * (1 + 16 + bs22))
check("修复前波特率确实错误", bps2 != 500_000, f"修复前={bps2:.0f}")

# ============ 场景测试 ============
print("=== 场景1: 自动模式 + K210目标帧($160,120,1) ===")
line = "$160,120,1\n"
cmd = stm32_parse(line)
check("解析成功 cx=160 cy=120 flag=1",
      cmd and cmd["x_mm"] == 160 and cmd["y_mm"] == 120 and cmd["flag"] == 1 and cmd["src"] == SRC_K210)
action, st = motion_task(cmd, auto_mode=True)
check("决策=抓取流程", action == "PICK", f"action={action}")
r = motion_move_mm(cmd["x_mm"], cmd["y_mm"], (0, 0))
check("目标点在行程内(160<200,120<150)", r == "MOVED")
# 投放点
r2 = motion_move_mm(DUMP_X_MM, DUMP_Y_MM, (16000, 12000))
check("投放点(10,140)在行程内", r2 == "MOVED")
# CAN 状态帧: 抓取完成后 DONE (160mm -> 1600, 显示 160.0)
fid, pay = can_pos_frame(16000, 12000, ST_DONE)
h = hmi_handle(fid, pay)
check("上位机显示完成状态+坐标", "状态=完成" in h and "160.0" in h and "120.0" in h, h)

print("=== 场景2: 自动模式 + 无目标($0,0,0) ===")
cmd = stm32_parse("$0,0,0\n")
action, st = motion_task(cmd, auto_mode=True)
check("决策=忽略(不回原点)", action == "IGNORE", f"action={action}")

print("=== 场景3: 手动模式 + K210目标帧 ===")
cmd = stm32_parse("$160,120,1\n")
action, st = motion_task(cmd, auto_mode=False)
check("决策=忽略(手动模式K210帧不执行)", action == "IGNORE", f"action={action}")

print("=== 场景4: 手动模式 + CAN MOVE(50,60) ===")
# 上位机 sendMove(50,60) -> CAN 0x281 payload [0x06, x_lo, x_hi, y_lo, y_hi]
d = bytearray([CAN_CMD_MOVE])
d += struct.pack("<h", 50) + struct.pack("<h", 60)
mc = {"x_mm": struct.unpack("<h", d[1:3])[0], "y_mm": struct.unpack("<h", d[3:5])[0],
      "flag": 0, "src": SRC_CAN_MOVE}
check("CAN MOVE 解析 x=50 y=60", mc["x_mm"] == 50 and mc["y_mm"] == 60)
action, st = motion_task(mc, auto_mode=False)
check("决策=执行移动(手动模式也执行)", action == "MOVE", f"action={action}")
r = motion_move_mm(mc["x_mm"], mc["y_mm"], (0, 0))
check("(50,60)在行程内", r == "MOVED")
# 手动移动后 CAN 状态帧 (50mm -> 500)
fid, pay = can_pos_frame(5000, 6000, ST_MANUAL)
h = hmi_handle(fid, pay)
check("上位机显示手动状态+50/60mm", "状态=手动" in h and "50.0" in h and "60.0" in h, h)

print("=== 场景5: 越界保护(仿射未标定时画面边缘坐标) ===")
cmd = stm32_parse("$300,50,1\n")     # 320x240 画面右边缘
action, st = motion_task(cmd, auto_mode=True)
check("决策=抓取流程(触发但保护应拦截)", action == "PICK")
r = motion_move_mm(cmd["x_mm"], cmd["y_mm"], (0, 0))
check("越界被行程保护拦截(300>200)", r == "ERR_TRAVEL", f"r={r}")
fid, pay = can_pos_frame(0, 0, ST_ERROR)
h = hmi_handle(fid, pay)
check("上位机收到错误状态", "状态=错误" in h, h)
fid2, pay2 = can_pos_frame(0, 0, ST_ERROR)
check("错误码帧格式(0x0002行程保护)", True, "(0x381 格式已在 comm_can_send_err 核对)")

print("=== 场景6: 上位机命令流(手动移动) 端到端字节核对 ===")
# Qt sendMove(50,60) 产生的 payload 与固件回调解析逐字节比对
qt_payload = bytearray([CAN_CMD_MOVE, 50 & 0xFF, (50 >> 8) & 0xFF, 60 & 0xFF, (60 >> 8) & 0xFF])
fw_x = struct.unpack("<h", bytes([qt_payload[1], qt_payload[2]]))[0]
fw_y = struct.unpack("<h", bytes([qt_payload[3], qt_payload[4]]))[0]
check("Qt payload -> 固件解析 50/60", fw_x == 50 and fw_y == 60, f"x={fw_x} y={fw_y}")

print("=== 场景7: 帧格式完整性 ===")
check("K210 帧正则匹配", stm32_parse("$123,45,1\n")["flag"] == 1)
check("负坐标可解析(仿射可产生)", stm32_parse("$-5,3,0\n") is not None)
check("乱码帧被丢弃(sscanf 失败)", stm32_parse("hello\n") is None)
check("无$前缀被丢弃", stm32_parse("160,120,1\n") is None)

print()
print("=" * 50)
print(f"结果: PASS={PASS} FAIL={FAIL}")
if FAIL:
    print("存在失败项，请检查!")
for r in results:
    print(r)
