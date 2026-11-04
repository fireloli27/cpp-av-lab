# 11: 视频·渲染

**What to build:** 视频 Learning Pipeline Stage「渲染」：读解码目录里的 tight NV12，用 AppKit 窗口按 PTS 间隔显示；写短文与课记。另写 `pipeline.md` 作为视频五段贯通说明（分段拼接，缺口点名）。

**Blocked by:** 10 视频·解码

**Status:** done

## Acceptance

- [x] `experiments/video/render/` 可执行目标 `learning_video_render`；目录内静态库 `learning_video_render_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`；不链接采集库或编码库
- [x] 显示：macOS **AppKit** `NSWindow` + `NSImageView`；NV12 经 Core Image 变成 `NSImage`；不调用外部查看器，不引入第三方媒体库
- [x] 输入：默认读 `out/video/decode/`；可选 `--input`；须为 1280×720 / `420v` / 每帧 1,382,400 字节，manifest 表头与采集相同；不重采样
- [x] 节奏：第一帧不等待；其后等待 `relative_pts_ns` 之差；非正间隔失败；大于 200 ms 夹到 200 ms。窗口 960×540 point，源像素不变。最后一帧再停 0.5 s
- [x] 成功：exit 0；stdout 含 token `frames_loaded`、`frames_presented`、`width`、`height`、`pixel_format`；判据 `frames_loaded == frames_presented`。不向 `out/` 写产物
- [x] 失败（缺输入 / 格式不符 / 0 帧 / 建窗口或转图失败等）：stderr 简述 + exit ≠ 0
- [x] 短文 `docs/learning/video/render.md`（按 `_template.md`）：目标、前置、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明 `frames_presented` 只证明交给了视图；链接 `pipeline.md`
- [x] 课记 `docs/learning/video/render.notes.md`（按 `_notes_template.md` 六栏）：jitter buffer ≠ 显示等待；A/V 同步以音频时钟为主钟（本实验没有音频）；AEC 参考在音频渲染路径
- [x] 贯通说明 `docs/learning/video/pipeline.md`：五段命令与产物交接；缺口点名（解码不吃 UDP、参数集不进环回、有损、离线接力、分片 ≠ RTP）。人工验收勾选留给作者。不提供一键串跑脚本
- [x] Catch2：PTS 间隔与 200 ms 夹断、目录加载与格式拒绝。不创建 `NSWindow`、不提交二进制 fixture；`scripts/check-learning.sh` 仍可通过
- [x] 不把 jitter buffer / A/V 同步 / AEC 做成实验
- [x] ADR：`docs/adr/0010-learning-video-render-appkit.md`；术语与 `GLOSSARY.md`「渲染」一致

## 约定（2026-11-03）

- **路径**：`experiments/video/render/` ↔ `docs/learning/video/render.md` + `render.notes.md`；贯通说明 `docs/learning/video/pipeline.md`
- **与 10 的交接**：硬输入是解码目录的 tight NV12。采集目录布局相同，可以 `--input` 短路预览，贯通路径仍走解码结果
- **可观察结果**：看见窗口 + stdout；不写成功标记文件
- **一键脚本**：不做。采集要摄像头，脚本会掩盖人工前置

## Comments

- 2026-11-03: 选定 AppKit 窗口而不是外部查看器，和音频不用 `afplay` 同一理由。Linux 上跑过目录加载与 PTS 夹断；窗口本身要等 macOS CI 编译，以及早上用真摄像头串跑时看画面。`pipeline.md` 的人工验收保持未勾。
