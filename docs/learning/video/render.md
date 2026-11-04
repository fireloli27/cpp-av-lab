# 视频·渲染

## 目标

Learning Pipeline Stages 的**渲染**阶段（视频）：把解码后的 tight NV12 帧显示到窗口里，按 **Presentation Timestamp** 的相对间隔往前播。验证「可显示帧 → 画面」。

`frames_presented` 只证明帧交给了 `NSImageView`。窗口里能不能看见画面，靠人眼。

jitter buffer 在接收后、解码前；A/V 同步通常以音频播放时钟为基准，在渲染这一带对齐音画。本实验只放视频，**不实现、不验收**这些旁路。贯通怎么串，见 [pipeline.md](./pipeline.md)。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入目录：`out/video/decode/`（先跑解码，或 `--input` 指向同契约目录，例如 `out/video/capture/`）
- 需要图形会话（窗口能弹出来）。Catch2 不创建窗口
- 非沙箱运行更稳妥

## 选用库

macOS **AppKit**（`NSWindow` + `NSImageView`），像素转换用 **Core Image**。无第三方媒体库。决策见 `docs/adr/0010-learning-video-render-appkit.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_video_render
./experiments/build/video/render/learning_video_render
# 可选：./experiments/build/video/render/learning_video_render --input out/video/decode
```

窗口标题是 `learning_video_render`，内容区 960×540 point，画面按比例缩放。播完后最后一帧再停 0.5 s，然后进程退出。

## 可观察成功判据

- 命令退出码为 0
- 播放期间能看见窗口里的画面（有损解码结果，不要求与摄像头逐像素相同）
- 终端成功日志含 token：`frames_loaded`、`frames_presented`、`width`、`height`、`pixel_format`
- `frames_loaded == frames_presented`
- 不向 `out/` 写新文件

## 失败时仍记录什么

- 缺 manifest / 表头不符 / 非 1280×720 `420v` / 帧文件大小不对 / PTS 不递增时的 stderr
- 建窗口失败、某一帧转不成 `NSImage` 时的 stderr
- unknown argument / `--input` 缺值时的 stderr
- 进程非 0 退出时，不要把中途闪过的窗口当成成功
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_video_render`；人工按本文看窗口。可无窗口契约见 `experiments/tests/test_video_render.cpp`（目录加载与 PTS 间隔）；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[render.notes.md](./render.notes.md)
- **视频五段怎么串**：[pipeline.md](./pipeline.md)
- **上游解码短文**：[decode.md](./decode.md)
