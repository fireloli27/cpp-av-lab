# Mission: 音频「本地传输/打包」——码流单元 → Transport Unit

## Why

你是 C++ Client AV SDK 学习者：issue 04 已把 `encode.caf` 里的 Opus access unit 打成长度前缀 **Transport Unit**，并在本机 UDP loopback 上校验收发一致。要能**自己讲清**本阶段在 Learning Pipeline 里干什么、为何「打包」≠ 编码侧的「分包」、以及学习选型为何**还不等于**产品必须 RTP。

## Success looks like

- 能用自己的话区分 **frames_per_packet**（编解码器吐帧）与 **Transport Unit**（环回发送单元）
- 能默写本票单元形状：`uint32` 大端 length + Opus 载荷（无 seq / timestamp / SSRC）
- 能说明单进程 `127.0.0.1` 临时端口环回在证明什么（逐包载荷一致），以及它**不**证明 ICE/TURN/弱网
- 能指着课记说：jitter buffer 通常在接收后、解码前——本课只预告、不实现

## Constraints

- 绑在 Learning Project / issue 04；短课 + 速查；契约数字只链 `docs/learning/audio/local-transport.notes.md`
- 中文讲解；Transport Unit / UDP / Opus / CAF / AudioFile 等保留英文
- 知识须能指向 `RESOURCES.md` 高可信来源（RFC / Apple / 仓库 ADR）

## Out of scope（本轮）

- 产品是否必须 RTP（spec 未决 #5 / U10）；WebRTC RTP 深挖留给对照短文
- ICE / TURN / SRTP、双进程、真实弱网、jitter buffer 动手
- 解码 / PCM 写出（issue 05）、重讲如何跑 `learning_audio_local_transport`
