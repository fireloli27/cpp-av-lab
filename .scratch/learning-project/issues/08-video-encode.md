# 08: 视频·编码

**What to build:** 视频 Learning Pipeline Stage「编码」：将采集侧 Raw Video Frame 压成 H.264 access unit，写出可观察 VAU，并写对应短文与课记。库与容器已在本票按音频阶段的系统 API 姿势选定。

**Blocked by:** 07 视频·采集

**Status:** done

## Acceptance

- [x] `experiments/video/encode/` 可执行目标 `learning_video_encode`；目录内静态库 `learning_video_encode_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 编码 API：macOS **VideoToolbox / `VTCompressionSession`**，格式 **H.264**（`kCMVideoCodecType_H264`，Baseline，`AllowFrameReordering = false`）；请求软件编码器；无第三方媒体库（不引入 FFmpeg / x264）
- [x] 输入：默认读 `out/video/capture/`；可选 `--input <dir>`；须为 **1280×720 / NV12 `420v` / 每帧 1,382,400 字节**，`pts_ns` 严格递增，帧数 **[1, 90]**，否则失败（不缩放、不改像素格式）
- [x] 码率固定 **2 Mbps**；GOP 最长 30 帧；第一帧强制 keyframe；无 `--bitrate`、无自适应码率
- [x] 成功时经 staging 替换 `out/video/encode/encode.vau` 与 `out/video/encode/manifest.csv`；一个 Raw Video Frame 对应一个 access unit；无 `--output`；本票不做 RTP
- [x] CLI：无摄像头、无 GUI；成功 exit 0，stdout 含 token `codec`、`width`、`height`、`pixel_format`、`bitrate_bps`、`frames_encoded`、`access_units`、`keyframes`、`output_bytes`、`output`、`manifest`；失败 stderr 简述 + exit ≠ 0
- [x] 短文 `docs/learning/video/encode.md`（按 `_template.md`）：目标、前置、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；弱网码率自适应位置预告，不实现、不验收
- [x] 课记 `docs/learning/video/encode.notes.md`（按 `_notes_template.md` 六栏）：VAU 布局为契约 SSOT、代码地图、数据流、API/决策、踩坑、下一课钩子
- [x] Catch2：VAU 往返与损坏拒绝、采集目录契约拒绝、合成 NV12 → `encode_nv12_to_vau`；不打开摄像头、不读真人采集目录；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0007-learning-video-encode-videotoolbox-h264.md`；术语与 `GLOSSARY.md`「编码」一致

## 约定（2026-11-03）

- **路径**：`experiments/video/encode/` ↔ `docs/learning/video/encode.md` + `encode.notes.md`
- **产物**：`encode.vau` 是学习容器（参数集 + AVCC access unit + PTS），不是产品容器拍板；09 以其中 access unit 载荷为默认可追溯输入
- **自动化边界**：容器与目录契约、以及不依赖摄像头的 `VTCompressionSession` 合成帧进 Catch2；真人采集目录按短文人工串跑
- **政策**：维持 harness「实验代码无第三方媒体库；系统框架允许」

## Comments

- 2026-11-03: 按 03/07 的系统 API 姿势选定 VideoToolbox H.264 Baseline + 学习容器 VAU（非 MP4、非 FFmpeg）。编码器时钟用帧序号/30，文件内 PTS 仍是采集 `pts_ns`。本环境是 Linux，不能链接 VideoToolbox；编译与合成帧编码以 GitHub Actions `macos-latest` 的 `./scripts/check-learning.sh` 为准。摄像头串跑留给人工。
