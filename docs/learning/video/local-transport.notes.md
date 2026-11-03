# 课记：视频·本地传输/打包

短文（怎么跑、怎么验收）见 [local-transport.md](./local-transport.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改分片大小或 artifact 文本先改这里（并同步代码常量）。

## 这一课在管线哪

Learning Pipeline Stages 的**本地传输/打包**（视频）：**压缩码流 → Transport Unit**。本课不解码、不渲染、不做 jitter buffer、不做 ICE。

- **上游**：`out/video/encode/encode.vau` 里的 access unit 载荷（AVCC 字节）。SPS/PPS 留在 VAU 文件头，**不**单独发送
- **本段产出**：`out/video/local-transport/loopback-ok.txt`（发送与接收一致的记录）。不落盘「收到的码流副本」
- **下游**：解码仍读 `encode.vau`，不读 socket。等价性靠本阶段逐单元字节比对

「打包」把已经压好的 access unit 装进可环回的单元。它不是编码器内部的 NAL 切分，也不是产品 RTP。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/video/local-transport/main.cpp` | CLI、`--input`、`read_vau`、环回、写 artifact、打印两侧计数 |
| `experiments/video/local-transport/transport_unit.hpp` | 分片常量 + `split_access_unit` / `reassemble_access_unit` |
| `experiments/video/local-transport/transport_unit.cpp` | 大端 datagram 的拆与合 |
| `experiments/video/local-transport/udp_loopback.hpp` | `LoopbackStats` + `udp_loopback_roundtrip` |
| `experiments/video/local-transport/udp_loopback.cpp` | `127.0.0.1` 临时端口，发完一个单元的全部分片再收 |
| `experiments/video/local-transport/loopback_artifact.hpp` | artifact 路径 |
| `experiments/video/local-transport/loopback_artifact.cpp` | 计数不一致就拒绝写文件 |
| `experiments/video/local-transport/CMakeLists.txt` | `learning_video_local_transport_lib` + 可执行文件（读 VAU 时链接 encode 库） |
| `experiments/tests/test_video_local_transport.cpp` | Catch2：乱序重组、UDP 环回、artifact 文本 |

可执行文件：`learning_video_local_transport`（cwd＝仓库根）。

## 数据流

```
out/video/encode/encode.vau
  → read_vau：取出每个 access unit 的 payload（不发送参数集）
  → 对每个单元 split_access_unit
  → sendto(127.0.0.1:临时端口) 该单元的全部分片
  → recvfrom 同样片数 → reassemble_access_unit
  → 与原载荷逐字节比较
  → 覆盖写出 out/video/local-transport/loopback-ok.txt
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 默认输入 | `out/video/encode/encode.vau` |
| 一个 Transport Unit | 一个 access unit 的 AVCC 载荷 |
| 单片载荷上限 | **1200** 字节（`kFragPayloadMax`） |
| datagram 头 | 12 字节，大端：`uint32 au_length`、`uint16 frag_index`、`uint16 frag_count`、`uint32 au_index` |
| `au_length` | 整个 access unit 的字节数，每个分片上都写同一个值 |
| `au_index` | 本文件内从 0 起的单元序号，只用于重组 |
| 末片大小 | `au_length - 1200 * (frag_count - 1)`；其余片必须是 1200 |
| 环回 | 单进程 POSIX UDP，`127.0.0.1`，`bind` 端口 0；接收超时 2 s |
| 成功 | `units_sent == units_recv`，载荷字节数相等，分片数相等，且逐单元字节一致 |
| artifact | `out/video/local-transport/loopback-ok.txt` |

artifact 正文（成功才写）：

```
learning_video_local_transport: ok
units_sent=<n>
units_recv=<n>
payload_bytes_sent=<n>
payload_bytes_recv=<n>
fragments_sent=<n>
fragments_recv=<n>
frag_payload_max=1200
port=<ephemeral>
```

Catch2 用合成字节（含 1201 字节、需要两片的单元）做乱序重组和真 UDP 环回；不读真人 `encode.vau`。

## 关键 API / 决策

- **POSIX** `socket` / `bind` / `sendto` / `recvfrom`。端口不写死，记在 `port`。
- **分片**：视频 access unit 放不进「一个 datagram = 一个单元」的音频写法，所以同一 Transport Unit 拆片。见 `docs/adr/0008-learning-video-local-transport-fragmented-udp.md`。
- **可测缝**：`split_access_unit`、`reassemble_access_unit`、`udp_loopback_roundtrip`、`write_loopback_ok_artifact`。

## 踩坑与边界

- 这不是 RTP。对照短文里的 FU-A 才是 WebRTC 的 H.264 分片；本票的头只有长度、片序号、片数和重组用的 `au_index`。
- 参数集不进环回。解码器靠 VAU 文件头建 format description。若只拿到 UDP 载荷而没有文件头，解不开——这是离线接力的缺口，不是本票要补的信令。
- 不做丢包重传。超时或少一片就失败。
- 旧 artifact ≠ 本次成功。
- 无 `--port`、无双进程、无 ICE/TURN/SRTP、无 jitter buffer。

## 下一课钩子

**10 视频·解码**默认读 `out/video/encode/encode.vau`（参数集 + access unit），不重跑 UDP。本阶段已经证明载荷可以原样走一圈环回。
