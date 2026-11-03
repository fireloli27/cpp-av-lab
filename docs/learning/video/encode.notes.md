# 课记：视频·编码

短文（怎么跑、怎么验收）见 [encode.md](./encode.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改码率/路径/容器先改这里（并同步代码常量），短文只引用。

## 这一课在管线哪

Learning Pipeline Stages 的**编码**（视频）：**Raw Video Frame → 压缩码流**。本课不采集、不传输/打包、不做弱网码率自适应。

- **上游**：采集目录 `out/video/capture/`（或 `--input` 指向的同契约目录）：tight NV12 `420v` + PTS manifest
- **本段产出**：`out/video/encode/encode.vau`（H.264 access unit）与 `out/video/encode/manifest.csv`
- **下游**：视频「本地传输/打包」默认吃该 VAU 里的 access unit **载荷**（参数集留在文件头，不单独当传输单元）。学习分帧见该阶段课记；产品是否必须 RTP 仍未决

弱网码率自适应依赖网络反馈去改编码码率——本阶段码率固定，实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/video/encode/main.cpp` | CLI、`--input`、读采集目录、打印成功 token |
| `experiments/video/encode/load_capture.hpp` | 采集目录契约常量 + `load_capture_directory` |
| `experiments/video/encode/load_capture.cpp` | 解析 manifest，读 tight `.nv12` |
| `experiments/video/encode/vau.hpp` | VAU 结构与读写声明 |
| `experiments/video/encode/vau.cpp` | 大端 VAU 读写（无 VideoToolbox） |
| `experiments/video/encode/encode_vt.hpp` | 码率/路径常量 + `encode_nv12_to_vau` |
| `experiments/video/encode/encode_vt.cpp` | `VTCompressionSession`：NV12 → H.264，staging 发布 |
| `experiments/video/encode/CMakeLists.txt` | `learning_video_encode_lib` + `learning_video_encode` |
| `experiments/tests/test_video_encode.cpp` | Catch2：VAU 往返、采集目录拒绝、合成帧编码 |

可执行文件：`learning_video_encode`（cwd＝仓库根）。

## 数据流

```
out/video/capture/（或 --input）
  → load_capture_directory：manifest + tight NV12
  → encode_nv12_to_vau
       VTCompressionSessionCreate(H.264, 软件编码器)
       每帧 CVPixelBuffer(420v) → VTCompressionSessionEncodeFrame
       回调拷出 AVCC 载荷 + SPS/PPS；PTS 用输入的 pts_ns
       write_vau → staging → 替换 out/video/encode/
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 输入 | 1280×720 tight NV12 `420v`，每帧 **1,382,400** 字节；`pts_ns` ≥ 0 且严格递增；帧数 **[1, 90]** |
| 默认输入 | `out/video/capture/` |
| 编解码器 | H.264，`kCMVideoCodecType_H264`，`kVTProfileLevel_H264_Baseline_AutoLevel` |
| 重排 | `kVTCompressionPropertyKey_AllowFrameReordering = false`（无 B 帧） |
| 编码器 | `kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder = false` |
| 目标码率 | 2,000,000 bps（固定；无 CLI） |
| GOP | `kVTCompressionPropertyKey_MaxKeyFrameInterval = 30`；第一帧强制 keyframe |
| 帧率属性 | `kVTCompressionPropertyKey_ExpectedFrameRate = 30`（给码控；文件里的时间是采集 PTS） |
| 一对一 | 一个 Raw Video Frame → 一个 access unit；丢帧即失败 |
| 产物 | `out/video/encode/encode.vau`、`out/video/encode/manifest.csv` |
| manifest 表头 | `au_index,pts_ns,is_keyframe,bytes` |
| 发布 | 先写 `out/video/encode.staging/`，成功后替换 `out/video/encode/`；失败留下旧目录 |

stdout 成功行的 `codec` 值为 `H264`。文件内 fourcc 是 `avc1`（`0x61766331`）。`frames_encoded == access_units`。`output_bytes` 是 `encode.vau` 的文件大小，不含 manifest。

### VAU 布局（大端）

| 偏移 | 字段 |
| --- | --- |
| 0 | magic `LVAU` |
| 4 | `uint16` version = 1 |
| 6 | `uint16` reserved = 0 |
| 8 | `uint32` codec = `avc1` |
| 12 | `uint32` width = 1280 |
| 16 | `uint32` height = 720 |
| 20 | `uint32` bitrate_bps |
| 24 | `uint32` access_unit_count（1…90） |
| 28 | `uint32` parameter_set_count（1…8） |
| 随后 | 每个参数集：`uint16` nal_bytes（1…2048）+ 原始 NAL（无 start code） |
| 随后 | 每个 access unit：`int64` pts_ns、`uint32` flags（bit0 = keyframe，其余必须为 0）、`uint32` payload_bytes、AVCC 载荷 |

AVCC 载荷是重复的「4 字节大端长度 + NAL」。NAL length size 必须是 4。参数集里至少要有 SPS（NAL type 7）和 PPS（NAL type 8）。第一个 access unit 必须是 keyframe。`pts_ns` 严格递增。尾部多余字节视为损坏。

Catch2 用合成 VAU 断言往返，并用两帧纯色 NV12 走 `VTCompressionSession`；不读真人采集目录、不打开摄像头。

## 关键 API / 决策

- **`VTCompressionSessionCreate`** + **`VTCompressionSessionEncodeFrame`** + **`VTCompressionSessionCompleteFrames`**：回调里用 `CMBlockBufferCopyDataBytes` 拷出 AVCC，用 `CMVideoFormatDescriptionGetH264ParameterSetAtIndex` 拷出 SPS/PPS。
- **PTS**：交给编码器的 `CMTime` 是帧序号 / 30，避免采集 host time 的纳秒刻度把 session 撑爆；写入 VAU 的 `pts_ns` 仍是输入 Raw Video Frame 的 PTS。
- **决策**：系统 H.264，不引入 FFmpeg → `docs/adr/0007-learning-video-encode-videotoolbox-h264.md`。
- **可测缝**：`write_vau` / `read_vau`、`load_capture_directory`、`encode_nv12_to_vau`。

## 踩坑与边界

- 沙箱里 VideoToolbox 可能直接失败；护栏在 GitHub Actions `macos-latest` 上跑，本机烟测须非沙箱。
- 不要求硬件编码器。Baseline + 软件路径是为了 CI 与「无 B 帧」同时成立。
- 旧 `out/video/encode/` ≠ 本次成功；失败 exit ≠ 0，且不发布半成品 staging。
- 本票不做 RTP、不做 `--bitrate` / `--output`、不做码率自适应、不把 VAU 当成产品容器。
- 输入不是 720p `420v` 就失败，不缩放、不改像素格式。
- 90 帧未压缩约 124 MB，编码前会整段留在内存里；这与采集票的上限一致。

## 下一课钩子

**09 视频·本地传输/打包**默认读 `out/video/encode/encode.vau`，把每个 access unit 的 AVCC 载荷组织成可环回的 Transport Unit。参数集留在文件头，供解码重建 format description；解码不从 UDP 再读一遍（与音频路径一样，等价性靠环回逐字节比对）。
