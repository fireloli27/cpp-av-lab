# 视频·编码

## 目标

Learning Pipeline Stages 的**编码**阶段（视频）：把采集契约的 **Raw Video Frame**（tight NV12 `420v`）压成 **H.264** 压缩码流，写出可观察的 access unit 文件，验证本段可独立跑通。格式契约以课记为准，本文不展开。

AEC 通常在采集后、编码前的发送侧处理带（视频这条路径不产生回声参考）；弱网码率自适应会作用在编码码率——**本阶段实验均不实现、不验收**（位置说明见课记）。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入目录：`out/video/capture/`（先跑 `learning_video_capture`，或用 `--input` 指向同契约目录）
- 无需摄像头权限（本可执行文件不打开摄像头）
- `VTCompressionSession` 须在非沙箱环境运行

## 选用库

macOS **VideoToolbox / `VTCompressionSession`**，编解码器 **H.264**（Baseline，`kCMVideoCodecType_H264`）。无第三方媒体库。决策见 `docs/adr/0007-learning-video-encode-videotoolbox-h264.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_video_encode
# 若尚无采集产物：
./experiments/build/video/capture/learning_video_capture --frames 5
./experiments/build/video/encode/learning_video_encode
# 可选：./experiments/build/video/encode/learning_video_encode --input out/video/capture
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被整目录替换）`out/video/encode/`：
  - `encode.vau`（学习容器里的 H.264 access unit，不是 MP4）
  - `manifest.csv`（每条 access unit 的 PTS 与字节数）
- 终端成功日志含 token：`codec`、`width`、`height`、`pixel_format`、`bitrate_bps`、`frames_encoded`、`access_units`、`keyframes`、`output_bytes`、`output`、`manifest`（须与课记契约一致）
- `frames_encoded` 等于输入帧数，且等于 `access_units`；`keyframes` ≥ 1

## 失败时仍记录什么

- 缺输入目录 / manifest 表头不符 / 非 1280×720 `420v` / 帧文件字节数不对时的 stderr
- `VTCompressionSessionCreate` / `VTSessionSetProperty` / `VTCompressionSessionEncodeFrame` 的 OSStatus
- 参数集缺少 SPS/PPS、帧被丢弃、PTS 对不上时的 stderr
- unknown argument / `--input` 缺值时的 stderr
- 若退出非 0 但仍有旧 `out/video/encode/`，注明该目录可能来自上次成功运行，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_video_encode`；人工按本文验收（看 `encode.vau` 与 `manifest.csv`，并核对 stdout）。可无摄像头契约见 `experiments/tests/test_video_encode.cpp`；一键护栏 `./scripts/check-learning.sh`。

- **课记（契约 + 代码地图）**：[encode.notes.md](./encode.notes.md)
- **上游采集短文**：[capture.md](./capture.md)
