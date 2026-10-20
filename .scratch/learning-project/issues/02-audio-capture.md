# 02: 音频·采集

**What to build:** 音频 Learning Pipeline Stage「采集」：用 macOS AudioQueue 从默认麦克风取到 PCM，写出可观察 WAV，并写对应短文。库与 API 已在本票 grilling 选定。

**Blocked by:** 01 Learning harness 地基

**Status:** done

## Acceptance

- [x] `experiments/audio/capture/` 可执行目标 `learning_audio_capture`；目录内静态库 `learning_audio_capture_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`
- [x] 采集 API：macOS **AudioQueue**（AudioToolbox / Core Audio）；无第三方媒体库；无独立重采样模块（以队列 ASBD 钉死格式）
- [x] PCM 契约：**48 kHz / mono / s16le**；成功时覆盖写出 `out/audio/capture/capture.wav`（cwd＝仓库根）
- [x] CLI：默认录制 **3** 秒；`--seconds` 取值 **(0, 60]**；仅系统默认输入设备；无 GUI、不做 .app / 自定义 Info.plist
- [x] 失败（无设备、权限拒绝、非法参数、写文件失败等）：stderr 简述 + **exit ≠ 0**；成功 exit 0
- [x] 短文 `docs/learning/audio/capture.md`（按 `_template.md`）：目标、前置（含 Terminal 麦克风权限）、选用库、运行步骤、可观察成功判据（能播放本次录音）、失败时仍记录什么、与主缝关系；AEC **仅一句位置预告**，不实现、不验收
- [x] Catch2：仅护合成 PCM→WAV 契约（RIFF/WAVE、`fmt` 为 48 kHz / 1 ch / 16-bit、`data` 大小一致）；不测 AudioQueue、不访问麦克风；`scripts/check-learning.sh` 仍可通过
- [x] ADR：`docs/adr/0001-learning-audio-capture-coreaudio.md`；术语与 `GLOSSARY.md`「采集」一致

## 约定（grilling 2026-10-20）

- **路径**：`experiments/audio/capture/` ↔ `docs/learning/audio/capture.md`
- **产物**：固定路径覆盖写；03 编码可以该 WAV 为默认可追溯输入
- **权限**：短文写明「系统设置 → 麦克风 → 允许终端」；本票不打包 App
- **自动化边界**：与 01 一致——可无硬件复现的契约进 Catch2；真麦人工按短文听

## Comments

- 2026-10-20: `/grill-with-docs` 定稿 API/PCM/WAV/CLI/权限/失败退出码/Catch2 边界/AEC 预告/命名/ADR；Glossary「采集」改为音＝PCM / 视＝帧；确认共享理解后落盘票面与 ADR。
- 2026-10-20: `/implement` 落地 AudioQueue 采集、`write_capture_wav`、Catch2、短文；本机 `--seconds 1` 写出可播放 WAV；`scripts/check-learning.sh` 通过。
