# Learning video local-transport fragments one access unit over UDP

Learning Project 视频「本地传输/打包」阶段沿用音频 ADR-0003 的姿势：单进程 **POSIX UDP loopback**（`127.0.0.1`，临时端口），把编码后的 **access unit** 组织成可发送/接收的 **Transport Unit**。单元内容是 VAU 里的一帧 H.264 AVCC 载荷，不是整份 `encode.vau`，也不是参数集。

和音频不同的是：一帧视频经常大于一个 UDP datagram（最大约 65507 字节，学习实验还把单片载荷钉在 **1200** 字节）。因此同一个 Transport Unit 被拆成多个 datagram：大端 `au_length` + `frag_index` + `frag_count` + `au_index` + 分片字节，接收端按 `au_index` 重组后再逐字节比对。`au_index` 只为这次环回重组，**不是** RTP sequence number；头里没有 timestamp、没有 SSRC。

本 ADR 只锁定**学习实验选型**。它不收口 spec 未决 #5 / U10（产品「打包」是否必须 RTP），也不把「视频必须分片」写成产品协议。WebRTC 对照短文再映射 `RtpPacketizerH264` 的 FU-A。不做 ICE/TURN/SRTP，不实现 jitter buffer。
