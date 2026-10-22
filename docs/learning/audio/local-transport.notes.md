# 课记：音频·本地传输/打包

短文（怎么跑、怎么验收）见 [local-transport.md](./local-transport.md)。  
**本文件是本阶段格式契约与代码地图的权威来源**；改路径/单元格式/产物字段先改这里（并同步代码常量），短文与 teach 课只引用。

## 这一课在管线哪

Learning Pipeline Stages 的**本地传输/打包**（音频）：把编码侧的 Opus **access unit** 组织成可在本机环回发送/接收的 **Transport Unit**。本课不解码、不写 PCM、不做 jitter buffer、不拍板产品必须 RTP。

- **上游**：编码产物 `out/audio/encode/encode.caf`（或短文写明的等价 Opus CAF）
- **本段产出**：UDP loopback 校验通过后的 `out/audio/local-transport/loopback-ok.txt`
- **下游**：音频「解码」将另开契约（本票**不定** 05 输入格式；可选落盘载荷序列不作为本票硬验收）

**打包 vs 编码「分包」**：编码课里的 `frames_per_packet` 是编解码器一次吐出的压缩帧大小；本课的 Transport Unit 是「一个 access unit + 长度前缀」的环回发送单元，二者不要混称。

jitter buffer（叙述必会旁路）通常在**接收之后、解码之前**，吸收网络抖动——本阶段实验不实现。

## 代码地图

| 路径 | 一句话 |
| --- | --- |
| `experiments/audio/local-transport/main.cpp` | CLI、`--input`、读 CAF → UDP 环回 → 写产物 |
| `experiments/audio/local-transport/transport_unit.hpp` | `pack_transport_unit` / `unpack_transport_unit` |
| `experiments/audio/local-transport/udp_loopback.hpp` | 单进程 `127.0.0.1` 临时端口自发自收 |
| `experiments/audio/local-transport/read_opus_caf.hpp` | AudioToolbox 读出 Opus 包 |
| `experiments/audio/local-transport/loopback_artifact.hpp` | 写出 `loopback-ok.txt` |
| `experiments/audio/local-transport/CMakeLists.txt` | `learning_audio_local_transport_lib` + exe |
| `experiments/tests/test_local_transport.cpp` | Catch2：组包/拆包 + UDP 环回 + 产物（合成载荷） |

可执行文件：`learning_audio_local_transport`（cwd＝仓库根）。

## 数据流

```
out/audio/encode/encode.caf（或 --input）
  → read_opus_packets_from_caf：AudioFileOpenURL + AudioFileReadPacketData
  → 每个 Opus 包 → pack_transport_unit（uint32 BE length + 载荷）
  → udp_loopback_roundtrip：bind 127.0.0.1:0 → sendto 自己 → recvfrom → unpack 校验
  → write_loopback_ok_artifact → out/audio/local-transport/loopback-ok.txt
```

### 契约（权威）

| 项 | 值 |
| --- | --- |
| 默认输入路径 | `out/audio/encode/encode.caf` |
| 输入内容 | CAF 内 Opus access unit（`mFormatID == kAudioFormatOpus`） |
| Transport Unit | `uint32` **大端** length + 载荷；length = 载荷字节数（不含这 4 字节）；无 seq / timestamp / SSRC |
| 环回 | 单进程 POSIX UDP、`127.0.0.1`；**bind 端口 0（临时端口）**后自发自收；端口写入产物与日志 |
| 成功产物路径 | `out/audio/local-transport/loopback-ok.txt` |
| 产物内容 | 见下（覆盖写） |

`loopback-ok.txt` 固定四行（末行换行）：

```
learning_audio_local_transport: ok
units=<n>
payload_bytes=<total payload bytes across units>
port=<ephemeral UDP port>
```

成功条件：`units_sent == units_recv` 且逐包载荷字节一致。产物里的 `payload_bytes` 与日志里的 `payload_bytes_sent` / `payload_bytes_recv` 均为载荷合计（不含 length 前缀）；成功时两侧相等。

Catch2 用合成 Opus 载荷字节测组包/拆包与 UDP 环回；**不**读真人 `encode.caf`、不提交二进制 fixture。

## 关键 API / 决策

- **AudioFileReadPacketData**：从 CAF 拉出 Opus 包字节；不解码。
- **POSIX UDP**：`socket` / `bind(127.0.0.1:0)` / `getsockname` / `sendto` / `recvfrom`。
- **决策**：长度前缀 Transport Unit + UDP loopback（非产品 RTP）→ `docs/adr/0003-learning-audio-local-transport-length-prefix-udp.md`。
- **可测缝**：`pack`/`unpack`、`udp_loopback_roundtrip`、`write_loopback_ok_artifact`；真 CAF→UDP 烟测按短文（人工）。

## 踩坑与边界

- 本票学习选型 ≠ 产品「必须 RTP」（spec 未决 #5 / U10 仍开）。
- 不做双进程、ICE/TURN、SRTP、jitter buffer、解码/PCM。
- 旧 `loopback-ok.txt` ≠ 本次成功；失败时 exit ≠ 0。
- 沙箱可能影响套接字或 AudioFile；护栏与烟测须在非沙箱环境跑（见 `docs/learning/harness/README.md`）。
- UDP 单数据报上限约 64 KiB；学习路径 Opus 包远小于此。空载荷在单元格式上合法（Catch2 覆盖），真人 CAF 通常无空包。

## 下一课钩子

**05 音频·解码**将另定输入契约（本票不锁定）。本段只证明：access unit 可按 Transport Unit 在本机收发一致。产品侧若改用 RTP，WebRTC 对照短文再映射；学习环回不必重写成 RTP。
