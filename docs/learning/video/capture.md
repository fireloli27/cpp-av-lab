# 视频·采集

## 目标

Learning Pipeline Stages 的**采集**阶段（视频）：从默认摄像头取到未压缩的 **Raw Video Frame**（tight NV12 `420v`），写出逐帧文件、PTS manifest 与首帧 PNG，验证本段可独立跑通。格式契约以课记为准，本文不展开。

A/V 同步属叙述必会旁路（通常在接收/播放一带对齐音画）；**本阶段实验不实现、不验收**。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- **摄像头权限**：系统设置 → 隐私与安全性 → 摄像头 → 允许你使用的终端（Terminal / iTerm / Cursor 等）
- 系统有可用的**默认摄像头**；硬件须支持 `AVCaptureSessionPreset1280x720` 与 NV12 `420v`（不支持则失败，不降级）
- Agent 烟测（真开摄像头）须在非沙箱环境执行（`required_permissions: ["all"]`）；`./scripts/check-learning.sh` 不访问摄像头

## 选用库

macOS **AVFoundation**（`AVCaptureSession` + `AVCaptureVideoDataOutput`）；首帧预览用 **Core Image + ImageIO**。无第三方媒体库。决策见 `docs/adr/0006-learning-video-capture-avfoundation-nv12.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_video_capture
./experiments/build/video/capture/learning_video_capture
# 可选：./experiments/build/video/capture/learning_video_capture --frames 10
```

`--frames` 取值须在 **[1, 90]**；默认 **30** 帧。采集硬上限 15 s（权限提示不计时）。

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被整目录替换）`out/video/capture/`：
  - `frames/frame-000000.nv12` 起逐帧 tight NV12
  - `manifest.csv`
  - `preview.png`（首帧预览；应能看见刚拍画面）
- 终端成功日志含 token：`frames_captured`、`width`、`height`、`pixel_format`、`frame_bytes`、`manifest`、`preview`（须与课记契约一致）

## 失败时仍记录什么

- 权限拒绝 / 无默认设备 / `AVCaptureSessionPreset1280x720` 不可用时的 stderr
- 像素格式或尺寸不符时的实际格式与 stderr（含可用 `videoSettings` 线索）
- 15 s 超时或帧数不足时的 stderr
- `--frames` 非法或 unknown argument 时的 stderr
- 写盘 / staging 发布失败原文
- 若退出非 0 但仍有旧 `out/video/capture/`，注明该目录可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_video_capture`；人工按本文验收（看 `preview.png` 与帧文件）。可无硬件契约见 `experiments/tests/test_video_capture.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[capture.notes.md](./capture.notes.md)
