# 03: 音频·编码

**What to build:** 音频 Learning Pipeline Stage「编码」：将采集侧 PCM 压成 Opus 码流，写出可观察 CAF，并写对应短文与课记。库与 API 已在本票 grilling 选定。

**Blocked by:** 02 音频·采集

**Status:** done

## Acceptance

- [x] `experiments/audio/encode/` 可执行目标 `learning_audio_encode`；目录内静态库 `learning_audio_encode_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 编码 API：macOS **AudioToolbox / `AudioConverter`**，格式 **Opus**（`kAudioFormatOpus`）；无第三方媒体库（不引入 libopus / FFmpeg）
- [x] 输入：默认读 `out/audio/capture/capture.wav`；可选 `--input <wav>`；须为 **48 kHz / mono / s16le PCM WAV**，否则失败（不做重采样/混音）
- [x] 码率固定 **64 kbps**；分包跟系统 `AudioConverter` 默认（课记记录实测 packet 帧数/时长）；无 `--bitrate`、无自适应码率
- [x] 成功时覆盖写出 `out/audio/encode/encode.caf`（CAF 内 Opus）；无 `--output`；本票不做 RTP/自定义分帧
- [x] CLI：无采麦、无 GUI；成功 exit 0；缺输入/格式不符/编码或写盘失败 → stderr 简述 + exit ≠ 0
- [x] 短文 `docs/learning/audio/encode.md`（按 `_template.md`）：目标、前置、选用库、运行步骤、可观察成功判据（`afplay` / 系统播放器能听出与输入对应的内容；日志含契约摘要、约包数/字节/时长）、失败时仍记录什么、与主缝关系；AEC 与弱网码率自适应 **各一两句位置预告**，不实现、不验收
- [x] 课记 `docs/learning/audio/encode.notes.md`（按 `_notes_template.md` 六栏）：概念、代码地图、数据流、API/决策、踩坑、下一课钩子；契约数字以课记为 SSOT
- [x] Catch2：合成短 PCM → lib 可测缝 → 临时 root 下 CAF；断言非空且可识别为含 Opus 的 CAF；不测麦克风、不依赖真人 `capture.wav`、不提交仓库内二进制 fixture；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0002-learning-audio-encode-opus-audioconverter.md`；术语与 `GLOSSARY.md`「编码」一致

## 约定（grilling 2026-10-22）

- **路径**：`experiments/audio/encode/` ↔ `docs/learning/audio/encode.md` + `encode.notes.md`
- **产物**：固定路径覆盖写；04 本地传输/打包以该 CAF（或短文写明的等价码流单元）为默认可追溯输入
- **自动化边界**：与 01/02 一致——可无硬件复现的契约进 Catch2；听感人工按短文
- **政策**：维持 harness「实验代码无第三方媒体库；系统框架允许」

## Comments

- 2026-10-22: `/grill-with-docs` 定稿 Opus + `AudioConverter`、离线 WAV 输入、`encode.caf`、64 kbps、系统默认分包、CLI/失败退出码、Catch2 边界、旁路预告、ADR-0002、Glossary「编码」sharpen；确认共享理解后落盘票面与 ADR。
- 2026-10-22: `/implement` 落地 AudioConverter + AudioFileWritePackets Opus CAF、`write_encode_caf`、Catch2、短文与课记；本机默认输入编码可 `afplay`；`scripts/check-learning.sh` 通过。
