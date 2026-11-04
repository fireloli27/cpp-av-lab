# 13: WebRTC 对照短文

**What to build:** 至少一篇 WebRTC / libwebrtc 对照短文：将 Learning Pipeline Stages 与叙述必会旁路映射到栈内对应概念。满足 Learning Exit Criteria 与产品假设 H8 的学习侧硬要求。不要求改得动 WebRTC。对照所针对的发行版/文档基线在短文中写明即可（属材料选型，非阻毕业）。

**Blocked by:** 12 管线旁路与边界叙述

**Status:** done

## Acceptance

- [x] `docs/learning/webrtc-crosswalk.md` 覆盖五段的模块映射（音、视分别点到 libwebrtc 的采集、编码、RTP 打包、解码、渲染）
- [x] 对照中纳入 AEC（`EchoCanceller3`）、弱网码率自适应（GoogCC / video stream encoder）、jitter buffer（`NetEq` 与视频 `video_stream_buffer_controller`）、A/V 同步（`stream_synchronization`）
- [x] 写明材料基线：libwebrtc `refs/heads/main`，查阅日 2026-11-03，`main@{#49134}`（Change-Id `I4fe360e90a4ed5c777ed3e05a82fb0b43e2b820d`）
- [x] 明确「改 WebRTC」不是本票验收条件；本仓库不 vendoring 该树
- [x] 写明学习实验的长度前缀 / UDP 分片不是 RTP；`RtpPacketizerH264` 的 FU-A 只是概念对应
- [x] 不把 `pc/` 的会话协商说成 Call Signaling，也不替 Media Signaling 的承载通道拍板；点明 SFU 不在这棵客户端树里

## Comments

- 2026-11-03: 材料基线取查阅当日的 `main@{#49134}`，只服务这篇对照。spec 未决 #14 的学习材料部分因此收口；产品假设 U4 / U9 仍开。
