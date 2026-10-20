# 课记：音频·采集

短文（怎么跑、怎么验收）见 [capture.md](./capture.md)。

## 这一课在管线哪

Learning Pipeline Stages 的**采集**（音频）：麦克风 → **原始 PCM 样本**。本课不编码、不传输、不做 AEC。

- **上游**：无（硬件默认输入设备）
- **本段产出**：`out/audio/capture/capture.wav`（内含 48 kHz / mono / s16le PCM）
- **下游**：音频「编码」默认以该 WAV 为可追溯输入（见 issue 03）

AEC 属叙述必会旁路，通常在采集后、编码前的发送侧处理带；本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/capture/main.cpp` | CLI、`AudioQueue` 录音、把 PCM 交给写盘 |
| `experiments/audio/capture/capture_wav.hpp` | PCM 契约常量 + `write_capture_wav` 声明 |
| `experiments/audio/capture/capture_wav.cpp` | 无硬件：PCM → RIFF/WAVE 文件 |
| `experiments/audio/capture/CMakeLists.txt` | `learning_audio_capture_lib` + `learning_audio_capture`（链 AudioToolbox / CoreAudio） |
| `experiments/tests/test_capture_wav.cpp` | Catch2：合成 PCM 校验 WAV 头（不碰麦克风） |

可执行文件：`learning_audio_capture`（cwd＝仓库根）。

## 数据流

```
默认麦克风
  → AudioQueue 输入回调（后台线程）拼出 vector<int16_t>
  → 主线程 sleep(--seconds) 后 Stop/Dispose
  → write_capture_wav(".", pcm) 覆盖写出 out/audio/capture/capture.wav
```

**契约**：48 kHz、1 声道、16-bit little-endian PCM。打开队列时用 ASBD 钉死该格式（含 native endian＝本机 LE）；`GetProperty(StreamDescription)` 复核 rate/ch/bits。队列内若发生设备→契约格式的转换，留在 AudioQueue，本课不自写重采样器。

WAV 只是可播放容器：`fmt` + `data`；自动化测试用字面量断言 48000/mono/16，不依赖头文件常量（避免与契约脱钩）。

## 关键 API / 决策

- **AudioQueue**（AudioToolbox）：`AudioQueueNewInput`（run loop 传 `nullptr`，由系统线程调回调）→ Allocate/Enqueue buffer → Start → Stop/Dispose。
- **决策**：系统 Core Audio，不引入 PortAudio。见 `docs/adr/0001-learning-audio-capture-coreaudio.md`。
- **可测缝**：只有 `write_capture_wav`；真麦路径靠短文人工听。

## 踩坑与边界

- 链接：`kCFRunLoopCommonModes` 需要 CoreFoundation；本实现改用 `nullptr` run loop，只链 AudioToolbox + CoreAudio。
- 沙箱：agent 默认沙箱里 `AudioQueueStart` 常失败（如 `-66680`）；真采麦须非沙箱（`required_permissions: ["all"]`）且终端已获麦克风权限。
- 失败须非 0 退出；旧 `capture.wav` 不能当作本次成功证据。
- `--seconds` 仅允许 `(0, 60]`；不做设备列表、不做 .app。

## 下一课钩子

**03 音频·编码**应默认读 `out/audio/capture/capture.wav`（或短文写明的等价 PCM/WAV）。编码库仍未决；本课不预设码流格式。
