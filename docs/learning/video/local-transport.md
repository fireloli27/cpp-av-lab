# 视频·本地传输/打包

## 目标

Learning Pipeline Stages 的**本地传输/打包**阶段（视频）：把编码后的 H.264 access unit 组织成 **Transport Unit**，经本机 UDP loopback 发送并接收，核对重组后的载荷与发出去的逐字节一致。验证的是「打包 ≠ 编码」以及「一个单元可以分成多个 datagram」。

本票的分片格式 **不是** 对「产品必须使用 RTP」的拍板，也不是「产品禁止 RTP」。产品侧仍见 spec 未决 #5。

jitter buffer 在接收之后、解码之前吸收网络到达抖动——**本阶段不实现、不验收**。

## 前置

- macOS；已安装 CMake ≥ 3.20 与 C++17 编译器（如 Apple Clang）
- 在仓库根（`matt/`）作为 cwd
- 默认输入：`out/video/encode/encode.vau`（先跑 `learning_video_encode`，或 `--input` 指向同契约 VAU）
- 无需摄像头；环回走 `127.0.0.1`。非沙箱更稳妥（与音频环回相同）

## 选用库

macOS 上读 VAU 用本仓库的 `read_vau`（不引入媒体库）。发送/接收用 **POSIX UDP**。无 RTP 库。决策见 `docs/adr/0008-learning-video-local-transport-fragmented-udp.md`。

## 运行步骤

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build --target learning_video_local_transport
./experiments/build/video/local-transport/learning_video_local_transport
# 可选：./experiments/build/video/local-transport/learning_video_local_transport --input out/video/encode/encode.vau
```

## 可观察成功判据

- 命令退出码为 0
- 仓库根下出现（或被覆盖）`out/video/local-transport/loopback-ok.txt`
- 终端成功日志含 token：`units_sent`、`units_recv`、`payload_bytes_sent`、`payload_bytes_recv`、`fragments_sent`、`fragments_recv`、`frag_payload_max`、`port`、`artifact`
- `units_sent == units_recv`，两侧载荷字节数相等，两侧分片数相等

## 失败时仍记录什么

- 缺 VAU / 非 LVAU / 契约不符时的 stderr
- `socket` / `bind` / `sendto` / `recvfrom`（含超时）原文
- 重组失败或逐字节不一致时的 stderr
- unknown argument / `--input` 缺值时的 stderr
- 若退出非 0 但仍有旧 `loopback-ok.txt`，该文件属于上次成功，不能当作本次成功证据
- CMake 配置/编译错误原文

## 与主缝关系

独立可执行入口 `learning_video_local_transport`；人工按本文验收（看 artifact 与 stdout 两侧计数）。可无摄像头契约见 `experiments/tests/test_video_local_transport.cpp`；一键护栏 `./scripts/check-learning.sh`。

解码阶段**不**再读 UDP。本阶段的逐字节一致就是「载荷可以原样交到解码器」的证据。

- **课记（契约 + 代码地图）**：[local-transport.notes.md](./local-transport.notes.md)
- **上游编码短文**：[encode.md](./encode.md)
