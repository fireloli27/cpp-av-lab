# WebRTC 对照

本篇把 Learning Pipeline Stages 和叙述必会的旁路，映射到 **libwebrtc** 这棵客户端源码树上的对应概念。它满足 Learning Exit Criteria 和产品假设 H8 的学习侧硬要求。

**改这棵树、编过它、或提交补丁，都不是本篇的验收条件。** 本仓库不 vendoring libwebrtc。

对照的是客户端栈。SFU 不在这棵树里：它是媒体服务器，转发已经编码的 RTP，基本不解码混画。树里的 `pc/` 是客户端的会话与媒体协商，不是 Host App 的 Call Signaling，也不借此决定 Media Signaling 走哪条承载（spec 未决 #1 仍开）。

## 材料基线

| 项 | 值 |
| --- | --- |
| 树 | [https://webrtc.googlesource.com/src/](https://webrtc.googlesource.com/src/) |
| 分支 | `refs/heads/main` |
| 查阅日 | 2026-11-03 |
| 位置 | `refs/heads/main@{#49134}` |
| 该次提交 | Change-Id `I4fe360e90a4ed5c777ed3e05a82fb0b43e2b820d`（[webrtc-review 513200](https://webrtc-review.googlesource.com/c/src/+/513200)，把 Chromium revision roll 到 `4c6d6a8665`） |

路径以下都按这次查阅时的 `main` 为准。`main` 会继续走；本篇不改写产品选哪一版（产品假设 U4、U9 仍开）。这只是学习对照的材料基线。

学习实验的选型（Opus、H.264、长度前缀、UDP 分片、AppKit 窗口）是 macOS 上的教学替身。WebRTC 的视频编码器接口同时覆盖 H.264 以外的编码格式；包格式是 RTP，不是本仓库的 Transport Unit。

## 五段

| Learning Pipeline Stage | 本仓库在做什么 | libwebrtc 里对应什么 |
| --- | --- | --- |
| 采集（音频） | AudioQueue 取 PCM | `AudioDeviceModule`，实现在 `modules/audio_device/`（`audio_device_impl.cc`）。采集到的 10 ms 帧经音频编码模块送进编码器 |
| 采集（视频） | AVFoundation → tight NV12 `420v` + PTS | `modules/video_capture/`（`video_capture_impl.cc`）拉设备帧。帧上带采集时间，后面的同步要用 |
| 编码（音频） | `AudioConverter` 把 PCM 压成 Opus，固定 64 kbps | `AudioEncoder`。音频编码模块的说明在 `modules/audio_coding/g3doc/index.md`：`Add10MsData()` 进去，编码后的包交给打包回调。Opus 是这套栈的常用语音编码 |
| 编码（视频） | `VTCompressionSession` 把 NV12 压成 H.264 access unit，固定 2 Mbps，无 B 帧 | `VideoEncoder`（`api/video_codecs/video_encoder.cc`）。发送管线在 `video/video_stream_encoder.cc`，它消费编码器输出，也是码率决定落下去的地方 |
| 本地传输/打包 | 一个 access unit 做一个 Transport Unit。音频是长度前缀 + 一个 UDP datagram；视频因单帧太大，拆成 1200 字节的 datagram（`au_index` 只用于重组）。**不是 RTP** | 视频 H.264 的 RTP 打包在 `modules/rtp_rtcp/source/rtp_format_h264.cc`：`RtpPacketizerH264` 对放不进一个包的 NAL 做 FU-A，对多个小 NAL 做 STAP-A。发送节奏和拥塞在 `video/video_send_stream_impl.cc`。ICE 一类的传输连通在 `p2p/`。学习实验的分片只对应「一个 access unit 可能变成多个 datagram」这件事，头不是 FU-A，也没有 RTP 的 sequence number / timestamp / SSRC |
| 解码（音频） | 按 Opus access unit 用 `AudioConverter` 还原 PCM。不读 UDP | 接收包先进 `NetEq`（`api/neteq/neteq.h`，实现在 `modules/audio_coding/neteq/`）。jitter buffer 和解码在这里是一件事：`IncomingPacket()` 入包，`PlayoutData10Ms()` 出 PCM |
| 解码（视频） | `VTDecompressionSession` 按 access unit 还原 tight NV12。参数集在 VAU 文件头，不在 UDP 里 | `video/video_receive_stream2.cc` 接收，解码器是 `VideoDecoder`。编码帧先经过下面的视频 jitter buffer，再进解码 |
| 渲染（音频） | AudioQueue output，设备回调来拉 PCM | 同一套 `AudioDeviceModule` 的播放方向。设备时钟就是后面 A/V 同步里的那只主钟 |
| 渲染（视频） | `NSWindow` + `NSImageView`，按视频自己的 `relative_pts_ns` 刷新。没有音频主钟 | `video/render/` 把解码后的帧交给渲染 sink。什么时候该画，要和同步模块一起看，不是来一帧画一帧 |

学习路径是五次独立进程、用 `out/` 里的文件接力。libwebrtc 是一条会同时跑着的发送/接收管线。对照的是**模块边界**，不是文件格式。

## 旁路

位置的叙述以 [pipeline-boundaries.md](./pipeline-boundaries.md) 为准。这里只点名栈里的对应物。

| 旁路 | 管线位置 | libwebrtc |
| --- | --- | --- |
| AEC | 采集后、编码前；参考信号来自播放 | `modules/audio_processing/aec3/echo_canceller3.cc` 的 `EchoCanceller3`。远端参考是正在播放的渲染音频，近端是麦克风 |
| 弱网码率自适应 | 网络反馈改编码器目标 | `modules/congestion_controller/goog_cc/`（`goog_cc_network_control.cc`，以及 delay-based / loss-based 估计）。目标码率进入 `video/video_stream_encoder.cc`。音频侧 `modules/audio_coding/g3doc/index.md` 写了 audio network adaptor：在带宽和丢包变化时调整 Opus |
| jitter buffer | 接收后、解码前 | 音频是 `NetEq`（统计里直接有 jitter buffer delay）。视频是 `video/video_stream_buffer_controller.cc`，时序估计在 `modules/video_coding/timing/`（`timing.h` 的 `VCMTiming`，`jitter_estimator.cc`）。这不是渲染端那几个输出 buffer |
| A/V 同步 | 接收/播放；音频设备时钟为主钟 | `video/rtp_streams_synchronizer2.cc` 驱动 `video/stream_synchronization.cc`。后者用 RTP 时间戳换算到 NTP，比较音画采集时刻和到达时刻，再增减两侧延迟 |

学习实验里没有这四件的实现。固定 64 kbps / 2 Mbps、AudioQueue 的 3×960 帧、视频渲染的 200 ms 夹断、以及本机 UDP 环回，都只是五段实验自己的机制。

## 和两种信令、SFU 的关系

libwebrtc 的 `pc/` 做会话描述和 ICE 这一类**媒体面**协商。把它当成 Media Signaling 的一种客户端实现来读，不要把它读成 Host App 的邀请/振铃/挂断。本仓库的 Media Signaling 是 Client AV SDK 和 SFU 之间的控制；承载通道未决，所以本篇不说「必须用 SDP over 某条 WebSocket」。

SFU 收到的是已经打包的媒体流，选择性转发给订阅者。它对应的是学习路径里「本地传输/打包」在真实网络上的那一侧，而不是采集、解码或渲染。libwebrtc 这份客户端树不包含 SFU。

## 本篇不证明什么

- 不证明本仓库的 VAU、长度前缀或 1200 字节分片等于 RTP。
- 不证明产品必须采用 `main@{#49134}`，也不关闭 U4 / U9。
- 不证明有人改过、编译过或跑通 libwebrtc。那不是毕业条件。
