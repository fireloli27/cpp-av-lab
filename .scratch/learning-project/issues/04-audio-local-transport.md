# 04: 音频·本地传输/打包

**What to build:** 音频 Learning Pipeline Stage「本地传输/打包」：从编码 CAF 读出 Opus access unit，打成长度前缀 **Transport Unit**，经本机 UDP loopback 发送/接收并校验一致性；写短文与课记。库与分帧已在本票 grilling 选定。不预设产品必须 RTP。

**Blocked by:** 03 音频·编码

**Status:** done

## Acceptance

- [x] `experiments/audio/local-transport/` 可执行目标 `learning_audio_local_transport`；目录内静态库（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 输入：默认读 `out/audio/encode/encode.caf`；可选 `--input`；用 macOS **AudioToolbox** 读出 Opus 包；无第三方媒体/RTP 库
- [x] **Transport Unit**：`uint32` 大端 length + Opus 载荷（length = 载荷字节数，不含这 4 字节）；无 seq / timestamp / SSRC
- [x] 环回：单进程 **POSIX UDP**、`127.0.0.1`（端口在课记写清；倾向 bind 临时端口再自发自收）；不要求双进程、ICE/TURN、SRTP
- [x] 成功：exit 0；日志含发送/接收单元数与载荷字节；**单元数相等且逐包载荷字节一致**；覆盖写出 `out/audio/local-transport/loopback-ok.txt`（cwd＝仓库根；具体内容格式以课记为 SSOT）
- [x] 失败（缺输入 / 非预期 CAF / 读包或套接字失败 / 收发不一致等）：stderr 简述 + exit ≠ 0
- [x] 本票**不**解码、不写 PCM；**不**定 05 的输入契约（可选落盘载荷序列不作为本票硬验收）
- [x] 短文 `docs/learning/audio/local-transport.md`（按 `_template.md`）：目标、前置、选用库/机制、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明本票单元格式 **≠** 对「必须 RTP」的产品拍板；jitter buffer **一两句**位置预告（接收后、解码前），不实现、不验收
- [x] 课记 `docs/learning/audio/local-transport.notes.md`（按 `_notes_template.md` 六栏）：概念（打包 vs 编码「分包」、Transport Unit）、代码地图、数据流、API/决策、踩坑、下一课钩子；契约数字/路径以课记为 SSOT
- [x] Catch2：组包/拆包（及可选无硬件 loopback 助手）；合成 Opus 载荷字节；不读真人 `encode.caf`、不提交二进制 fixture；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0003-learning-audio-local-transport-length-prefix-udp.md`；术语与 `GLOSSARY.md`「Transport Unit」/「本地传输/打包」一致

## 约定（grilling 2026-10-22）

- **路径**：`experiments/audio/local-transport/` ↔ `docs/learning/audio/local-transport.md` + `local-transport.notes.md`
- **产物**：固定路径覆盖写 `out/audio/local-transport/loopback-ok.txt`；听感不在本段验收
- **未决 #5 / U10**：本票学习选型 = 长度前缀 + UDP；**产品是否必须 RTP 仍未决**
- **自动化边界**：与 01–03 一致——可无硬件复现的契约进 Catch2；真 CAF→UDP 烟测按短文（非沙箱若需要）
- **教室**：`teach/` 中「04 = RTP」预告与本决定不一致 → `/implement` 收官或随后再改，不挡本票

## Comments

- 2026-10-22: `/grill-with-docs` 定稿 Opus access unit + 长度前缀 Transport Unit + 单进程 UDP loopback、成功判据、Catch2 边界、与 05 交接、jitter 预告、ADR-0003、Glossary「Transport Unit」；确认共享理解后落盘票面与 ADR。
- 2026-10-22: `/implement` 完成：`learning_audio_local_transport` + Catch2 组包/拆包/UDP 环回；短文/课记/ADR-0003；teach 去掉「04=RTP」硬预告。
