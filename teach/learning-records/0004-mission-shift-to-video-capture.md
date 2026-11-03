# Mission 从音频本地传输切换到视频采集

用户在完成视频课 0006（NV12 字节账）与 0007（PTS 时序）后，明确确认把学习 mission 从音频「本地传输/打包」切换到视频采集。这个决定把后续课程的 ZPD 锚在 Raw Video Frame、NV12、PTS 与 AVFoundation callback，而不是继续扩展 RTP 或音频旁路。

**Implications:** 下一课应把已有的帧布局与时间语义接到真实采集数据流、权限和 callback 边界。用户尚未回答 0006/0007 的检索题，因此本记录只说明 mission 变化，不声明这些概念已经掌握。
