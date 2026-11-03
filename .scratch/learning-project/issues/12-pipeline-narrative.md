# 12: 管线旁路与边界叙述

**What to build:** 毕业所需的叙述短文（无硬实验）：说明 AEC、弱网码率自适应、jitter buffer、A/V 同步在管线中的位置与作用；并写清 Call Signaling vs Media Signaling、Client AV SDK vs SFU 的职责边界。使用 GLOSSARY 词汇。

**Blocked by:** 11 视频·渲染

**Status:** done

## Acceptance

- [x] 短文 `docs/learning/pipeline-boundaries.md` 能安放 AEC（采集后、编码前，参考来自音频渲染）、弱网码率自适应（网络反馈作用于编码器）、jitter buffer（接收后、解码前）、A/V 同步（接收/播放一带，音频时钟为主钟）的位置与作用
- [x] 短文能区分 Call Signaling（Host App）与 Media Signaling（SDK↔SFU），且不替未决的承载通道拍板
- [x] 短文能说明 Client AV SDK 盖住客户端五段与旁路，SFU 转发已编码的流、基本不解码混画、不兼任 Call Signaling
- [x] 明确上述旁路均无本票硬实验；动手属加分包。并点明学习实验里的固定码率、输出缓冲、200 ms PTS 夹断、本机环回都不是这些旁路

## Comments

- 2026-11-03: 叙述落在 `docs/learning/pipeline-boundaries.md`，链到音、视两篇贯通说明。未决 #1（Media Signaling 承载）与未决 #5（是否必须 RTP）保持开放。
