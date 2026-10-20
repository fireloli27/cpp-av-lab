# 课记：音频·采集

短文（怎么跑、怎么验收）见 [capture.md](./capture.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改采样率/声道/位深先改这里（并同步代码常量），短文与 teach 课只引用。

PCM / WAV 概念、字节账练习、G.711 辨析 → [课 0001](../../../teach/lessons/0001-pcm-samples-wav-box.html) · [速查](../../../teach/reference/pcm-wav-cheatsheet.html)。

## 这一课在管线哪

Learning Pipeline Stages 的**采集**（音频）：麦克风 → **原始 PCM 样本**。本课不编码、不传输、不做 AEC。

- **上游**：无（硬件默认输入设备）
- **本段产出**：`out/audio/capture/capture.wav`（WAV 容器内为下述 PCM）
- **下游**：音频「编码」默认以该 WAV 为可追溯输入（见 issue 03）

AEC 属叙述必会旁路，通常在采集后、编码前的发送侧处理带；本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/capture/main.cpp` | CLI、`AudioQueue` 录音、把 PCM 交给写盘 |
| `experiments/audio/capture/capture_wav.hpp` | 契约常量 + `write_capture_wav` 声明 |
| `experiments/audio/capture/capture_wav.cpp` | 无硬件：PCM → RIFF/WAVE |
| `experiments/audio/capture/CMakeLists.txt` | `learning_audio_capture_lib` + `learning_audio_capture` |
| `experiments/tests/test_capture_wav.cpp` | Catch2：合成 PCM 校验 WAV 头 |

可执行文件：`learning_audio_capture`（cwd＝仓库根）。

## 数据流

```
默认麦克风
  → AudioQueue 输入回调（后台线程）拼出 vector<int16_t>
  → 主线程 sleep(--seconds) 后 Stop/Dispose
  → write_capture_wav(".", pcm) 覆盖写出 out/audio/capture/capture.wav
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 采样率 | 48 000 Hz |
| 声道 | mono（1） |
| 样本 | s16le（signed 16-bit little-endian） |
| 产物路径 | `out/audio/capture/capture.wav` |
| 裸 PCM 字节率 | 48 000 × 1 × 2 = **96 000 B/s** |

打开队列时用 ASBD 钉死上表（含 `kAudioFormatFlagsNativeEndian`）；`GetProperty(StreamDescription)` 复核 rate/ch/bits。设备→契约若转换，留在 AudioQueue；本课无独立重采样模块。

WAV：`fmt` 描述格式，`data` 装 PCM。Catch2 用字面量断言头字段（与上表一致），不测麦克风。

## 关键 API / 决策

- **AudioQueue**：`NewInput`（run loop `nullptr`）→ Allocate/Enqueue → Start → Stop/Dispose。
- **决策**：系统 Core Audio，不引入 PortAudio → `docs/adr/0001-learning-audio-capture-coreaudio.md`。
- **可测缝**：仅 `write_capture_wav`；真麦靠短文人工听。

## 踩坑与边界

- 勿链 `kCFRunLoopCommonModes`（需 CoreFoundation）；本实现用 `nullptr` run loop。
- Agent 沙箱内采麦常失败；须非沙箱 + 终端麦克风权限（见短文前置）。
- 失败非 0；旧 `capture.wav` ≠ 本次成功。
- `--seconds` ∈ `(0, 60]`；无设备列表、无 .app。

## 下一课钩子

**03 音频·编码**默认读 `out/audio/capture/capture.wav`（或短文写明的等价输入）。编码库未决。
