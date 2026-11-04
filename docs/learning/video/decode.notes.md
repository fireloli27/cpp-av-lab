# 课记：视频·解码

短文（怎么跑、怎么验收）见 [decode.md](./decode.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**。

## 这一课在管线哪

Learning Pipeline Stages 的**解码**（视频）：**压缩码流 → 可显示的 Raw Video Frame**。本课不跑环回、不渲染、不做 jitter buffer。

- **上游**：`out/video/encode/encode.vau`（SPS/PPS + 每个 access unit 的 AVCC 载荷与 `pts_ns`）。不是 UDP 分片
- **本段产出**：`out/video/decode/` 下 tight `.nv12`、PTS manifest、首帧 `preview.png`
- **下游**：视频「渲染」默认读这个目录。布局与采集目录相同，所以 `--input` 也可以指向 `out/video/capture/` 做短路预览，但贯通路径吃的是解码结果

有损编码：解码帧不要求与编码前逐字节相同。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/video/decode/main.cpp` | CLI、`--input`、打印成功 token |
| `experiments/video/decode/decode_vau.hpp` | 路径常量 + `decode_vau_to_nv12` |
| `experiments/video/decode/decode_vau.mm` | `VTDecompressionSession` 按 access unit 解码，staging 发布，Core Image PNG |
| `experiments/video/decode/CMakeLists.txt` | `learning_video_decode_lib` + `learning_video_decode` |
| `experiments/tests/test_video_decode.cpp` | Catch2：合成纯色帧编码后再解码，亮度与 PNG 签名 |

可执行文件：`learning_video_decode`（cwd＝仓库根）。

## 数据流

```
out/video/encode/encode.vau
  → read_vau
  → CMVideoFormatDescriptionCreateFromH264ParameterSets(SPS/PPS, NAL length 4)
  → VTDecompressionSessionCreate（软件解码器，目标像素 420v）
  → 每个 access unit：CMBlockBuffer + CMSampleBufferCreateReady
       → VTDecompressionSessionDecodeFrame
       → 回调拷出 tight NV12，PTS 用 VAU 里的 pts_ns
  → 写 out/video/decode.staging/ 后替换 out/video/decode/
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 默认输入 | `out/video/encode/encode.vau` |
| 解码器 | `VTDecompressionSession`，`EnableHardwareAcceleratedVideoDecoder = false` |
| 输出像素 | NV12 `420v`，1280×720，每帧 **1,382,400** 字节；其他格式即失败 |
| 一对一 | 一个 access unit → 一帧；丢帧即失败 |
| PTS | 写入 manifest 的是 VAU 里的 `pts_ns`，不是解码器内部的帧序号时钟 |
| manifest 表头 | `frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes`（与采集相同） |
| 帧文件 | `frames/frame-000000.nv12` 起 |
| 预览 | 解码后的第一帧 → `preview.png` |
| 发布 | `out/video/decode.staging/` 成功后替换 `out/video/decode/` |

送给解码器的 `CMTime` 是 access unit 序号 / 30，避免采集 host time 的纳秒刻度。文件里的呈现时间仍是采集 PTS。

Catch2 用 Y=48 与 Y=200 的两帧纯色图走编码再解码，要求平均亮度落在目标 ±40 以内，并检查 PNG 签名。不打开摄像头。

## 关键 API / 决策

- **`CMVideoFormatDescriptionCreateFromH264ParameterSets`**：用 VAU 头里的原始 SPS/PPS NAL。
- **`CMSampleBufferCreateReady`**：载荷已是 4 字节长度前缀的 AVCC，直接包成一个 sample。
- **`VTDecompressionSessionDecodeFrame`** + **`VTDecompressionSessionWaitForAsynchronousFrames`**：一帧一等，回调里把 biplanar `420v` 收成 tight 字节。
- **决策**：系统解码器，按 access unit，不引入 FFmpeg → `docs/adr/0009-learning-video-decode-videotoolbox.md`。
- **可测缝**：`decode_vau_to_nv12`（内部会先编码合成帧）。

## 踩坑与边界

- 沙箱里 VideoToolbox 可能失败；护栏在 GitHub Actions `macos-latest`。
- 像素格式不是 `420v` 就失败，不悄悄改成 BGRA。stderr 会带上实际 fourcc。
- 不重跑 UDP。只拿到分片、没有 VAU 头里的 SPS/PPS，本函数解不开——那是离线接力的缺口。
- 旧 `out/video/decode/` ≠ 本次成功。失败不发布半成品。
- 不做丢包隐藏、不做 jitter buffer、不把解码帧修到与源逐字节相同。

## 下一课钩子

**11 视频·渲染**默认读 `out/video/decode/` 的 tight NV12 与 manifest，按 `relative_pts_ns` 把画面显示出来。A/V 同步仍然只叙述、不在渲染票里做成实验。
