# 10: 视频·解码

**What to build:** 视频 Learning Pipeline Stage「解码」：把 H.264 access unit 还原为 tight NV12 帧，留下可观察的逐帧文件、manifest、首帧 PNG 与短文。不重跑 UDP。

**Blocked by:** 09 视频·本地传输/打包

**Status:** done

## Acceptance

- [x] `experiments/video/decode/` 可执行目标 `learning_video_decode`；目录内静态库 `learning_video_decode_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`；不链接采集库
- [x] 解码 API：macOS **VideoToolbox / `VTDecompressionSession`**，按 access unit；`CMVideoFormatDescriptionCreateFromH264ParameterSets` + `CMSampleBufferCreateReady`；请求软件解码器；无第三方媒体库
- [x] 输入：默认读 `out/video/encode/encode.vau`；可选 `--input`；不读 UDP，不把 Transport Unit 外壳当输入
- [x] 输出与采集契约对齐：1280×720 tight NV12 `420v`，每帧 1,382,400 字节；其他像素格式或尺寸即失败；PTS 来自 VAU 的 `pts_ns`
- [x] 成功时经 staging 替换 `out/video/decode/`（`frames/frame-000000.nv12`、`manifest.csv`、`preview.png`）；`frames_decoded` 等于 access unit 数；不要求与编码前逐字节相同
- [x] CLI：无摄像头、无 GUI；成功 exit 0，stdout 含 token `frames_decoded`、`width`、`height`、`pixel_format`、`frame_bytes`、`manifest`、`preview`；失败 stderr 简述 + exit ≠ 0
- [x] 短文 `docs/learning/video/decode.md`（按 `_template.md`）：目标、前置、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明不重跑 UDP
- [x] 课记 `docs/learning/video/decode.notes.md`（按 `_notes_template.md` 六栏）
- [x] Catch2：合成纯色 NV12 编码后再解码，帧大小、亮度窗口、PNG 签名、manifest 表头；不打开摄像头；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0009-learning-video-decode-videotoolbox.md`；术语与 `GLOSSARY.md`「解码」一致

## 约定（2026-11-03）

- **路径**：`experiments/video/decode/` ↔ `docs/learning/video/decode.md` + `decode.notes.md`
- **与 09 的交接**：硬输入是 VAU 码流；环回只证明载荷字节能走通，不把 socket 再接进解码器
- **自动化边界**：合成帧编解码进 Catch2；真人画面看 `preview.png` 按短文

## Comments

- 2026-11-03: 与 ADR-0007 对称，用 `VTDecompressionSession` 按 access unit 解回 tight NV12。送给解码器的 `CMTime` 是序号/30，manifest 里仍是采集 PTS。本环境不能链接 VideoToolbox；`macos-latest` 上的 `check-learning` 是编译和合成帧证明。摄像头画面留给人工看 PNG。
