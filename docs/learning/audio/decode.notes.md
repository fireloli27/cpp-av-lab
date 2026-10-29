# 课记：音频·解码

短文（怎么跑、怎么验收）见 [decode.md](./decode.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改路径/时长窗口/输入契约先改这里（并同步代码常量），短文与 teach 课只引用。

## 这一课在管线哪

Learning Pipeline Stages 的**解码**（音频）：**压缩码流 → PCM 样本**。输入是 Opus access unit 本身，不是 **Transport Unit** 的长度前缀外壳。本课不重跑 UDP、不修 CAF priming、不做 jitter buffer、不模拟丢包、不做 PLC。

- **上游**：编码产物 `out/audio/encode/encode.caf`（或短文写明的等价 48 kHz mono Opus CAF）。这些包字节就是 04 放进 Transport Unit 的载荷；环回一致性已由 [local-transport.notes.md](./local-transport.notes.md) 证明
- **本段产出**：`out/audio/decode/decode.wav`（48 kHz / mono / s16le PCM WAV）
- **下游**：音频「渲染」默认可吃该 WAV；播放 API 本票不定

jitter buffer（叙述必会旁路）通常在**接收之后、本阶段之前**——本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/decode/main.cpp` | CLI、`--input`、调用解码并打成功日志 |
| `experiments/audio/decode/decode_wav.hpp` | 契约常量 + `DecodeStats` + `write_decode_wav` |
| `experiments/audio/decode/decode_wav.cpp` | `AudioFileReadPacketData` + 按包 `AudioConverter` → WAV |
| `experiments/audio/decode/CMakeLists.txt` | `learning_audio_decode_lib` + `learning_audio_decode` |
| `experiments/tests/test_decode_wav.cpp` | Catch2：合成 Opus CAF → WAV（格式、非全零、时长窗口） |

可执行文件：`learning_audio_decode`（cwd＝仓库根）。不链接采集库。

## 数据流

```
out/audio/encode/encode.caf（或 --input）
  → AudioFileOpenURL(CAF)
  → 校验 Opus + 48 kHz / mono + magic cookie
  → AudioFileReadPacketData 逐批读出 access unit
  → AudioConverterNew(Opus → PCM) + DecompressionMagicCookie
  → 输入回调每次喂 1 个 access unit
  → AudioConverterFillComplexBuffer 拉出 s16le
  → 时长窗口通过后覆盖写出 out/audio/decode/decode.wav
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 输入 | Opus CAF，48 000 Hz / mono；须有 magic cookie |
| 默认输入路径 | `out/audio/encode/encode.caf` |
| 输出 PCM | 48 000 Hz / mono / s16le（与采集课记一致） |
| 产物路径 | `out/audio/decode/decode.wav` |
| `source_frames` | `packet_count × frames_per_packet`（文件 ASBD，不修 priming） |
| 时长窗口 | `pcm_frames > 0` 且 `source_frames / 2 ≤ pcm_frames ≤ source_frames × 2` |
| 成功日志 | stdout 含 `packets`、`pcm_frames`（以及 `frames_per_packet`） |

非契约 CAF（非 Opus、采样率或声道不符、无 cookie、零包、转换失败、窗口外）直接失败，不做重采样、不修齐时长。不与 `capture.wav` 逐样本对比。

Catch2 用 `learning_audio_encode_lib` 从 1 秒合成音调写出 Opus CAF，再解码；断言 WAV 头、至少一个样本非 0、以及时长窗口。窗口的 `source_frames` 由测试自己用 AudioFile 读取包数与 `mFramesPerPacket`，不复算解码器内部公式。不读真人 `encode.caf`、不提交二进制 fixture。

## 关键 API / 决策

- **`AudioFileReadPacketData`**：从 CAF 拉出 Opus access unit；与 04 读的是同一类包字节。
- **`AudioConverterNew` + `AudioConverterSetProperty(kAudioConverterDecompressionMagicCookie)` + `AudioConverterFillComplexBuffer`**：按包 Opus → PCM。输入回调一次一个 access unit。
- **决策**：不用 `ExtAudioFile`、不引入 libopus → `docs/adr/0004-learning-audio-decode-audioconverter.md`。
- **可测缝**：仅 `write_decode_wav`；听感靠短文人工 `afplay`。

## 踩坑与边界

- 沙箱内 `AudioConverter` / Opus 可能不可用；护栏与烟测须在非沙箱环境跑。
- 不按 CAF packet table 的 priming 把 `pcm_frames` 修到与原始 PCM 等长。差一个编码延迟量级仍应落在 1/2～2 倍窗口内；窗口外 exit ≠ 0，且不写新 WAV。
- 转换器若在 access unit 尚未喂完时就不再吐 PCM，同样失败（窗口下沿是一半，不能靠窗口单独证明包已解完）。
- 旧 `decode.wav` ≠ 本次成功；失败时 exit ≠ 0。
- 本票不做 UDP、jitter buffer、丢包、PLC、`--output`。
- WAV 由本目录自己写 RIFF（不链接 `learning_audio_capture_lib`）。宿主是小端 macOS，内存中的 s16 与文件 s16le 一致。

## 下一课钩子

**06 音频·渲染**默认可吃 `out/audio/decode/decode.wav`（48 kHz mono s16le PCM WAV）。本票不定播放 API。
