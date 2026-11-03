# 视频·解码

## 目标

Learning Pipeline Stages 的**解码**阶段（视频）：把 H.264 access unit 还原成可显示的 **Raw Video Frame**（tight NV12 `420v`），写出逐帧文件、PTS manifest 与首帧 PNG。格式契约以课记为准。

输入是码流本身，不是 Transport Unit 的分片外壳。jitter buffer 在本阶段之前；A/V 同步在把帧交给画面的时候——**本阶段都不实现、不验收**。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入：`out/video/encode/encode.vau`（先跑编码，或 `--input` 指向同契约 VAU）
- 不需要摄像头，也不重跑 UDP 环回
- `VTDecompressionSession` 须在非沙箱环境运行

## 选用库

macOS **VideoToolbox / `VTDecompressionSession`**（`CMVideoFormatDescriptionCreateFromH264ParameterSets` + `CMSampleBufferCreateReady`）。无第三方媒体库。决策见 `docs/adr/0009-learning-video-decode-videotoolbox.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_video_decode
./experiments/build/video/decode/learning_video_decode
# 可选：./experiments/build/video/decode/learning_video_decode --input out/video/encode/encode.vau
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被整目录替换）`out/video/decode/`：
  - `frames/frame-000000.nv12` 起逐帧 tight NV12
  - `manifest.csv`（表头与采集阶段相同）
  - `preview.png`（解码后的首帧；有损，不要求与采集 PNG 逐像素相同，但应能认出同一画面）
- 终端成功日志含 token：`frames_decoded`、`width`、`height`、`pixel_format`、`frame_bytes`、`manifest`、`preview`
- `frames_decoded` 等于 VAU 里的 access unit 数

## 失败时仍记录什么

- 缺 VAU / 非 LVAU / 缺少 SPS 或 PPS 时的 stderr
- `CMVideoFormatDescriptionCreateFromH264ParameterSets` / `VTDecompressionSessionCreate` / `VTDecompressionSessionDecodeFrame` 的 OSStatus
- 解码像素格式不是 `420v`、尺寸不是 1280×720、或帧数对不上时的 stderr（含实际 fourcc）
- unknown argument / `--input` 缺值时的 stderr
- 若退出非 0 但仍有旧 `out/video/decode/`，该目录可能来自上次成功，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_video_decode`；人工按本文看 `preview.png` 与帧文件。可无摄像头契约见 `experiments/tests/test_video_decode.cpp`（合成纯色帧编解码，看亮度是否回到源附近）；一键护栏 `./scripts/check-learning.sh`。

本阶段读 `encode.vau`，不读 UDP。环回等价性在本地传输/打包阶段已经用逐字节比对证明。

- **课记（契约 + 代码地图）**：[decode.notes.md](./decode.notes.md)
- **上游编码短文**：[encode.md](./encode.md)
