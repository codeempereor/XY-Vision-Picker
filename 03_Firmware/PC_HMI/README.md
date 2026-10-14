# PC_HMI 上位机

Qt 上位机工程（运动控制面板）。

## 工程

`mypro00001/`：Qt Widgets 工程（qmake，MinGW 工具链）

- `mypro00001/mypro00001.pro`：工程文件
- 源码：`main.cpp`、`mainwindow.cpp/h`、`dotmatrixview.cpp/h`、`protocol.h`
- 对外接口：CAN 500kbps，标准帧 ID 0x181（状态上报）/ 0x281（命令下发）/ 0x381（错误码），帧格式见根 README「通信协议」

## 目录

- `src/`：源码归档区（工程实际源码在 mypro00001/，见该目录 README）
- `qcustomplot/`：QCustomPlot 绘图库（预留，当前界面用自定义点阵视图 dotmatrixview）

## 打开方式

Qt Creator 打开 `mypro00001/mypro00001.pro`，选择 MinGW 套件构建运行。
