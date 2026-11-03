# 07: 视频·采集

**What to build:** 视频 Learning Pipeline Stage「采集」：从默认摄像头取得 NV12 `420v` 原始帧，留下逐帧字节、manifest、首帧预览与短文。音频五段完成后再做。库与本票的格式契约由 grilling 选定。

**Blocked by:** 06 音频·渲染

**Status:** done

## Acceptance

- [x] `experiments/video/capture/` 提供可执行目标 `learning_video_capture` 与静态库 `learning_video_capture_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 采集 API：macOS **AVFoundation** 的 `AVCaptureSession` + `AVCaptureVideoDataOutput`；无第三方媒体库；仅默认摄像头，无音频输入、设备列表、device id、实时预览窗口或 `.app`
- [x] 会话固定使用 `AVCaptureSessionPreset1280x720`，目标 30 fps（`activeVideoMinFrameDuration = 1/30 s`），请求 `kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange`；`alwaysDiscardsLateVideoFrames = YES`；不自动降分辨率或像素格式
- [x] 权限：查询 `authorizationStatusForMediaType:AVMediaTypeVideo`；未决定时请求 access；拒绝/受限、无默认设备、配置失败、参数非法等写 stderr 并 exit ≠ 0，成功 exit 0
- [x] CLI：无参数时采集 **30** 帧；`--frames` 取值 **[1, 90]**；unknown argument / 缺值 / 非整数 / 越界均失败；无 `--seconds`、`--device`
- [x] 帧与超时：必须恰好取得请求帧数；权限提示不计时，session 开始后完整采集的硬上限为 **15 s**；不足则失败
- [x] 方向与镜像：保持设备交付顺序与方向，显式关闭自动旋转/镜像；首帧预览与原始帧一致
- [x] 帧契约：每帧 tight-packed NV12；Y plane 为 `width × height` bytes，随后是 interleaved CbCr plane 的 `2 × ceil(width/2) × ceil(height/2)` bytes；移除 `CVPixelBufferGetBytesPerRow` padding；`1280×720` 每帧固定 **1,382,400** bytes
- [x] 回调边界：delegate queue 只校验 `420v` 与尺寸、复制 tight NV12 及 PTS；文件 I/O、颜色转换和发布在采集完成后执行；90 帧契约允许最多约 124 MB 帧数据在内存
- [x] 成功产物先写入 `out/video/capture.staging/`，完整成功后替换 `out/video/capture/`；此前完整成功目录在本次发布前保持原样；失败清理 staging，不发布半成品
- [x] 产物：`out/video/capture/frames/frame-000000.nv12` 起逐帧文件（索引至少六位）、`out/video/capture/manifest.csv`、`out/video/capture/preview.png`
- [x] `manifest.csv` 表头固定为 `frame_index,file,width,height,pixel_format,pts_ns,relative_pts_ns,bytes`；PTS 来自 `CMSampleBufferGetPresentationTimeStamp` 并转换为整数纳秒，`relative_pts_ns` 以首帧为零点；不伪造 duration
- [x] 首帧预览由本批第一张有效 `CVPixelBuffer` 经 macOS **Core Image + ImageIO** 写 PNG；PNG 是派生证据，不取代原始 `.nv12`
- [x] 成功 stdout 含 token `frames_captured`、`width`、`height`、`pixel_format`、`frame_bytes`、`manifest`、`preview`；失败 stderr 简述 NSError / permission / format / timeout / IO 原因，格式不符时给出实际像素格式和可用 `videoSettings` 信息
- [x] 短文 `docs/learning/video/capture.md`（按 `_template.md`）：目标、前置（摄像头权限、非沙箱、默认摄像头、硬件支持）、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；一句话交代 A/V 同步位置，不实现
- [x] 课记 `docs/learning/video/capture.notes.md`（按 `_notes_template.md` 六栏）：NV12 字节布局、PTS、callback queue、权限/硬件边界、代码地图、下游编码钩子
- [x] Catch2：用合成 tight NV12 测帧布局、逐帧文件、CSV 字段、首帧 PNG 与 PTS 归一化；不测 AVFoundation、不访问摄像头；`./scripts/check-learning.sh` 在无摄像头环境仍通过
- [x] 真实摄像头人工烟测：按短文运行，PNG 能看见刚拍画面，帧文件/manifest 与 stdout 一致（2026-11-03 FaceTime `--frames 5` 通过）
- [x] 屏幕共享不在范围内

## 约定（grilling 2026-10-30）

- **API / 格式**：AVFoundation `AVCaptureSession` + `AVCaptureVideoDataOutput`，NV12 `420v`；无第三方媒体库。见 `docs/adr/0006-learning-video-capture-avfoundation-nv12.md`
- **固定采集**：默认摄像头、1280×720、目标 30 fps、默认 30 帧、上限 90 帧；按帧数而非秒数停止
- **可观察结果**：逐帧 tight `.nv12` + PTS manifest + 首帧 PNG；真实摄像头首帧人工确认
- **自动化边界**：合成帧与纯写盘契约进 Catch2；权限、AVFoundation 会话与真实摄像头只做人工烟测
- **无降级**：硬件不支持 720p/420v、超时、写盘/发布失败都失败；合成帧不冒充摄像头

## Comments

- 2026-10-30: `/grill-with-docs` 完成三轮 grilling：选定 AVFoundation + `AVCaptureVideoDataOutput`、NV12 `420v`、1280×720/目标 30 fps/默认 30 帧、逐帧 tight 文件 + PTS manifest + 首帧 PNG、staging 发布、15 s 上限和无硬件降级；新增 ADR-0006，磨尖 `GLOSSARY.md` 的 Raw Video Frame / PTS。当前主机未发现摄像头，真实采集留待人工验收。
- 2026-11-03: `/implement`——合成帧 Catch2 + AVFoundation 实验 + 短文/课记；macOS 上 `startRunning` 会重置 `activeFormat`，须再钉 720p/30；本机 FaceTime 摄像头烟测 `--frames 5` 通过（1280×720、`frame_bytes=1382400`、preview PNG 可见）。
