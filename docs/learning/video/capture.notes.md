# 课记：视频·采集

短文（怎么跑、怎么验收）见 [capture.md](./capture.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改分辨率/像素格式/帧数上限先改这里（并同步代码常量），短文与 teach 课只引用。

NV12 平面布局与 PTS vs 帧序号 → [课 0006](../../../teach/lessons/0006-nv12-frame-memory.html) · [课 0007](../../../teach/lessons/0007-pts-vs-frame-index.html) · [课 0008](../../../teach/lessons/0008-capture-stays-cpp-session-is-objc.html)（语言缝） · [速查](../../../teach/reference/video-raw-frame-cheatsheet.html)。

## 这一课在管线哪

Learning Pipeline Stages 的**采集**（视频）：摄像头 → **Raw Video Frame**（未压缩）。本课不编码、不传输、不做 A/V 同步。

- **上游**：无（硬件默认摄像头）
- **本段产出**：`out/video/capture/` 下 tight `.nv12` 逐帧 + PTS `manifest.csv` + `preview.png`
- **下游**：视频「编码」默认读这些 Raw Video Frame（见 [encode.notes.md](./encode.notes.md)，ADR-0007）

A/V 同步属叙述必会旁路，通常在接收/播放一带；本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/video/capture/main.mm` | CLI、权限、`AVCaptureSession` 取帧、回调里只拷贝 tight NV12 + PTS |
| `experiments/video/capture/capture_publish.hpp` | 契约常量 + `publish_capture_artifacts` / PNG 声明 |
| `experiments/video/capture/nv12.cpp` | `nv12_tight_bytes`、`pack_nv12_planes_to_tight`（去 stride padding） |
| `experiments/video/capture/capture_publish.mm` | staging 写盘、manifest、Core Image + ImageIO 预览、发布替换 |
| `experiments/video/capture/CMakeLists.txt` | `learning_video_capture_lib` + `learning_video_capture` |
| `experiments/tests/test_video_capture.cpp` | Catch2：合成 NV12 校验布局 / CSV / PNG / 失败不污染旧产物 |

可执行文件：`learning_video_capture`（cwd＝仓库根）。

## 数据流

```
默认摄像头
  → AVCaptureVideoDataOutput 回调（serial queue）
      校验 420v + 1280×720，拷贝 tight NV12 + PTS（不做文件 I/O）
  → 凑满 --frames 或 15 s 超时
  → 主线程 publish_capture_artifacts：
      写 out/video/capture.staging/ → 成功后替换 out/video/capture/
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 分辨率 | 1280×720（`AVCaptureSessionPreset1280x720`） |
| 目标帧率 | 30 fps（`activeVideoMinFrameDuration` / `activeVideoMaxFrameDuration = 1/30 s`） |
| 像素格式 | NV12 `420v` = `kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange` |
| 每帧字节 | tight：Y `width×height` + interleaved CbCr `2×ceil(w/2)×ceil(h/2)` → **1,382,400** |
| 默认 / 上限帧数 | 30 / 90（`--frames` ∈ [1, 90]） |
| 采集超时 | session 开始后 **15 s**（权限提示不计时） |
| 产物目录 | `out/video/capture/`（经 `capture.staging` 发布） |
| 帧文件 | `frames/frame-000000.nv12` 起（索引 ≥ 六位） |
| manifest 表头 | `frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes` |
| PTS | `CMSampleBufferGetPresentationTimeStamp` → 整数纳秒；`relative_pts_ns` 以首帧为零 |
| 预览 | 首帧经 Core Image + ImageIO → `preview.png`（派生证据） |

回调队列只做校验与内存拷贝；文件 I/O、颜色转换、发布在采集完成后执行。Catch2 用合成 tight NV12，不测 AVFoundation、不访问摄像头。

## 关键 API / 决策

- **AVFoundation**：`authorizationStatusForMediaType:` / `requestAccessForMediaType:` → `defaultDeviceWithMediaType:` → `AVCaptureSession` + `AVCaptureVideoDataOutput`；`alwaysDiscardsLateVideoFrames = YES`；关闭自动镜像，并在支持时把 `videoRotationAngle` 钉为 0。
- **预览**：首帧 tight NV12（与采集回调里从 `CVPixelBuffer` 拷出的字节一致）→ `CIImage` + `CIContext` + `CGImageDestination`（PNG）。
- **决策**：系统 AVFoundation + NV12，不引入第三方 → `docs/adr/0006-learning-video-capture-avfoundation-nv12.md`。
- **可测缝**：`nv12_tight_bytes`、`pack_nv12_planes_to_tight`、`relative_pts_ns`、`publish_capture_artifacts`；真摄像头靠短文人工看 PNG。

## 踩坑与边界

- Agent 沙箱内开摄像头常失败；须非沙箱 + 终端摄像头权限（见短文前置）。
- 硬件不支持 720p / `420v` 时明确失败，不降分辨率或改像素格式。
- **macOS**：仅设 `AVCaptureSessionPreset1280x720` 不够——`startRunning` 常把 `activeFormat` 打回 1080p；须在 start 后再 `lockForConfiguration` 钉死 1280×720 / 30 fps，**再**挂 `setSampleBufferDelegate`，避免中间 1080p 帧误杀整次采集。
- 失败非 0；旧 `out/video/capture/` ≠ 本次成功。失败清理 staging，不发布半成品。
- 无设备列表、无 `--device` / `--seconds`、无实时预览窗口、无 `.app`、无屏幕共享。
- 90 帧契约允许约 124 MB 帧数据驻留内存（拷齐后再写盘）。

## 下一课钩子

**08 视频·编码**默认读 `out/video/capture/` 下的 Raw Video Frame（或短文写明的等价输入），压成 H.264 access unit（`encode.vau`）。见 [encode.notes.md](./encode.notes.md)。
