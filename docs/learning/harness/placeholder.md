# Harness 占位实验

## 目标

验证 Learning Project 地基：CMake 可配置编译，且存在可从仓库外部运行的独立入口；成功时留下廉价可观察产物。这不是 Learning Pipeline Stages 中的采集/编码等任一段，仅用于证明主缝约定可操作。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 无需麦克风/摄像头权限；无第三方库

## 选用库

无。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build
./experiments/build/harness/placeholder/learning_placeholder
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现文件 `out/harness/placeholder-ok.txt`
- 文件内容包含一行：`learning_placeholder: ok`

## 失败时仍记录什么

CMake 配置/编译错误原文；若编译成功但未生成 `out/harness/placeholder-ok.txt`，记录实际 cwd、二进制路径与终端输出。

## 与主缝关系

本入口为 harness 占位的独立可执行文件 `learning_placeholder`；人工按本文步骤验收。可观察产物契约另由 `experiments/tests/test_placeholder_artifact.cpp`（Catch2）覆盖，一键入口见 `scripts/check-learning.sh`。后续各 Learning Pipeline Stage 实验沿用「各自入口 + 短文判据」；自动化测试只护可无硬件复现的契约。
