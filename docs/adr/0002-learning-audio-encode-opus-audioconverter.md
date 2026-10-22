# Learning audio encode uses AudioConverter (Opus)

Learning Project 音频「编码」阶段在 macOS 上选用系统 **AudioToolbox / `AudioConverter`**，将采集契约 PCM（48 kHz / mono / s16le）压成 **Opus** 码流并写入可观察 **CAF**（`out/audio/encode/encode.caf`）。不引入 libopus、FFmpeg 等第三方媒体库。理由：与 harness「实验默认无媒体第三方依赖」、采集票 ADR-0001 的系统 API 姿势一致；本机系统编码器已支持 Opus，可在不破政策的前提下贴近 RTC 语音主线。备选 AAC 同属系统能力但与 WebRTC 语音对照较远；引入 libopus 需改 harness 依赖政策，本票不取。码率学习实验固定 64 kbps；分包跟系统默认。产品 Target Platforms 绑定与其余管线阶段库选型仍未决。
