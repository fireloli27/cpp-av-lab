# Client AV Platform

跨平台客户端音视频 SDK 与媒体服务器，供 IM / 社交类 App 嵌入集成。当前阶段以学习工程沉淀概念与文档为主。

## Language

**Client AV SDK**:
嵌入业务 App 的跨平台客户端音视频库，负责采集、编解码、传输与通话会话侧能力；不做成面向终端用户的独立通话 App。
_Avoid_: 通话 App, 独立产品, RTC App

**Media Server**:
与 Client AV SDK 配套的服务端媒体组件；愿景假设为 SFU（选择性转发）：基本不做解码混画，将各路媒体流转发给订阅者。不兼任 Call Signaling。
_Avoid_: MCU（混流服务器）, 业务服务器, IM 服务器, 信令服务器

**Host App**:
集成 Client AV SDK 的业务应用（典型为 IM / 社交 App）。
_Avoid_: 客户, 用户端（易与终端用户混淆）

**Call Signaling**:
呼叫生命周期与房间成员变更的业务信令（邀请、振铃、接听、拒绝、挂断、进房、离房等）。由 Host App / 既有 IM 通道主导；Client AV SDK 消费已建立的通话/房间上下文，不自建这套信令通道。
_Avoid_: Media Signaling, 把 SDP/ICE 等媒体协商塞进 IM 业务信令并不加区分

**Media Signaling**:
Client AV SDK 与 Media Server（SFU）之间的媒体面协商与控制信令（如媒体描述、传输候选、订阅/转发控制等）。与 Call Signaling 分离；承载通道（复用 IM、独立 WebSocket/HTTP、或随媒体通道等）有意待定。
_Avoid_: Call Signaling, 假定「信令」只有一种

**Call**:
一次实时音视频通话会话，涵盖 1v1 与小群多方；产品愿景以 IM 场景下的 1v1 与小群（约 ≤9 人）为主，不以大房间/直播为默认目标。
_Avoid_: 直播, 大房间, Webinar（除非另开产品线）

**Target Platforms**:
产品愿景上 Client AV SDK 以 Android 与 iOS 为优先交付端（C++ 核心 + 各端绑定）；macOS 作为学习与调试宿主，不视为产品第一优先级。
_Avoid_: Web 优先, 全端对等（除非愿景升级）

**Call Media**:
一次 Call 的默认媒体为麦克风音频与摄像头视频；屏幕共享不作为当前愿景的核心能力。
_Avoid_: 默认含屏幕共享, 纯音频产品（语音可作为学习路径，但不是产品默认边界）

**Stack Posture**:
产品假设为「能力自研、协议可复用成熟栈」：Client AV SDK 可基于 WebRTC / libwebrtc 等裁剪封装；Media Server（SFU）可基于开源改造或自研；对外仍呈现为自有 SDK + 自有 Media Server，而非薄封装第三方 RTC 云。部署形态（私有化 / 公有云多租户）有意延后，不作为当前愿景约束。
_Avoid_: 从零重造完整 RTC 栈（可作为学习专题）, 纯云 RTC 转售

**Out of Scope (vision default)**:
默认愿景不包含：大房间/直播、屏幕共享作为核心能力、自建 Call Signaling、从零重造完整 RTC、通话录制回放、美颜滤镜、IM 消息能力本身、以及将端到端加密（E2EE）列为早期必达。
_Avoid_: 把上述能力当作「顺便做掉」

**Learning Project**:
正式工程之前的 C++ 音视频学习仓（macOS 宿主）。主目标是沉淀可复用的概念与文档；可演示的最小闭环是加分项，但不得以牺牲文档质量为代价去追「半吊子产品」。早期以本仓库（matt）单体承载文档与实验代码，臃肿后再拆专用代码仓。
_Avoid_: 正式 SDK, 产品仓, 演示优先的玩具产品

**Learning Pipeline Stages**:
学习工程硬范围的五个关键阶段（音、视至少各覆盖一条完整路径）：
1. **采集**：麦克风取到原始 PCM 样本 / 摄像头取到原始帧
2. **编码**：音频侧将 **PCM 样本**压成**压缩码流**；视频侧将原始帧压成压缩码流。码流不是 WAV 容器里的 PCM。
3. **本地传输/打包**：将编码后数据组织成可在本机环回发送/接收的单元（如 UDP 或文件环回），不要求生产级 ICE/TURN；「打包」是否必须为 RTP 有意未决
4. **解码**：码流还原为可播放帧
5. **渲染**：扬声器播放 / 画面显示
_Avoid_: 把回声消除、弱网自适应、jitter buffer、A/V 同步当成这五段之一；在未决拍板前把「打包」偷换成「必须 RTP」；把 PCM-in-WAV 或未压缩样本称作「码流」

**Learning Exit Criteria**:
学习工程收官标准：能按 Learning Pipeline Stages 讲清整条管线，并说明 Client AV SDK、Call Signaling、Media Signaling、SFU 各覆盖哪一段；五段每段至少一个自跑通实验 + 短文；另有 ≥1 篇 WebRTC 对照短文（模块映射，硬要求）。讲清管线时须能说明 AEC、弱网码率自适应、jitter buffer、A/V 同步的位置与作用，但不要求单独硬实验或调通。网络真实感、SFU 实验、双端互通、以及上述旁路的动手调参为加分包。时间窗口约每周 10–15 小时、约 4–8 周。学习姿势为混合：前段自下而上用小库/API；后段 WebRTC 对照为硬毕业条件；不以改得动 WebRTC 为毕业条件。
_Avoid_: 以「两人能通话」或「自研 SFU」作为唯一毕业条件；把加分包当成硬门槛；把 WebRTC 对照降成可选项或把「改 WebRTC」升成硬门槛；纯巨石摸索或与产品栈完全脱节的玩具管线
