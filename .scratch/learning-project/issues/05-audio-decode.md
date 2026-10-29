# 05: 音频·解码

**What to build:** 音频 Learning Pipeline Stage「解码」：从 Opus CAF 读出 access unit，用 `AudioConverter` 还原为与采集契约相同的 PCM，写成 WAV；写短文与课记。不重跑 UDP，不修 CAF priming。

**Blocked by:** 04 音频·本地传输/打包

**Status:** ready-for-agent

## Acceptance

- [ ] `experiments/audio/decode/` 可执行目标 `learning_audio_decode`；目录内静态库（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`；可执行文件不链接采集库
- [ ] 输入：默认读 `out/audio/encode/encode.caf`；可选 `--input`；macOS **AudioToolbox** `AudioFileReadPacketData` 读出 Opus access unit，并带上 magic cookie；无第三方媒体库（不引入 libopus）
- [ ] 非 Opus、或不是 48 kHz / mono 的 CAF：失败。不重跑 UDP，不改 04
- [ ] 解码：按包 `AudioConverter`（Opus → 48 kHz / mono / s16le PCM），自己写 WAV。不用 `ExtAudioFile` 整文件转换
- [ ] 成功：exit 0；覆盖写出 `out/audio/decode/decode.wav`（cwd＝仓库根）；stdout 含 token `packets` 与 `pcm_frames`（与短文判据同名；语义以课记为 SSOT）
- [ ] 时长窗口：`source_frames = packet_count × frames_per_packet`；`pcm_frames > 0` 且 `source_frames / 2 ≤ pcm_frames ≤ source_frames × 2`。不按 CAF priming 修到与原始 PCM 等长；不与 `capture.wav` 逐样本对比
- [ ] 失败（缺输入 / 非预期 CAF / 读包或转换失败等）：stderr 简述 + exit ≠ 0
- [ ] 本票**不**做 jitter buffer、丢包、PLC；**不**定 06 的播放 API。`decode.wav` 是交给 06 的默认产物
- [ ] 短文 `docs/learning/audio/decode.md`（按 `_template.md`）：目标、前置、选用库/机制、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明所读包字节就是 04 放进 **Transport Unit** 的载荷，环回一致性已由 04 证明；jitter buffer **一两句**位置预告（接收后、本阶段之前），不实现、不验收；`afplay` 听感为人工项
- [ ] 课记 `docs/learning/audio/decode.notes.md`（按 `_notes_template.md` 六栏）：概念（码流 → **PCM 样本**；输入不是长度前缀外壳）、代码地图、数据流、API/决策、踩坑、下一课钩子；契约数字/路径（含时长窗口）以课记为 SSOT
- [ ] Catch2：用 `learning_audio_encode_lib` 从不少于 1 秒的合成音调生成 Opus CAF，再解码；断言 48 kHz / mono / s16le、至少一个样本非 0、以及时长窗口。不读真人 `encode.caf`、不提交二进制 fixture；`scripts/check-learning.sh` 仍可通过
- [ ] ADR：`docs/adr/0004-learning-audio-decode-audioconverter.md`；术语与 `GLOSSARY.md`「解码」一致（音频侧还原为 PCM 样本）

## 约定（grilling 2026-10-29）

- **路径**：`experiments/audio/decode/` ↔ `docs/learning/audio/decode.md` + `decode.notes.md`
- **产物**：固定路径覆盖写 `out/audio/decode/decode.wav`；听感不在自动验收内
- **与 04 的交接**：硬输入是 CAF 里的 Opus access unit，不是 UDP 载荷落盘，也不是 Transport Unit 的长度前缀
- **自动化边界**：与 01–04 一致——可无硬件复现的契约进 Catch2；真人 `encode.caf` 的 `afplay` 按短文（`AudioConverter` 须非沙箱，见 harness）
- **priming**：接受解码时长与包时间轴差一个编码延迟量级；窗口用 1/2～2 倍，不修齐

## Comments

- 2026-10-29: `/grill-with-docs` 定稿 Opus CAF access unit + 按包 `AudioConverter` → PCM WAV、时长窗口、Catch2 边界、与 04/06 交接、ADR-0004、Glossary「解码」改为音频侧 PCM 样本；确认共享理解后落盘票面与 ADR。
