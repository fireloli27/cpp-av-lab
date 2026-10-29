# 06: 音频·渲染

**What to build:** 音频 Learning Pipeline Stage「渲染」：读 05 产出的 PCM WAV，用 AudioQueue output 由设备回调拉取、经默认输出设备可听见地播放；写短文与课记。另写 `pipeline.md` 作为音频五段贯通说明（分段拼接，缺口点名）。

**Blocked by:** 05 音频·解码

**Status:** done

## Acceptance

- [x] `experiments/audio/render/` 可执行目标 `learning_audio_render`；目录内静态库 `learning_audio_render_lib`（供 exe + Catch2 链接）；CMake 挂入 `experiments/`；不抽 `experiments/common`；不链接采集库
- [x] 播放：macOS **AudioToolbox** `AudioQueueNewOutput`，仅系统默认输出设备；不调用 `afplay` / `system()`，不引入第三方媒体库
- [x] 输入：默认读 `out/audio/decode/decode.wav`；可选 `--input`；用 `AudioFileOpenURL` + `AudioFileReadPacketData` 读（不手写 RIFF 解析）；仅接受 48 kHz / mono / s16le PCM WAV，其余失败；不重采样、不做声道转换；无 `--tone`、`--device`、音量、截断参数
- [x] 缓冲：3 个 AudioQueue buffer × 960 帧（20 ms），约 60 ms 输出侧排队；常量在头文件，契约数字以课记为 SSOT
- [x] 成功：整段播完——`AudioQueueStop(queue, false)` 排空并等 `kAudioQueueProperty_IsRunning` 变成 0（期间跑 run loop）——后 exit 0；stdout 含 token `pcm_frames`、`frames_rendered`（以及 `buffers`、`buffer_frames`）；判据为 `pcm_frames == frames_rendered`。不向 `out/` 写任何产物
- [x] 失败（缺输入 / 格式不符 / 0 帧 / AudioQueue 创建或启动失败等）：stderr 简述 + exit ≠ 0
- [x] 短文 `docs/learning/audio/render.md`（按 `_template.md`）：目标、前置（非沙箱、默认输出设备）、选用库/机制、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系；写明 `frames_rendered` 只证明「交给了设备」，出声靠人耳；jitter buffer / A/V 同步 / AEC 参考信号各一两句预告；链接 `pipeline.md`
- [x] 课记 `docs/learning/audio/render.notes.md`（按 `_notes_template.md` 六栏）：概念（pull 模型、buffer 个数 × 大小 = 延迟 vs 抗卡顿）、代码地图、数据流、API/决策、踩坑、下一课钩子；旁路叙述 SSOT：①jitter buffer（接收后、解码前，吸收网络到达抖动）≠ 输出缓冲（渲染端，吸收调度抖动）②A/V 同步以音频播放时钟为主钟 ③AEC 参考信号取自渲染路径
- [x] 贯通说明 `docs/learning/audio/pipeline.md`：表格列五段命令与产物交接（capture.wav → encode.caf → 04 环回比对 → decode.wav → 播放）；缺口点名：①解码读 `encode.caf` 而非 UDP 接收落盘，等价性由 04 逐字节比对证明 ②CAF priming 时长偏差未修齐 ③离线文件接力，非实时流式（实时流式属加分包）；人工验收：按序跑完五段、用 `learning_audio_render` 听见自己采集时说的话，记日期。不提供一键串跑脚本
- [x] Catch2：WAV 加载与格式拒绝（测试自行合成 WAV，含不符格式与 0 帧）；PCM 游标分块（末块不满、总帧数守恒）。不打开真实输出设备、不提交二进制 fixture；`scripts/check-learning.sh` 仍可通过
- [x] 不把 jitter buffer / A/V 同步 / AEC 做成实验
- [x] ADR：`docs/adr/0005-learning-audio-render-audioqueue.md`；术语与 `GLOSSARY.md`「渲染」一致

## 约定（grilling 2026-10-29）

- **路径**：`experiments/audio/render/` ↔ `docs/learning/audio/render.md` + `render.notes.md`；贯通说明 `docs/learning/audio/pipeline.md`
- **与 05 的交接**：硬输入是 48 kHz / mono / s16le PCM WAV；`capture.wav` 同契约，也可直接喂入（采集→渲染短路试听）
- **可观察结果**：听见声音 + stdout 日志；不写「渲染成功标记文件」
- **自动化边界**：与 01–05 一致——纯逻辑缝进 Catch2；真实播放按短文人工验收，AudioQueue 须非沙箱（见 harness）
- **一键脚本**：不做；采集需真人与麦克风权限，脚本会掩盖人工前置

## Comments

- 2026-10-29: `/grill-with-docs` 定稿 AudioQueue output、WAV 输入契约、3×960 帧缓冲、`pcm_frames == frames_rendered` 判据、Catch2 纯逻辑缝、旁路叙述三点、`pipeline.md` 贯通说明与缺口；Glossary「渲染」磨尖；ADR-0005。
- 2026-10-29: `/implement` 完成：`learning_audio_render`（AudioQueue output）+ Catch2（WAV 加载/拒绝、游标分块）；短文/课记/`pipeline.md`。改前真机串跑 encode→decode→render 通过（`pcm_frames=frames_rendered=50040`）；评审后修短输入 `IsRunning` 竞态与 `frames_rendered` 改计成功入队帧数，复测时本机输出设备整体不可用（`afplay` 同样 `AudioQueueStart` 失败），待设备恢复后人工复测；`pipeline.md` 人工验收由作者勾选。
