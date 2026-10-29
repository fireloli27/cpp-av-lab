# 音频·五段贯通说明

Learning Pipeline Stages 音频路径：采集 → 编码 → 本地传输/打包 → 解码 → 渲染。五段各有独立入口，靠 `out/` 下的文件接力串成一条路径证据（分段拼接）。每段的契约以各自课记为准，本文只讲怎么串、哪里有缺口。

## 串跑顺序

cwd＝仓库根；须在非沙箱、有麦克风权限与默认输出设备的终端里跑。

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build
B=./experiments/build/audio
$B/capture/learning_audio_capture --seconds 3          # 对着麦克风说一句话
$B/encode/learning_audio_encode
$B/local-transport/learning_audio_local_transport
$B/decode/learning_audio_decode
$B/render/learning_audio_render                         # 听见刚才那句话
```

| 阶段 | 入口 | 输入 | 输出 | 课记 |
| --- | --- | --- | --- | --- |
| 采集 | `learning_audio_capture` | 默认麦克风 | `out/audio/capture/capture.wav`（PCM） | [capture.notes.md](./capture.notes.md) |
| 编码 | `learning_audio_encode` | `capture.wav` | `out/audio/encode/encode.caf`（Opus 码流） | [encode.notes.md](./encode.notes.md) |
| 本地传输/打包 | `learning_audio_local_transport` | `encode.caf` | `out/audio/local-transport/loopback-ok.txt`（逐包一致） | [local-transport.notes.md](./local-transport.notes.md) |
| 解码 | `learning_audio_decode` | `encode.caf` | `out/audio/decode/decode.wav`（PCM） | [decode.notes.md](./decode.notes.md) |
| 渲染 | `learning_audio_render` | `decode.wav` | 扬声器声音 + stdout | [render.notes.md](./render.notes.md) |

## 缺口（如实标明）

1. **解码不吃 UDP 接收端的数据**：解码读的是 `encode.caf`，不是本地传输/打包阶段从 UDP 收到后落盘的包。等价性靠本地传输/打包阶段的逐包字节比对证明（`units_sent == units_recv`、载荷一致），而不是让数据真的从 socket 流进解码器。
2. **CAF priming 时长偏差未修齐**：解码出的 `pcm_frames` 与采集帧数差一个编码延迟量级，只用 1/2～2 倍窗口约束（见 decode 课记），渲染照原样播放。
3. **离线文件接力，不是实时流式管线**：每段跑完写文件、下一段再读，没有边采边编边发边收边播，也就没有网络抖动、丢包、时钟漂移需要对付。jitter buffer、A/V 同步、AEC 在这条路径上只有叙述位置，没有实物。实时流式属加分包，与双端互通、网络真实感同档。

「讲得清管线」≠「做过实时管线」：本文证明的是五段各自能跑通、且产物能前后接上。

## 人工验收

- [ ] 按上面顺序一次跑完五段，各段 exit 0
- [ ] `learning_audio_render` 播放时能听见自己在采集阶段说的那句话
- 验收日期：〈填写 YYYY-MM-DD〉
