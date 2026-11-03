# Mission: 视频采集——从摄像头帧到可追踪的时间序列

## Why

你是做 C++ Client AV SDK 的工程师，音频五段实验已经走完，接下来要进入 Learning Project 的「视频·采集」（issue 07）。目标是能在写 AVFoundation 之前讲清：摄像头帧在内存里是什么、PTS 如何保留时间、callback 为什么不能阻塞，以及什么证据能证明这一阶段真的跑通。

## Success looks like

- 能区分 Raw Video Frame、压缩码流、文件容器和 Transport Unit，不把相邻阶段混成同一层
- 能算出 tight 1280×720 8-bit NV12 的 Y、UV 与总字节数，并解释 row stride 不能当 visible width
- 能区分 frame index、source PTS、duration 与 callback arrival，并读懂 issue 07 的 PTS manifest
- 能画出 `AVCaptureSession → AVCaptureVideoDataOutput → CMSampleBuffer → CVPixelBuffer → tight .nv12 / preview / manifest` 的数据流
- 能说明权限、默认摄像头、格式/尺寸协商、late-frame drop 与本阶段失败边界
- 能指出 A/V 同步、颜色转换和视频编码在真实管线里的位置，同时说清 07 不证明其中哪些内容

## Constraints

- 绑定 Learning Project 的视频采集、issue 07 与 ADR-0006；格式细节以工程课记为 SSOT，教室不另立一套契约
- 中文讲解；保留 Raw Video Frame、NV12、PTS、AVFoundation、CVPixelBuffer 等英文术语
- 知识来自 Apple / Microsoft / ITU 等高可信一手资料；短课与可复用速查优先
- 用 retrieval practice 验证理解；只接触不等于掌握，不凭已完成课程写 learning record
- 真实采集烟测：本机可有默认摄像头时按 `docs/learning/video/capture.md` 验收；无摄像头环境仍以 Catch2 / `check-learning.sh` 护写盘契约

## Out of scope（本轮）

- 视频编码、传输/打包、解码、渲染四段的实现选型
- H.264 / VP8 / VP9 的码流深入拆解
- 完整颜色矩阵推导、色彩管理专题
- A/V 同步算法、jitter buffer、AEC 与弱网自适应动手
- 屏幕共享、实时 GUI 预览、双端互通与 SFU
