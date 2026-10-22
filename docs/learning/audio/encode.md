# 音频·编码

## 目标

Learning Pipeline Stages 的**编码**阶段（音频）：把采集契约 PCM 压成 **Opus** 压缩码流，写出可播放 CAF，验证本段可独立跑通。格式契约以课记为准，本文不展开。

AEC 通常在采集后、编码前的发送侧处理带；弱网码率自适应会作用在编码码率——**本阶段实验均不实现、不验收**（位置说明见课记）。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入：`out/audio/capture/capture.wav`（先跑 `learning_audio_capture`，或用 `--input` 指向同契约 WAV）
- 无需麦克风权限（本可执行文件不采麦）

## 选用库

macOS **AudioToolbox / `AudioConverter`**（`AudioFileWritePackets` 写入 CAF），编解码器 **Opus**。无第三方媒体库。决策见 `docs/adr/0002-learning-audio-encode-opus-audioconverter.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_audio_encode
# 若尚无采集产物：
./experiments/build/audio/capture/learning_audio_capture --seconds 1
./experiments/build/audio/encode/learning_audio_encode
# 可选：./experiments/build/audio/encode/learning_audio_encode --input path/to/contract.wav
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被覆盖）`out/audio/encode/encode.caf`
- `afplay out/audio/encode/encode.caf`（或系统播放器）能听出与输入 WAV 对应的内容
- 终端日志含采样契约摘要、`bitrate_bps`、约包数、`frames_per_packet`、输出字节与时长（须与课记契约一致）

## 失败时仍记录什么

- 缺输入 / 非 RIFF WAVE / 非 48 kHz mono s16le PCM 时的 stderr 原文
- `AudioConverterNew` / `AudioConverterFillComplexBuffer` / `AudioFileWritePackets` 等 OSStatus
- unknown argument / `--input` 缺值时的 stderr 原文
- 若退出非 0 但仍有旧 `encode.caf`，注明该文件可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_audio_encode`；人工按本文验收（听 CAF）。可无硬件契约见 `experiments/tests/test_encode_caf.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[encode.notes.md](./encode.notes.md)
- **上游采集短文**：[capture.md](./capture.md)
