# 课记：音频·编码

短文（怎么跑、怎么验收）见 [encode.md](./encode.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改码率/路径/输入契约先改这里（并同步代码常量），短文与 teach 课只引用。

## 这一课在管线哪

Learning Pipeline Stages 的**编码**（音频）：**PCM 样本 → 压缩码流**。本课不采集、不传输/打包、不做 AEC、不做弱网自适应。

- **上游**：采集产物 `out/audio/capture/capture.wav`（或短文写明的等价 48 kHz mono s16le PCM WAV）
- **本段产出**：`out/audio/encode/encode.caf`（CAF 容器内为 Opus 码流）
- **下游**：音频「本地传输/打包」默认吃该 CAF；学习选型见 [local-transport.notes.md](./local-transport.notes.md) 与 ADR-0003；产品是否必须 RTP 仍未决

AEC 属叙述必会旁路，通常在采集后、编码前；弱网码率自适应依赖网络反馈作用于编码码率——本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/encode/main.cpp` | CLI、`--input`、严格读 WAV、调用写 CAF |
| `experiments/audio/encode/encode_caf.hpp` | 契约常量 + `EncodeStats` + `write_encode_caf` |
| `experiments/audio/encode/encode_caf.cpp` | AudioConverter + AudioFileWritePackets：PCM → Opus CAF |
| `experiments/audio/encode/CMakeLists.txt` | `learning_audio_encode_lib` + `learning_audio_encode` |
| `experiments/tests/test_encode_caf.cpp` | Catch2：合成 PCM 校验 Opus CAF |

可执行文件：`learning_audio_encode`（cwd＝仓库根）。

## 数据流

```
out/audio/capture/capture.wav（或 --input）
  → main 校验 RIFF/WAVE + 48 kHz / mono / s16le PCM
  → write_encode_caf(".", pcm, stats)
       AudioConverterNew(PCM → Opus) + EncodeBitRate 64 kbps
       AudioFileCreateWithURL(CAF)
       循环 AudioConverterFillComplexBuffer → AudioFileWritePackets
       AudioFileOptimize
  → 覆盖写出 out/audio/encode/encode.caf
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 输入 PCM | 48 000 Hz / mono / s16le（与采集课记一致） |
| 默认输入路径 | `out/audio/capture/capture.wav` |
| 编解码器 | Opus（`kAudioFormatOpus`） |
| 目标码率 | 64 000 bps（固定；无 CLI） |
| 分包 | 跟系统 `AudioConverter` 默认；本机实测 `frames_per_packet=120`（2.5 ms @ 48 kHz；以运行日志为准） |
| 产物路径 | `out/audio/encode/encode.caf` |
| 容器 | CAF（可 `afplay`） |

非契约 WAV（错采样率/声道/位深/非 PCM）直接失败，不做重采样。

Catch2 用合成 1 s 音调 PCM 断言：文件非空、`mFormatID == 'opus'`、48 kHz mono，且 `output_bytes` 低于码率包络上界（防 CAF `free` 膨胀回归）；不读真人 `capture.wav`。

## 关键 API / 决策

- **AudioConverter** + **AudioFileWritePackets**：拉包写入 CAF；创建时带 `kAudioFileFlags_DontPageAlignAudioData`，并写入 magic cookie。
- **码率**：`AudioConverterSetProperty(..., kAudioConverterEncodeBitRate, 64000)`。
- **决策**：系统 Opus，不引入 libopus → `docs/adr/0002-learning-audio-encode-opus-audioconverter.md`。
- **可测缝**：仅 `write_encode_caf`；听感靠短文人工 `afplay`。

## 踩坑与边界

- 沙箱内 `AudioConverter` / Opus 可能不可用（`fmt?`）；护栏与烟测须在非沙箱环境跑。
- 旧 `encode.caf` ≠ 本次成功；失败时 exit ≠ 0。
- 本票不做 RTP、自定义分帧、`--output`、`--bitrate`、自适应码率。
- WAV 解析只认 PCM（format tag 1）；不支持浮点 WAV / 扩展 fmt 变体作为输入。
- 无 `DontPageAlignAudioData` 时 CAF 可能留下百 KB 级 `free` chunk；写完后仍 `AudioFileOptimize`。`output_bytes` 为文件大小（含 CAF 头）。

## 下一课钩子

**04 音频·本地传输/打包**默认吃 `out/audio/encode/encode.caf`（或短文写明的等价码流单元）。学习选型见 ADR-0003（长度前缀 Transport Unit + UDP loopback）；产品「是否必须 RTP」仍为 spec 未决 #5 / U10。
