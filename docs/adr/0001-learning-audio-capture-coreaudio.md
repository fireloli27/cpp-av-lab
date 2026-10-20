# Learning audio capture uses Core Audio (AudioQueue)

Learning Project 音频「采集」阶段在 macOS 上选用系统 **Core Audio / AudioQueue**（经 AudioToolbox），不引入 PortAudio 等第三方媒体库。理由：与 harness「实验默认无媒体第三方依赖」、前段自下而上小 API 的学习姿势一致；本阶段目标是可观察 PCM/WAV 产物，AudioQueue 足够，不必为跨平台抽象提前付依赖成本。产品 Target Platforms（Android/iOS）绑定不在本学习票范围；其余管线阶段的库选型仍未决。
