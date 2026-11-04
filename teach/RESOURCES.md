# 音频管线教室 Resources

## Knowledge

### 编码（上游，课 0002–0003）

- [RFC 6716: Definition of the Opus Audio Codec](https://www.rfc-editor.org/rfc/rfc6716.html)
  IETF 标准：交互式语音/音频编解码器。Use for: 「Opus 是什么」的一手定义。
- [RFC 6716 §2.1.1 Bitrate](https://www.rfc-editor.org/rfc/rfc6716.html#section-2.1.1)
  码率控制与 sweet spots。Use for: 「64 kbps」是压缩预算。
- [Apple: Core Audio Format (CAF) Overview](https://developer.apple.com/library/archive/documentation/MusicAudio/Reference/CAFSpec/CAF_overview/CAF_overview.html)
  CAF 是容器。Use for: 分清盒子与里面的 Opus 包。
- [Apple: AudioConverter](https://developer.apple.com/documentation/audiotoolbox/audioconverter-swift.struct)
  系统 PCM ↔ 压缩格式。Use for: 仓库为何用 AudioConverter。
- Repo: `docs/learning/audio/encode.notes.md` + ADR-0002
  编码契约。Use for: 课内数字；教室不另写一套。

### 本地传输/打包（课 0004+）

- [RFC 768: User Datagram Protocol](https://www.rfc-editor.org/rfc/rfc768.html)
  UDP 一手定义（无连接、数据报）。Use for: 环回实验为何选 UDP、它保证什么/不保证什么。
- [Apple: Audio File Services — AudioFileReadPacketData](https://developer.apple.com/documentation/audiotoolbox/1512134-audiofilereadpacketdata)
  从音频文件按 packet 读出压缩载荷。Use for: 「从 CAF 取出 Opus access unit」的系统 API 名。
- [Beej's Guide to Network Programming — Datagram Sockets](https://beej.us/guide/bgnet/html/#datagram)
  `sendto` / `recvfrom` 可读讲解（非 IETF，但久经检验）。Use for: 单进程 loopback 收发直觉。
- Repo: `docs/learning/audio/local-transport.notes.md` + ADR-0003
  Transport Unit 契约与学习选型。Use for: length 前缀数字、路径、环回边界；**产品是否 RTP 仍未决**。
- Repo: `GLOSSARY.md`「Transport Unit」/「本地传输/打包」
  仓库术语。Use for: 课内用词不另造同义词。

### 视频（课 0006+）

- [Microsoft: Recommended 8-Bit YUV Formats for Video Rendering](https://learn.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering)
  YUV sampling、stride/pitch 与 NV12 布局的一手技术说明。Use for: 4:2:0 直觉、NV12 两 plane 几何、video-range 公式。
- [Apple: CVPixelBufferGetBytesPerRowOfPlane](https://developer.apple.com/documentation/corevideo/cvpixelbuffergetbytesperrowofpane(_:_:))
  Apple Core Video API：查询指定 plane 的实际分配行宽。Use for: 区分 width 与 row stride。
- [Apple: CVPixelBufferGetBaseAddressOfPlane](https://developer.apple.com/documentation/corevideo/cvpixelbuffergetbaseaddressofplane(_:_:))
  Apple Core Video API：查询指定 plane 的起始地址。Use for: 逐 plane 读取与 tight copy。
- [Apple: CMSampleBufferGetPresentationTimeStamp](https://developer.apple.com/documentation/coremedia/cmsamplebuffergetpresentationtimestamp(_:_:))
  Apple Core Media API：读取 sample buffer 中最早的 presentation timestamp；错误返回 `kCMTimeInvalid`。Use for: manifest `pts_ns` 的一手语义。
- [Apple: CMSampleBufferGetDuration](https://developer.apple.com/documentation/coremedia/cmsamplebuffergetduration(_:))
  Apple Core Media API：读取 sample buffer 总时长；不保证等于相邻 PTS 间隔。Use for: PTS / duration 边界。
- [Apple: CMTime](https://developer.apple.com/documentation/coremedia/cmtime)
  Core Media 的时间值结构。Use for: value / timescale 与纳秒归一化。
- [Apple: AVCaptureSession](https://developer.apple.com/documentation/avfoundation/avcapturesession)
  AVFoundation 采集会话，文档接口是 Objective-C 类。Use for: 为何调用点在 `.mm`，以及 `startRunning` 启动的是已提交配置的会话。
- [Apple: AVCaptureVideoDataOutput](https://developer.apple.com/documentation/avfoundation/avcapturevideodataoutput)
  AVFoundation 视频帧输出。Use for: delegate queue、late frame 丢弃与 callback 边界。
- SDK: `CoreVideo/CVPixelBuffer.h`（用 `xcrun --sdk macosx --show-sdk-path` 定位）
  Apple SDK 内的 `420v` 常量、plane 描述与 video-range 注释。Use for: macOS 精确像素格式取值。
- Repo: `docs/adr/0006-learning-video-capture-avfoundation-nv12.md` + issue 07
  Learning 视频采集选型与契约。Use for: 720p、tight NV12、PTS manifest、硬件验收边界。

## Wisdom (Communities)

- [Hydrogenaudio Forums](https://hydrogenaud.io/index.php)
  编码器/容器边界讨论较密。Use for: 「文件能播」vs「网上传的是裸 packet」。
- 可选：组内对照 WebRTC 文档里 Opus payload / RTP —— 实战检验「学习 Transport Unit」vs「产品 RTP」。

- [Apple Developer Forums](https://developer.apple.com/forums/)
  Apple API 官方社区；回答质量仍应回到 SDK / 文档核验。Use for: Core Video / AVFoundation 行为边界与版本回归。

## Gaps

- WebRTC 对照短文已写：`docs/learning/webrtc-crosswalk.md`（libwebrtc `main`，查阅日 2026-11-03）。教室仍不把 RTP 深挖或改 libwebrtc 当课。
- jitter buffer / 弱网自适应动手实验属加分包，本教室只预告位置。
- A/V 同步的跨 clock 实作与颜色矩阵转换尚未开课；PTS、duration 与 callback queue 已由课 0007 建立边界。语言缝（C++ 契约 / `.mm` 会话）由课 0008 建立，尚未经检索证实。
