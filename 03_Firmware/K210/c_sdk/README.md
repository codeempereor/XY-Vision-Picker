# K210 C/C++ SDK 工程（预留）

当前 K210 视觉用 MaixPy（见 `../py/`）。本目录预留，后续需要更高性能时迁移到 Kendryte SDK C 开发。

## 适用场景
- YOLO kmodel 推理（MaixPy 调 KPU 不方便）
- 帧率要求 > 10 fps
- 需要直接操作底层寄存器

## 不变的接口
对外 UART 协议保持 `$cx,cy,flag\n` 格式，STM32 侧零修改。

## 工具链
- Kendryte SDK（kendryte-standalone-sdk）或 MaixPy C 扩展
- 编译：cmake + riscv64-unknown-elf-gcc
- 烧录：kflash
