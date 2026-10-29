# Learning audio decode uses AudioConverter on Opus packets

Learning Project 音频「解码」阶段在 macOS 上用 **AudioToolbox**：`AudioFileReadPacketData` 从 Opus CAF 读出 access unit（含 magic cookie），经 **`AudioConverter`** 按包还原为采集契约 **PCM**（48 kHz / mono / s16le），写入 `out/audio/decode/decode.wav`。不用 `ExtAudioFile` 整文件转换，不引入 libopus。理由：与 ADR-0002 一样走系统 API、无第三方媒体库；按包解码才能看见与 **Transport Unit** 载荷相同的 access unit，而不是把容器转码藏进一次文件调用。不重跑 UDP（环回一致性已由 issue 04 证明）。不按 CAF priming 把时长修到与原始 PCM 等长。jitter buffer、丢包与 PLC 不在本阶段。
