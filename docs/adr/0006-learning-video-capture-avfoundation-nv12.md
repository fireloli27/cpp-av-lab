# Learning video capture uses AVFoundation and NV12 `420v`

Learning Project 视频「采集」阶段在 macOS 上选用系统 **AVFoundation** 的 `AVCaptureSession` + `AVCaptureVideoDataOutput`，把默认摄像头的 1280×720 视频帧请求为 **NV12 `kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange`**，并保存为 tight-packed 逐帧文件。理由：这与 harness「系统框架允许、无第三方媒体库」和前段自下而上学习 Sample Buffer / pixel buffer / PTS 的姿势一致；NV12 比 BGRA 少一半原始字节，也更接近后续视频编码器的输入。首帧预览交给 Core Image + ImageIO 完成 YUV 颜色转换，原始 `.nv12` 仍保留。

备选的 BGRA 更容易直接展示，但会把 YUV 子采样与颜色转换推给编码阶段；CoreMediaIO 更底层，却会把学习重点从帧时间与平面布局移到设备接管。采集固定 720p、目标 30 fps，并以帧数停止；硬件不支持该组合时明确失败，不自动降级。真实摄像头是本阶段最终人工验收条件，Catch2 只用合成 NV12 验证字节布局、manifest 与预览写盘契约。
