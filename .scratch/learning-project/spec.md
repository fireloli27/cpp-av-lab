Status: ready-for-agent

# Spec: Learning Project

## Problem Statement

我是 C++ 客户端 SDK 工程师，日常做 IM App 集成，音视频基础薄弱。最终想做跨平台 Client AV SDK + Media Server（SFU），但若直接开正式工程，会在术语、管线边界和验收标准不清的情况下硬闯。需要先有一个以文档为先的 C++ 学习工程（macOS 宿主），把媒体管线学到「能讲清、能跑通关键阶段实验」，再启动正式工程。

## Solution

在本仓库（matt）内建立 Learning Project：按 Learning Pipeline Stages 做五段自下而上的可运行小实验，每段一篇短文；并完成至少一篇 WebRTC 对照短文（模块映射，硬要求）。毕业标准以「讲清管线 + 五段实验短文 + WebRTC 对照短文」为准；双端互通、SFU 动手、弱网/AEC/抖动缓冲/A/V 同步调参等为加分包。产品愿景以假设形式另册记录（见同目录 `product-hypotheses.md`），学习阶段用实验验证其中可验证者，未决事项不在本 spec 内拍板。

## User Stories

1. As a Learning Project 作者, I want 一份与正式产品愿景对齐的术语表, so that 我写短文和实验时不混用「信令 / SDK / SFU」等词
2. As a Learning Project 作者, I want 明确的五段硬范围清单, so that 我知道做什么算进度、什么算加分
3. As a Learning Project 作者, I want 每段至少一个可在 macOS 上自行跑通的 C++ 小实验, so that 概念不是纸上谈兵
4. As a Learning Project 作者, I want 每段对应一篇短文, so that 正式工程开工时能直接复用文档资产
5. As a Learning Project 作者, I want 音频至少一条完整路径（采集→…→渲染）, so that 语音链路我能从头讲到尾
6. As a Learning Project 作者, I want 视频至少一条完整路径（采集→…→渲染）, so that 视频链路我能从头讲到尾
7. As a Learning Project 作者, I want 「本地传输/打包」可用本机环回完成, so that 我不被 ICE/TURN 卡住毕业
8. As a Learning Project 作者, I want 在讲清管线时能点名回声消除（AEC）的位置与作用, so that 我知道它不是五段之一、但属于真实通话必谈模块
9. As a Learning Project 作者, I want 在讲清管线时能点名弱网码率自适应的位置与作用, so that 我知道拥塞/码率反馈卡在哪一段
10. As a Learning Project 作者, I want 在讲清管线时能点名 jitter buffer（抖动缓冲）的位置与作用, so that 我理解接收侧如何吸收网络抖动
11. As a Learning Project 作者, I want 在讲清管线时能点名 A/V 同步的位置与作用, so that 我理解音画对齐卡在接收/播放哪一带
12. As a Learning Project 作者, I want AEC / 弱网自适应 / jitter buffer / A/V 同步的动手实验都被标成加分包, so that 硬毕业不被高级专题绑架
13. As a Learning Project 作者, I want 前段用小库/系统 API 自搭管道, so that 我建立模块级直觉而不是只会调大框架
14. As a Learning Project 作者, I want 至少一篇 WebRTC 对照短文（五段及关键旁路模块的映射）作为硬毕业条件, so that 学习和产品 Stack Posture 不断档
15. As a Learning Project 作者, I want 「改得动 WebRTC」明确不作为毕业条件, so that 巨石工程不会无限延期
16. As a Learning Project 作者, I want 能说明 Call Signaling 与 Media Signaling 的分工, so that 我不会把 IM 呼叫信令和 SDK↔SFU 媒体信令混成一种「信令」
17. As a Learning Project 作者, I want 能说明 Client AV SDK 与 SFU 各自覆盖管线的哪一段, so that 正式工程模块边界有语言
18. As a Learning Project 作者, I want 文档与实验早期都落在 matt 单体仓库, so that 启动摩擦最低
19. As a Learning Project 作者, I want 仓库变臃肿时再拆代码仓的选项被保留, so that 结构决策可逆
20. As a Learning Project 作者, I want 时间投入按约每周 10–15 小时、约 4–8 周作为窗口假设, so that 有收官压力但不写死排期清单
21. As a Learning Project 作者, I want 双端互通列为加分退出条件而非硬门槛, so that 「两人能通话」不会变成唯一毕业定义
22. As a Learning Project 作者, I want SFU 概念实验或精读验证列为加分包, so that 多方转发可以学、但不挡硬毕业
23. As a Learning Project 作者, I want 网络真实感（抖动/丢包等）列为加分包, so that 硬范围保持可完成
24. As a Learning Project 作者, I want 「打包」是否等于 RTP 打包被显式列为未决, so that 阶段 3 实验选型前不被偷偷锁死
25. As a Learning Project 作者, I want 产品假设单独成册并标注「待哪个实验验证」, so that 学习实验和产品拍板不会搅在一起
26. As a Learning Project 作者, I want 未决事项单独列出且不被代为决定, so that 我保留关键选择权
27. As a future 正式工程参与者, I want 学习短文使用 GLOSSARY 词汇, so that 正式仓与学习仓语言一致
28. As a future Host App 集成者（假设读者）, I want 文档写清 SDK 不负责 Call Signaling, so that 集成边界从学习阶段就正确
29. As a Learning Project 作者, I want 每个阶段实验有可观察的外部结果（文件、窗口、波形、日志摘要等）, so that 「跑通」可验收而不靠读心
30. As a Learning Project 作者, I want 实验失败时短文仍能记录失败与学到的边界, so that 文档优先原则在卡住时仍然成立
31. As a Learning Project 作者, I want 加分包完成与否不影响「达到 Learning Exit Criteria」的判定, so that 收官标准单一
32. As a Learning Project 作者, I want 采集/编码/解码/渲染所选用的具体库保持可替换叙述, so that 实现选型未决时文档仍可写
33. As an agent 协助者, I want 本 spec 使用 Status: ready-for-agent, so that 后续可按票拆解而不再重新 grilling 愿景

## Implementation Decisions

- **范围**：本 spec 只覆盖 Learning Project（文档 + 五段实验 + 毕业标准）。正式 Client AV SDK / Media Server 产品实现不在本 spec 交付范围内；产品侧仅以假设册约束方向。
- **仓库形态**：早期单体（matt）承载短文与实验代码；是否拆仓延后，见未决事项。
- **宿主**：学习与调试宿主为 macOS；产品 Target Platforms（Android/iOS）不要求在本学习 spec 内交付绑定层。
- **Learning Pipeline Stages（硬范围）**：
  1. 采集
  2. 编码
  3. 本地传输/打包（本机环回即可；不要求生产级 ICE/TURN）
  4. 解码
  5. 渲染  
  音、视至少各一条完整路径；每段：≥1 自跑通实验 + 1 短文。  
  **「打包」含义未定**：是否指 RTP 打包（或其他分帧/容器/自定义包格式）见未决事项，本 spec 不代为决定；阶段 3 在未决拍板前只要求「把编码后的数据组织成可在本机环回发送/接收的单元」。
- **学习姿势**：混合——前段自下而上小库/API；后段做 WebRTC 对照。
- **WebRTC 对照（硬要求，与 H8 对齐）**：毕业必须有 ≥1 篇对照短文，将五段及叙述必会的旁路模块映射到 WebRTC / libwebrtc（或等价完整栈）中的对应概念。**「改得动 WebRTC」不是毕业条件**，也不算加分门槛。
- **叙述必会、硬实验不加（同档）**：AEC、弱网码率自适应、jitter buffer、A/V 同步——毕业叙述必须说明其在管线中的位置与作用；不要求单独硬实验或调通；动手验证属加分包。
- **信令概念（学习文档必写清）**：Call Signaling 归 Host App；Media Signaling 为 SDK↔SFU；二者分离。Media Signaling 的承载通道不在本 spec 决定。
- **主测试缝（已决，issue 01；测试护栏经 retro 修订）**：以「阶段实验」为产品主缝——每一 Learning Pipeline Stage 提供一个可从仓库外部运行的实验入口，输入/前置条件写在短文里，成功时有可观察输出；**人工按短文验收**仍为一等公民。另以 **Catch2 + CTest** 护住可自动化的可观察契约（见 Testing Decisions）。WebRTC 对照以短文叙述验收，不另开代码缝。不做统一 pipeline runner。
- **文档与代码布局（已决，issue 01）**：
  - 短文根：`docs/learning/`；模板：`docs/learning/_template.md`（必填：目标、前置、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系）。正文中文；路径/命令/目标名/GLOSSARY 术语不硬翻。
  - **课记**（已决，retro 2026-10-20）：模板 `docs/learning/_notes_template.md`；路径 `docs/learning/{audio|video}/{stage}.notes.md`，与短文成对。短文＝跑通验收；课记＝概念与代码讲解。Learning Pipeline Stage 票 `/implement` 收官必写课记（六栏）；harness 占位不要求。
  - 阶段短文/实验/课记镜像：`docs/learning/{audio|video}/{capture,encode,local-transport,decode,render}.md`（+ 同名 `.notes.md`）↔ `experiments/{audio|video}/{同名}/`。
  - CMake 根：`experiments/`（CMake ≥ 3.20，C++17）；构建目录 `experiments/build/`；可观察产物写在仓库根下 `out/`；运行约定 cwd＝仓库根。仓库根 `.gitignore` 忽略 `build/`、`experiments/build/`、`out/`、`.DS_Store`。
  - harness 地基（非管线阶段）：`experiments/harness/placeholder/`（目标名 `learning_placeholder`）+ `docs/learning/harness/` 总览与样例短文。01 不预建空的音/视阶段目录；不抽跨阶段 `experiments/common`；占位可抽本目录内小静态库供可执行文件与测试链接。
  - 自动化测试：`experiments/tests/`（Catch2 经 FetchContent）；仓库护栏命令：`scripts/check-learning.sh`（configure + build + ctest）。
- **文档与代码关系**：短文是一等公民；实验为短文服务。演示优先或半吊子产品化不得压过文档。
- **不写排期清单**：时间窗口仅作假设（约每周 10–15h，约 4–8 周），本 spec 不展开周序任务表。

## Testing Decisions

- **好测试的定义（对本学习工程）**：只验收外部可观察行为与文档承诺，不验收某库内部私有结构是否「优雅」。
- **主缝验收（人工）**：
  - 对每一硬范围阶段：实验可按短文步骤运行；成功判据在短文中写明且可人为确认。
  - 音、视路径：至少各有一条从采集到渲染的贯通证据（可分段拼接，但缺口处必须在文档标明）。
- **自动化护栏（已决，retro 2026-10-20）**：
  - 框架：**Catch2**（CMake FetchContent）+ **CTest**；入口 `./scripts/check-learning.sh`。
  - 测什么：可在无硬件/无权限环境下复现的可观察契约（例如 harness 占位写出约定路径与内容）。麦克风/摄像头等需人工的成功判据仍以短文为准，不强行自动化。
  - 01 已覆盖：`write_placeholder_artifact` 在给定 root 下写出 `out/harness/placeholder-ok.txt`，内容为 `learning_placeholder: ok\n`（与样例短文判据一致）。
- **叙述验收（人工）**：作者能按五段讲解管线；安放 AEC、弱网码率自适应、jitter buffer、A/V 同步的位置与作用；说明 Client AV SDK、Call Signaling、Media Signaling、SFU 的职责边界；并完成 WebRTC 对照短文（硬）。
- **加分包**：双端互通、SFU 实验、网络真实感、AEC / jitter buffer / A/V 同步 / 弱网自适应动手——有则记，无则不影响 Learning Exit Criteria。
- **Prior art**：经 `/retro` 推翻「不引入测试框架」的早期基线；以 Catch2 + `scripts/check-learning.sh` 为新基线。

## Out of Scope

- 正式 Client AV SDK / SFU 产品实现与发布
- 排期清单、周任务甘特、里程碑日历细化
- 代为决定未决事项（见下节与 `product-hypotheses.md`）
- 大房间/直播、屏幕共享作为核心学习目标
- 自建 Call Signaling、从零重造完整 RTC
- 通话录制、美颜滤镜、IM 消息能力、E2EE 早期必达
- 将「两人能通话」或「自研 SFU」设为唯一毕业条件
- 生产级 ICE/TURN、私有化/公有云部署方案定稿
- Android/iOS 绑定与 Host App 真实集成（属正式工程）

## Further Notes

- 术语以根目录 `GLOSSARY.md` 为准；短文与假设册不得发明冲突同义词。
- 产品假设与验证钩子见 `.scratch/learning-project/product-hypotheses.md`。
- 叙述必会旁路（硬实验不加）：AEC、弱网码率自适应、jitter buffer、A/V 同步。
- WebRTC 对照 = **硬要求**（对照短文）；改 WebRTC ≠ 毕业条件。与 `product-hypotheses.md` H8 对齐。
- 阶段 3「打包」是否 = RTP：未决，不代定。

## 未决事项（本 spec 不代为决定）

1. Media Signaling 的承载通道（复用 IM / 独立 WebSocket 或 HTTP / 随媒体通道 / 其他）
2. Media Server（SFU）具体实现路线（自研比例 vs 开源改造选型）
3. 部署形态（私有化 / 公有云多租户 / 两者）
4. 五段实验各自的具体库与 API 选型（采集、编码、传输/打包、解码、渲染）——**部分**：音频·采集已选 Core Audio / AudioQueue（issue 02、ADR-0001）；音频·编码已选 AudioToolbox / `AudioConverter` + Opus → CAF（issue 03、ADR-0002）；音频·本地传输/打包已选 AudioToolbox 读包 + 长度前缀 Transport Unit + POSIX UDP loopback（issue 04、ADR-0003）；其余阶段仍未决
5. **阶段 3「打包」的具体含义（产品 / 是否必须 RTP）**：仍未决（U10）。**学习音频实验**已选自定义长度前缀（非 RTP）作为本机环回单元（issue 04、ADR-0003）；该选型**不**视为对本条的产品拍板
6. 音路径与视路径的先后顺序
7. ~~短文的目录约定与模板细则~~ → **已决（issue 01）**：见 Implementation Decisions「文档与代码布局」
8. ~~实验目录命名与构建系统选型~~ → **已决（issue 01）**：`experiments/` + CMake ≥ 3.20 / C++17；阶段 slug 见上
9. 何时从 matt 单体拆出专用代码仓的触发条件
10. ~~是否引入自动化测试框架 / one-command 验收~~ → **已决（retro 2026-10-20）**：Catch2 + CTest；`scripts/check-learning.sh`；见 Testing Decisions
11. ~~主测试缝是否改为统一 pipeline runner~~ → **已决（issue 01）**：维持每阶段各自入口 + 人工按短文验收（自动化护栏不替代人工短文缝）
12. 加分包的触发条件与是否在收官后另开学习专项
13. 时间窗口在「约 4–8 周」内的实际取舍与起止日期
14. WebRTC 对照所针对的具体发行版 / 文档基线（在「对照短文」硬要求之下的材料选型）

## Comments

- 2026-10-20: grilling + 对照表定稿后由 `/to-spec` 发布；Status = ready-for-agent。
- 2026-10-20: 修订——jitter buffer / A/V 同步同 AEC 档；WebRTC 对照定为硬要求并与 H8 对齐；「打包」是否 RTP 列入未决。
- 2026-10-20: `/grill-me` on issue 01——收口原未决 7/8/10/11（目录/模板、CMake+experiments 骨架、当时不引入测试框架、确认 per-stage 主缝）；细节见 `issues/01-learning-harness.md`。
- 2026-10-20: `/retro`——文件改名为 `spec.md`（对齐 issue-tracker）；推翻「不引入测试框架」，引入 Catch2 + `scripts/check-learning.sh`，并补 01 占位产物测试。
- 2026-10-20: `/grill-with-docs` on issue 02——音频·采集选定 Core Audio / AudioQueue、PCM/WAV 契约与验收边界；未决 #4 仅部分收口；见 `issues/02-audio-capture.md` 与 `docs/adr/0001-learning-audio-capture-coreaudio.md`。
- 2026-10-20: `/retro`（issue 02 会话）——护栏接入 pre-commit + GitHub Actions；FetchContent 缓存与增量 build；`.cursorignore`；harness/短文写明系统框架与非沙箱麦烟测；tdd/implement/code-review 技能收紧 grilled seams 与 working-tree 审查。
- 2026-10-20: `/retro`——Learning 阶段票收官必写课记 `{stage}.notes.md`（模板 `_notes_template.md`）；样例 `docs/learning/audio/capture.notes.md`；`/implement` 与 harness/AGENTS 已接线。
- 2026-10-20: 文档分层瘦身——短文只验收；课记为阶段契约 SSOT；`lessons/`+`reference/` 属 `/teach`，链课记不另写契约。
- 2026-10-20: `/teach` 教室迁入 `teach/`（与 `docs/` 分离）；见 `teach/README.md`。
- 2026-10-20: 文档语言钉死——正文中文、术语/路径/命令英文；写入 `AGENTS.md` Doc language 与 `teach/README.md`。
- 2026-10-22: `/grill-with-docs` on issue 03——音频·编码选定 Opus + `AudioConverter`、CAF 产物与验收边界；未决 #4 再部分收口；见 `issues/03-audio-encode.md` 与 `docs/adr/0002-learning-audio-encode-opus-audioconverter.md`；Glossary「编码」区分 PCM 与压缩码流。
- 2026-10-22: `/grill-with-docs` on issue 04——音频·本地传输/打包选定长度前缀 Transport Unit + UDP loopback（非产品 RTP 拍板）；未决 #4 再部分收口；未决 #5 产品侧仍开、学习音频实验选型见 ADR-0003；Glossary 增补 Transport Unit；见 `issues/04-audio-local-transport.md`。
