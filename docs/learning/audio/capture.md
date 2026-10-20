# 音频·采集

## 目标

Learning Pipeline Stages 的**采集**阶段（音频）：从默认麦克风取到原始 PCM，写出可播放 WAV，验证本段可独立跑通。格式契约以课记为准，本文不展开。

AEC 属叙述必会旁路；**本阶段实验不实现、不验收**（位置说明见课记）。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- **麦克风权限**：系统设置 → 隐私与安全性 → 麦克风 → 允许你使用的终端（Terminal / iTerm 等）
- 系统有可用的默认输入设备
- Agent 烟测（真采麦）须在非沙箱环境执行（`required_permissions: ["all"]`）；`./scripts/check-learning.sh` 不测麦克风

## 选用库

macOS **Core Audio / AudioQueue**（AudioToolbox）。无第三方媒体库。决策见 `docs/adr/0001-learning-audio-capture-coreaudio.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_audio_capture
./experiments/build/audio/capture/learning_audio_capture
# 可选：./experiments/build/audio/capture/learning_audio_capture --seconds 5
```

`--seconds` 取值须在 **(0, 60]**；默认 **3** 秒。

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被覆盖）`out/audio/capture/capture.wav`
- 用系统播放器打开该文件，能听到刚录制的内容
- 终端日志含采样率、声道、位深与大致时长（须与课记契约一致）

## 失败时仍记录什么

- `AudioQueueNewInput` / `AudioQueueAllocateBuffer` / `AudioQueueEnqueueBuffer` / `AudioQueueStart` / `AudioQueueGetProperty` 的 OSStatus 与是否已给终端麦克风权限
- 若报 ASBD 未兑现，记录实际 rate/ch/bits
- 「no PCM captured」或写文件失败时的 stderr 原文
- `--seconds` 非法或 unknown argument 时的 stderr 原文
- 若退出非 0 但仍有旧 `capture.wav`，注明该文件可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_audio_capture`；人工按本文验收（听 WAV）。可无硬件契约见 `experiments/tests/test_capture_wav.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[capture.notes.md](./capture.notes.md)
- **概念课（PCM≠WAV）**：[lessons/0001-pcm-samples-wav-box.html](../../../lessons/0001-pcm-samples-wav-box.html)（`/teach` 工作区，非本缝硬交付）
