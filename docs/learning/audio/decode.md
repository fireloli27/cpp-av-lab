# 音频·解码

## 目标

Learning Pipeline Stages 的**解码**阶段（音频）：把 Opus CAF 里的 access unit 还原成与采集契约相同的 **PCM 样本**，写出可播放 WAV，验证本段可独立跑通。格式契约以课记为准，本文不展开。

所读的包字节就是 04 放进 **Transport Unit** 的载荷；环回一致性已由本地传输/打包证明，本阶段不重跑 UDP。jitter buffer 通常在接收后、解码前吸收网络抖动——**本阶段不实现、不验收**。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入：`out/audio/encode/encode.caf`（先跑 `learning_audio_encode`，或用 `--input` 指向同契约 Opus CAF：48 kHz / mono）
- 无需麦克风权限；不播放（听感用 `afplay` 人工确认）
- `AudioConverter` 须在非沙箱环境跑（见 `docs/learning/harness/README.md`）

## 选用库

macOS **AudioToolbox**：`AudioFileReadPacketData` 读出 Opus access unit（含 magic cookie），`AudioConverter` / `AudioConverterFillComplexBuffer` 按包解码为 PCM。自己写 WAV。不用 `ExtAudioFile`，无第三方媒体库。决策见 `docs/adr/0004-learning-audio-decode-audioconverter.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_audio_decode
# 若尚无编码产物：
./experiments/build/audio/capture/learning_audio_capture --seconds 1
./experiments/build/audio/encode/learning_audio_encode
./experiments/build/audio/decode/learning_audio_decode
# 可选：./experiments/build/audio/decode/learning_audio_decode --input path/to/opus.caf
afplay out/audio/decode/decode.wav
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被覆盖）`out/audio/decode/decode.wav`
- 终端日志含 `packets` 与 `pcm_frames`（以及 `frames_per_packet`）；`pcm_frames > 0`，且落在课记的时长窗口内
- `afplay out/audio/decode/decode.wav` 能听出与上游 CAF 对应的内容（人工项，不进 Catch2）

## 失败时仍记录什么

- 缺输入 / 非 Opus CAF / 非 48 kHz mono / 无 magic cookie 时的 stderr 原文
- `AudioFileReadPacketData` / `AudioConverterNew` / `AudioConverterFillComplexBuffer` 等 OSStatus
- 时长窗口未通过时 stderr 里的 `pcm_frames` 与 `source_frames`
- unknown argument / `--input` 缺值时的 stderr 原文
- 若退出非 0 但仍有旧 `decode.wav`，注明该文件可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_audio_decode`；人工按本文验收（听 WAV）。可无硬件契约见 `experiments/tests/test_decode_wav.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[decode.notes.md](./decode.notes.md)
- **上游编码短文**：[encode.md](./encode.md)
- **上游本地传输/打包短文**：[local-transport.md](./local-transport.md)
