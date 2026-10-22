# Learning audio local-transport uses length-prefixed Transport Unit over UDP loopback

Learning Project 音频「本地传输/打包」阶段在本机用 **POSIX UDP loopback**（`127.0.0.1`）收发 **Transport Unit**：从上游 Opus-in-CAF 读出的 access unit，外加 **`uint32` 大端 length + 载荷** 自定义分帧；单进程自发自收。不引入第三方媒体/RTP 库，不做 ICE/TURN/SRTP，不实现 jitter buffer。理由：在未决「打包是否必须 RTP」（spec #5 / U10）之下仍能练发送/接收单元直觉；长度前缀比最小 DIY RTP 更不易被读成产品协议已定；WebRTC 对照短文再映射真实 RTP。本 ADR 只锁定**学习实验选型**，**不**收口产品是否必须 RTP。
