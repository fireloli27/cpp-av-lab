# Learning video decode uses VTDecompressionSession

Learning Project 视频「解码」阶段在 macOS 上用 **VideoToolbox / `VTDecompressionSession`**，按 access unit 把 `encode.vau` 里的 H.264 码流还原成与采集相同的 tight NV12 `420v`（1280×720）。参数集来自 VAU 文件头，经 `CMVideoFormatDescriptionCreateFromH264ParameterSets` 建成 format description；每个 access unit 单独做成 `CMSampleBuffer` 再送进解码器。不使用 `AVAssetReader` 整文件解码，不引入 FFmpeg。理由与 ADR-0004 一样：按包解码才能看见和 Transport Unit 载荷相同的 access unit，而不是把容器藏进一次文件调用。

不重跑 UDP。环回逐字节一致已由 issue 09 证明。解码输出不要求与编码前像素逐字节相同（有损）。请求软件解码器，避免 CI 上没有硬件解码器时失败。jitter buffer 与丢包隐藏不在本阶段。
