# 音频·本地传输/打包

## 目标

Learning Pipeline Stages 的**本地传输/打包**阶段（音频）：从上游 Opus-in-CAF 读出 access unit，打成 **Transport Unit**，经本机 UDP loopback 自发自收并校验一致性。格式契约以课记为准。

本票单元格式是学习选型（长度前缀），**不等于**对产品「必须 RTP」的拍板。jitter buffer 通常在接收后、解码前吸收网络抖动——**本阶段不实现、不验收**（一两句位置预告，见课记）。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入：`out/audio/encode/encode.caf`（先跑 `learning_audio_encode`，或用 `--input` 指向同契约 Opus CAF）
- 无需麦克风权限；不做解码、不写 PCM

## 选用库

- 读包：macOS **AudioToolbox**（`AudioFileReadPacketData`）
- 环回：**POSIX UDP**（`127.0.0.1`，bind 临时端口后自发自收）
- 无第三方媒体/RTP 库

决策见 `docs/adr/0003-learning-audio-local-transport-length-prefix-udp.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_audio_local_transport
# 若尚无编码产物：
./experiments/build/audio/capture/learning_audio_capture --seconds 1
./experiments/build/audio/encode/learning_audio_encode
./experiments/build/audio/local-transport/learning_audio_local_transport
# 可选：./experiments/build/audio/local-transport/learning_audio_local_transport --input path/to/opus.caf
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被覆盖）`out/audio/local-transport/loopback-ok.txt`（字段格式见课记）
- 终端日志含 `units_sent` / `units_recv` / `payload_bytes_sent` / `payload_bytes_recv` / `port`；单元数相等且逐包载荷一致

## 失败时仍记录什么

- 缺输入 / 非 Opus CAF / `AudioFile*` 失败时的 stderr 原文
- 套接字 `bind` / `sendto` / `recvfrom` 失败或收发不一致时的 stderr 原文
- unknown argument / `--input` 缺值时的 stderr 原文
- 若退出非 0 但仍有旧 `loopback-ok.txt`，注明该文件可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_audio_local_transport`；人工按本文验收（产物文件 + 日志）。可无硬件契约见 `experiments/tests/test_local_transport.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[local-transport.notes.md](./local-transport.notes.md)
- **上游编码短文**：[encode.md](./encode.md)
