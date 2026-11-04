# 视频·五段贯通说明

Learning Pipeline Stages 视频路径：采集 → 编码 → 本地传输/打包 → 解码 → 渲染。五段各有独立入口，靠 `out/` 下的文件接力串成一条路径证据（分段拼接）。每段的契约以各自课记为准，本文只讲怎么串、哪里有缺口。

## 串跑顺序

cwd＝仓库根。采集需要摄像头权限和非沙箱；渲染需要能弹出窗口的图形会话。编码、环回、解码不打开摄像头。

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build
B=./experiments/build/video
$B/capture/learning_video_capture --frames 30
$B/encode/learning_video_encode
$B/local-transport/learning_video_local_transport
$B/decode/learning_video_decode
$B/render/learning_video_render
```

| 阶段 | 入口 | 输入 | 输出 | 课记 |
| --- | --- | --- | --- | --- |
| 采集 | `learning_video_capture` | 默认摄像头 | `out/video/capture/`（tight NV12 + PTS manifest + `preview.png`） | [capture.notes.md](./capture.notes.md) |
| 编码 | `learning_video_encode` | 采集目录 | `out/video/encode/encode.vau`（H.264 access unit） | [encode.notes.md](./encode.notes.md) |
| 本地传输/打包 | `learning_video_local_transport` | `encode.vau` | `out/video/local-transport/loopback-ok.txt`（逐单元一致） | [local-transport.notes.md](./local-transport.notes.md) |
| 解码 | `learning_video_decode` | `encode.vau` | `out/video/decode/`（tight NV12 + manifest + `preview.png`） | [decode.notes.md](./decode.notes.md) |
| 渲染 | `learning_video_render` | 解码目录 | 窗口画面 + stdout | [render.notes.md](./render.notes.md) |

## 缺口（如实标明）

1. **解码不吃 UDP 收到的字节**：解码读的是 `encode.vau`，不是环回之后落盘的包。等价性靠本地传输/打包的逐单元比对（`units_sent == units_recv`、载荷一致）。参数集（SPS/PPS）留在 VAU 文件头，环回根本不发送它们；只有文件头还在，解码器才建得起 format description。
2. **有损，且不修到与采集逐字节相同**：`preview.png` 应能认出刚拍的画面，不要求像素差为零。编码码率固定 2 Mbps，没有弱网反馈。
3. **离线文件接力，不是实时流式管线**：每段跑完写文件、下一段再读。没有边采边编边发，也就没有丢包和时钟漂移要对付。jitter buffer、A/V 同步在这条路径上只有叙述位置。渲染只按视频自己的 `relative_pts_ns` 播，没有音频主钟。实时流式与 A/V 同步动手都属于加分包。
4. **分片不是 RTP**：环回把一个 access unit 拆成 1200 字节的 UDP datagram。这是学习选型，不决定产品必须（或禁止）使用 RTP。

「讲得清管线」≠「做过实时管线」：本文证明的是五段各自能跑通、且产物能前后接上。

## 人工验收

- [ ] 按上面顺序一次跑完五段，各段 exit 0
- [ ] `learning_video_render` 的窗口里能看见采集阶段对着摄像头的那一段画面
- 验收日期：〈填写 YYYY-MM-DD〉
