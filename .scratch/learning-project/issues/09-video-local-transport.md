# 09: 视频·本地传输/打包

**What to build:** 视频 Learning Pipeline Stage「本地传输/打包」：从编码 VAU 读出 H.264 access unit，打成可重组的 Transport Unit，经本机 UDP loopback 发送/接收并校验一致性；写短文与课记。分片是因为视频单元可能超过一个 datagram。不预设产品必须 RTP。

**Blocked by:** 08 视频·编码

**Status:** done

## Acceptance

- [x] `experiments/video/local-transport/` 可执行目标 `learning_video_local_transport`；目录内静态库（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 输入：默认读 `out/video/encode/encode.vau`；可选 `--input`；用 `read_vau` 取出 access unit 载荷；参数集不单独发送；无第三方媒体/RTP 库
- [x] **Transport Unit**：一个 access unit。因可能超过一个 UDP datagram，同一单元拆成多个 datagram：大端 `uint32 au_length` + `uint16 frag_index` + `uint16 frag_count` + `uint32 au_index` + 分片字节；单片载荷最多 **1200** 字节。`au_index` 只用于重组，无 RTP timestamp / SSRC
- [x] 环回：单进程 **POSIX UDP**、`127.0.0.1`（bind 临时端口）；接收超时 2 s；不要求双进程、ICE/TURN、SRTP
- [x] 成功：exit 0；stdout 含 token `units_sent`、`units_recv`、`payload_bytes_sent`、`payload_bytes_recv`、`fragments_sent`、`fragments_recv`、`frag_payload_max`、`port`、`artifact`；单元数、载荷字节、分片数两侧相等且逐单元载荷一致；覆盖写出 `out/video/local-transport/loopback-ok.txt`
- [x] 失败（缺输入 / 非预期 VAU / 套接字或重组失败 / 收发不一致等）：stderr 简述 + exit ≠ 0
- [x] 本票**不**解码、不写 NV12；**不**把 UDP 接收端当成 10 的输入
- [x] 短文 `docs/learning/video/local-transport.md`（按 `_template.md`）：目标、前置、选用库/机制、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明本票单元格式 **≠** 对「必须 RTP」的产品拍板；jitter buffer 一两句位置预告，不实现、不验收
- [x] 课记 `docs/learning/video/local-transport.notes.md`（按 `_notes_template.md` 六栏）：概念（打包 vs 编码、Transport Unit vs 分片）、代码地图、数据流、API/决策、踩坑、下一课钩子
- [x] Catch2：乱序重组、空载荷拒绝、合成载荷的 UDP 环回（含超过 1200 字节的单元）与 artifact 文本；不读真人 `encode.vau`；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0008-learning-video-local-transport-fragmented-udp.md`；术语与 `GLOSSARY.md`「Transport Unit」一致

## 约定（2026-11-03）

- **路径**：`experiments/video/local-transport/` ↔ `docs/learning/video/local-transport.md` + `local-transport.notes.md`
- **未决 #5 / U10**：本票学习选型 = access unit + UDP 分片；**产品是否必须 RTP 仍未决**
- **自动化边界**：分片与环回进 Catch2；真 VAU 烟测按短文
- **与音频的差别**：音频一个 datagram 装下一个 Opus 包；视频同一 Transport Unit 要分片。分片不是 RTP

## Comments

- 2026-11-03: 沿 ADR-0003 的长度前缀语义，把视频 access unit 按 1200 字节拆成 UDP datagram。`au_index` 只用于本机重组。Linux 上对合成载荷做过环回（3 个单元、4 个分片）。macOS 编译以 GitHub Actions `check-learning` 为准。
