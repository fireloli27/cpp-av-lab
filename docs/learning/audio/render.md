# 音频·渲染

## 目标

Learning Pipeline Stages 的**渲染**阶段（音频）：把 PCM WAV 里的 **PCM 样本**按输出设备时钟交给扬声器，由 AudioQueue 回调拉取，验证本段可独立跑通且能听见。格式与缓冲契约以课记为准，本文不展开。

旁路位置预告（**本阶段不实现、不验收**，展开见课记）：

- jitter buffer 在接收后、解码前吸收网络到达抖动；本实验的 AudioQueue buffer 是渲染端输出缓冲，二者不是一回事
- A/V 同步落在渲染这一带，通常以音频播放时钟为主钟
- AEC 的参考信号取自渲染路径（即将播出的远端 PCM）

五段如何串成一条音频路径证据，见 [pipeline.md](./pipeline.md)。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 系统有可用的默认输出设备，音量不为静音
- 默认输入：`out/audio/decode/decode.wav`（先跑 `learning_audio_decode`，或用 `--input` 指向同契约 PCM WAV：48 kHz / mono / s16le，例如 `out/audio/capture/capture.wav`）
- Agent 烟测（真播放）须在非沙箱环境执行（见 `docs/learning/harness/README.md`）；`./scripts/check-learning.sh` 不打开输出设备

## 选用库

macOS **AudioToolbox**：`AudioFileOpenURL` + `AudioFileReadPacketData` 读 WAV；**AudioQueue output**（`AudioQueueNewOutput` / `AudioQueueEnqueueBuffer` / `AudioQueueStart` / `AudioQueueStop`）播放。不调用 `afplay`，无第三方媒体库。决策见 `docs/adr/0005-learning-audio-render-audioqueue.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_audio_render
# 若尚无解码产物：见 decode.md 或 pipeline.md
./experiments/build/audio/render/learning_audio_render
# 可选：./experiments/build/audio/render/learning_audio_render --input out/audio/capture/capture.wav
```

## 可观察成功判据

- 命令播放完整段后退出，退出码为 0
- 播放期间扬声器能听出与输入 WAV 对应的内容（人工项，不进 Catch2）
- 终端日志含 `pcm_frames` 与 `frames_rendered`，且二者相等（以及 `buffers`、`buffer_frames`）
- `frames_rendered` 只证明这些帧**交给了设备**；是否真的从喇叭出来，只能靠人耳
- 不写 `out/` 产物

## 失败时仍记录什么

- 缺输入 / 非 48 kHz mono s16le PCM WAV / 0 帧时的 stderr 原文
- `AudioQueueNewOutput` / `AudioQueueAllocateBuffer` / `AudioQueueEnqueueBuffer` / `AudioQueueStart` 的 OSStatus，以及当时的默认输出设备
- 「playback did not drain before deadline」或 `frames_rendered != pcm_frames` 的 stderr 原文
- 退出码 0 但没听见声音：记下输出设备、音量、是否连了蓝牙耳机等
- unknown argument / `--input` 缺值时的 stderr 原文
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_audio_render`；人工按本文验收（听扬声器）。可无硬件契约见 `experiments/tests/test_render_pcm.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[render.notes.md](./render.notes.md)
- **音频五段贯通说明**：[pipeline.md](./pipeline.md)
- **上游解码短文**：[decode.md](./decode.md)
