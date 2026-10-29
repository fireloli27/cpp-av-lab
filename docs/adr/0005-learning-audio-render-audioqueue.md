# Learning audio render uses AudioQueue output

Learning Project 音频「渲染」阶段在 macOS 上用 **AudioToolbox** 的 **AudioQueue output**（`AudioQueueNewOutput`）：读 05 产出的 48 kHz / mono / s16le PCM WAV，由设备回调拉取、逐块填 buffer（3 × 960 帧），经系统默认输出设备播放。不用 `afplay` / `AVAudioEngine`，不直接上 AudioUnit，不引入第三方媒体库。理由：与采集的 AudioQueue input（ADR-0001）对称；C API 能直接看见渲染的 pull 模型与 buffer 排队延迟；AudioUnit 的实时线程约束超出本阶段学习目标，`afplay` / 高层播放器则藏掉了 buffer。成功判据是 `pcm_frames == frames_rendered`（交给设备的帧数），真实出声靠人耳；Catch2 不打开真实设备。jitter buffer、A/V 同步、AEC 只做叙述，不在本阶段实现。
