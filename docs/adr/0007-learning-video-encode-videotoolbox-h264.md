# Learning video encode uses VideoToolbox H.264

Learning Project 视频「编码」阶段在 macOS 上选用系统 **VideoToolbox / `VTCompressionSession`**，把采集契约的 tight NV12 `420v`（1280×720）压成 **H.264 Baseline** access unit，写入学习容器 `out/video/encode/encode.vau`。不引入 FFmpeg、x264 或其他第三方媒体库。理由：与 ADR-0001～0006 的系统 API 姿势一致；H.264 是 WebRTC 视频主线之一，Baseline 且关闭 `AllowFrameReordering` 可以避开 B 帧，让本阶段看见「一帧进、一个 access unit 出」。请求软件编码器（`EnableHardwareAcceleratedVideoEncoder = false`），这样 Baseline 不依赖本机硬件编码器是否在位。码率学习实验固定 2 Mbps，GOP 最长 30 帧；不做弱网码率自适应。

容器不用 MP4/MOV：那些容器会把 access unit 藏进 sample table。VAU 只按大端字段记下参数集（SPS/PPS）和每帧 AVCC 载荷与 PTS，方便下一阶段把**载荷**当成 Transport Unit，也方便解码重建 `CMVideoFormatDescription`。VAU 是学习容器，不是产品容器选型。产品 Target Platforms 绑定与「打包是否必须 RTP」仍未决。
