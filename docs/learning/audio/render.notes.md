# 课记：音频·渲染

短文（怎么跑、怎么验收）见 [render.md](./render.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改输入契约/缓冲数字/成功判据先改这里（并同步代码常量），短文与 teach 课只引用。

## 这一课在管线哪

Learning Pipeline Stages 的**渲染**（音频）：**PCM 样本 → 扬声器**。输出设备按自己的时钟消费样本；AudioQueue 在需要下一块时回调我们，我们从 PCM 里取一块填进去（**pull 模型**）。不是我们决定什么时候播，而是设备来拉。

- **上游**：解码产物 `out/audio/decode/decode.wav`（48 kHz / mono / s16le PCM WAV）。`capture.wav` 同契约，也可直接喂入试听「采集 → 渲染」短路
- **本段产出**：扬声器里的声音 + stdout 日志；不写文件
- **下游**：无（音频路径终点）。五段贯通见 [pipeline.md](./pipeline.md)

### 旁路叙述（必会，不实现）

1. **jitter buffer ≠ 输出缓冲**：jitter buffer 在**接收之后、解码之前**，按包序号/时间戳攒一点包，吸收网络到达时间的抖动（包早到晚到）。本实验的 AudioQueue buffer 在**渲染端**，吸收的是线程调度抖动——回调晚一点醒来，设备手里还有已排队的样本可播。两者都用「多攒一点 = 多一点延迟」换平滑，但位置与对付的抖动不同。
2. **A/V 同步**落在渲染这一带：音频按设备时钟连续播放，不好随意跳帧，通常作为主钟；视频渲染按「音频此刻实际播到哪个时间戳」决定显示、等待或丢帧。
3. **AEC 参考信号**取自渲染路径：即将交给扬声器的远端 PCM 同时交给 AEC 作参考，AEC 用它从采集信号里减掉扬声器漏进麦克风的回声（处理本身在发送侧，见 [capture.notes.md](./capture.notes.md)）。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/render/main.cpp` | CLI、`--input`、加载 → 播放、打成功日志 |
| `experiments/audio/render/render_pcm.hpp` | 契约常量 + `load_render_wav` + `PcmCursor` |
| `experiments/audio/render/render_pcm.cpp` | `AudioFileOpenURL` / `AudioFileReadPacketData` 读 WAV；游标分块 |
| `experiments/audio/render/play_queue.hpp` | `RenderStats` + `play_pcm` |
| `experiments/audio/render/play_queue.cpp` | AudioQueue output：预填、回调续填、排空等待 |
| `experiments/audio/render/CMakeLists.txt` | `learning_audio_render_lib` + `learning_audio_render` |
| `experiments/tests/test_render_pcm.cpp` | Catch2：WAV 加载 / 格式拒绝 / 游标分块 |

可执行文件：`learning_audio_render`（cwd＝仓库根）。不链接采集库。

## 数据流

```
out/audio/decode/decode.wav（或 --input）
  → AudioFileOpenURL + 校验 48 kHz / mono / s16le、帧数 > 0
  → AudioFileReadPacketData 读出全部 PCM（内存）
  → AudioQueueNewOutput（回调挂在当前线程 run loop）
  → 预填 3 个 buffer（每个 ≤ 960 帧）并 Enqueue → AudioQueueStart
  → 设备播完一块 → 回调 → PcmCursor 取下一块 → Enqueue
  → 游标取空 → AudioQueueStop(queue, false)（播完已排队的再停）
  → run loop 等到 kAudioQueueProperty_IsRunning == 0 → Dispose
  → stdout: pcm_frames / frames_rendered / buffers / buffer_frames
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 输入 | PCM WAV，48 000 Hz / mono / s16le（signed integer、little-endian）；帧数 > 0 |
| 默认输入路径 | `out/audio/decode/decode.wav` |
| 输出设备 | 系统默认输出设备 |
| 缓冲 | `kNumBuffers = 3` × `kBufferFrames = 960`（20 ms）；排队约 60 ms |
| `pcm_frames` | WAV 里加载到的帧数 |
| `frames_rendered` | `AudioQueueEnqueueBuffer` 成功排队的累计帧数 |
| 成功判据 | 排空后 exit 0，且 `pcm_frames == frames_rendered` |
| 成功日志 | stdout 含 `pcm_frames`、`frames_rendered`、`buffers`、`buffer_frames` |
| 产物 | 无（不写 `out/`） |

非契约 WAV 直接失败，不做重采样、声道转换或位深转换。

**缓冲取舍**：buffer 个数 × 大小 ≈ 输出侧延迟。块越大/越多，回调偶尔迟到也不会断音，但「从拿到样本到出声」越晚；块越小/越少，延迟低但更容易欠载（underrun）出爆音。3 × 20 ms 是学习用的保守中间值，不作调参实验。

Catch2 用手写 RIFF 合成 WAV（含额外 `LIST` chunk），断言样本按序加载；44.1 kHz、stereo、8-bit、0 帧、缺文件均拒绝；游标按 960 帧分块、末块不满、总帧数守恒。不打开真实输出设备、不提交二进制 fixture。

## 关键 API / 决策

- **`AudioFileOpenURL` + `AudioFileReadPacketData`**：读 WAV；对 PCM 而言 1 packet = 1 frame。不手写 RIFF 解析，额外 chunk 由 AudioFile 处理。
- **`AudioQueueNewOutput`**：传 `CFRunLoopGetCurrent()`，回调在主线程 run loop 里跑，因此状态无需加锁。
- **`AudioQueueAllocateBuffer` / `AudioQueueEnqueueBuffer`**：先手动预填所有 buffer 再 `AudioQueueStart`，否则一开始没东西可播。
- **`AudioQueueStop(queue, false)`**：非立即停止——已排队的 buffer 播完才真正停；随后轮询 `kAudioQueueProperty_IsRunning`。
- **决策**：AudioQueue output 而非 AudioUnit / `AVAudioEngine` / `afplay` → `docs/adr/0005-learning-audio-render-audioqueue.md`。
- **可测缝**：`load_render_wav`、`PcmCursor`；`play_pcm` 只靠短文人工验收。

## 踩坑与边界

- 沙箱内 AudioQueue output 可能拿不到设备；真播放须非沙箱（见 harness）。
- `frames_rendered` 是「交给设备」不是「已从喇叭出来」；静音、错误输出设备时仍可能 exit 0。
- 排空等待有截止时间（时长 + 5 s），设备卡死时 stderr 报错并 exit ≠ 0，而不是挂住。
- 不足 3 个 buffer 的极短输入：游标在预填阶段就取空，`AudioQueueStart` 后立即请求非立即停止。`kAudioQueueProperty_IsRunning` 在 Start 后是异步变 1 的，所以要先等到它变成 1、再等它回到 0；否则可能一开始就读到 0，提前 Dispose 把声音截掉。`kAudioQueueProperty_IsRunning` 在 Start 后是异步变 1 的，所以要先等到它变成 1、再等它回到 0；否则可能一开始就读到 0，提前 Dispose 把声音截掉。
- 本票不做 jitter buffer、A/V 同步、AEC、`--device`、音量、截断、Ctrl-C 处理。

## 下一课钩子

音频五段到此齐了，贯通证据与缺口见 [pipeline.md](./pipeline.md)。视频路径五段的库选型仍未决（spec 未决 #4）；WebRTC 对照短文可从本课的旁路三点（jitter buffer、A/V 同步、AEC 参考）切入映射。
