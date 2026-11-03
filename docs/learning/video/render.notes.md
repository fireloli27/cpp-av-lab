# 课记：视频·渲染

短文（怎么跑、怎么验收）见 [render.md](./render.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**。

## 这一课在管线哪

Learning Pipeline Stages 的**渲染**（视频）：**可显示帧 → 画面**。本课不解码、不打开摄像头。

- **上游**：`out/video/decode/`（或 `--input` 指向的同契约目录）：tight NV12 + PTS manifest
- **本段产出**：窗口里的画面 + stdout。不写文件
- **下游**：无。视频五段到此接上，串法见 [pipeline.md](./pipeline.md)

旁路（只叙述）：

1. **jitter buffer** 在接收之后、解码之前，吸收网络到达抖动。它不是这里的显示等待。
2. **A/V 同步** 以音频播放时钟为主钟，在接收/播放一带把视频 PTS 对齐到那只钟。本实验没有音频，只按视频自己的 `relative_pts_ns` 往前播。
3. **AEC** 的参考信号取自音频渲染路径，不取自这个窗口。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/video/render/main.mm` | CLI、`NSWindow` / `NSImageView`、按间隔 `setImage:` |
| `experiments/video/render/render_frames.hpp` | 契约常量、`load_frame_directory`、`presentation_delays_ns` |
| `experiments/video/render/render_frames.cpp` | 读 manifest 与 tight NV12；算每帧开播前要等的纳秒 |
| `experiments/video/render/CMakeLists.txt` | `learning_video_render_lib`（纯 C++）+ `learning_video_render` |
| `experiments/tests/test_video_render.cpp` | Catch2：PTS 间隔与夹断、目录加载 / 格式拒绝。不创建窗口 |

可执行文件：`learning_video_render`（cwd＝仓库根）。

## 数据流

```
out/video/decode/（或 --input）
  → load_frame_directory
  → presentation_delays_ns
  → NSWindow 960×540 pt，标题 learning_video_render
  → 每帧：等到 delay → Core Image 把 tight NV12 变成 NSImage → NSImageView
  → 最后一帧再停 0.5 s → 打印 frames_loaded / frames_presented 后退出
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 默认输入 | `out/video/decode/` |
| 帧 | 1280×720 tight NV12 `420v`，每帧 **1,382,400** 字节，最多 90 帧 |
| manifest 表头 | `frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes` |
| 第一帧等待 | 0 |
| 后续等待 | `relative_pts_ns` 之差；≤ 0 失败；大于 **200000000** ns（200 ms）则夹到 200 ms |
| 窗口 | 960×540 point，`NSImageScaleProportionallyUpOrDown`。源像素不改 |
| 结尾停留 | 0.5 s |
| 成功 | `frames_loaded == frames_presented`；`frames_presented` 是成功 `setImage:` 的次数 |
| 产物 | 无 |

Catch2 不创建 `NSWindow`，也不链接 AppKit。真窗口按短文人工看。

## 关键 API / 决策

- **`NSWindow`** + **`NSImageView`**：`setImage:` 之后 `displayIfNeeded`。
- **Core Image**：`CIImage imageWithCVPixelBuffer:` → `CIContext createCGImage:` → `NSImage initWithCGImage:size:`。
- **节奏**：`NSRunLoop runUntilDate:` 用课记里的 delay，不是显示器的 vsync 时钟。
- **决策**：自己把帧交给视图，不用外部查看器 → `docs/adr/0010-learning-video-render-appkit.md`。
- **可测缝**：`load_frame_directory`、`presentation_delays_ns`。

## 踩坑与边界

- 无图形会话时窗口出不来；这不是 Catch2 的失败条件。
- 200 ms 夹断是为了学习实验不要被一个异常 PTS 卡住。它不吸收网络抖动，不要把它叫成 jitter buffer。
- 只播视频。没有音频主钟，就还没有 A/V 同步实验。
- 不写 `out/`、无 `--device`、无全屏、无屏幕共享。
- 旧的解码目录不会被本程序修改。失败 exit ≠ 0，成功行不会打印。

## 下一课钩子

视频五段的拼接与缺口写在 [pipeline.md](./pipeline.md)。AEC、弱网码率自适应、jitter buffer、A/V 同步以及两种信令的边界不在本阶段做实验。
