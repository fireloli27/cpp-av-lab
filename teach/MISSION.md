# Mission: 音频「编码」——PCM → 压缩码流

## Why

你是 C++ Client AV SDK 学习者：issue 03 已把 `capture.wav` 压成 `encode.caf`（Opus）。要能**自己讲清**「编码」在 Learning Pipeline 里干什么、为何 `encode.caf` 不是「又一个 WAV」，以及下游 04「打包」吃的是码流单元而不是 PCM。

## Success looks like

- 能用自己的话区分 **编解码器（Opus）** 与 **容器（CAF）**——对仗采集课的 PCM ≠ WAV
- 能口头对比：同 1 秒内容，契约 PCM 约 96 000 B，64 kbps 码流量级约 8 000 B（再加 CAF 头）
- 能指着课记说明：输入仍是 48 kHz mono s16le；本阶段不采麦、不做 RTP
- 知道弱网自适应会动**码率**，但本票实验是固定 64 kbps

## Constraints

- 绑在 Learning Project / issue 03；短课 + 速查；契约数字只链 `docs/learning/audio/encode.notes.md`
- 中文讲解；Opus / CAF / AudioConverter / bitrate 等保留英文
- 知识须能指向 `RESOURCES.md` 高可信来源（RFC / Apple CAF）

## Out of scope（本轮）

- Opus 内部 SILK/CELT 细节、自己实现编码器
- RTP / SRTP 打包（留给 04）
- AAC 选型对比深挖、AEC / 弱网动手调参
