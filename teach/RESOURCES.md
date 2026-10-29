# 音频管线教室 Resources

## Knowledge

### 编码（上游，课 0002–0003）

- [RFC 6716: Definition of the Opus Audio Codec](https://www.rfc-editor.org/rfc/rfc6716.html)
  IETF 标准：交互式语音/音频编解码器。Use for: 「Opus 是什么」的一手定义。
- [RFC 6716 §2.1.1 Bitrate](https://www.rfc-editor.org/rfc/rfc6716.html#section-2.1.1)
  码率控制与 sweet spots。Use for: 「64 kbps」是压缩预算。
- [Apple: Core Audio Format (CAF) Overview](https://developer.apple.com/library/archive/documentation/MusicAudio/Reference/CAFSpec/CAF_overview/CAF_overview.html)
  CAF 是容器。Use for: 分清盒子与里面的 Opus 包。
- [Apple: AudioConverter](https://developer.apple.com/documentation/audiotoolbox/audioconverter-swift.struct)
  系统 PCM ↔ 压缩格式。Use for: 仓库为何用 AudioConverter。
- Repo: `docs/learning/audio/encode.notes.md` + ADR-0002
  编码契约。Use for: 课内数字；教室不另写一套。

### 本地传输/打包（课 0004+）

- [RFC 768: User Datagram Protocol](https://www.rfc-editor.org/rfc/rfc768.html)
  UDP 一手定义（无连接、数据报）。Use for: 环回实验为何选 UDP、它保证什么/不保证什么。
- [Apple: Audio File Services — AudioFileReadPacketData](https://developer.apple.com/documentation/audiotoolbox/1512134-audiofilereadpacketdata)
  从音频文件按 packet 读出压缩载荷。Use for: 「从 CAF 取出 Opus access unit」的系统 API 名。
- [Beej's Guide to Network Programming — Datagram Sockets](https://beej.us/guide/bgnet/html/#datagram)
  `sendto` / `recvfrom` 可读讲解（非 IETF，但久经检验）。Use for: 单进程 loopback 收发直觉。
- Repo: `docs/learning/audio/local-transport.notes.md` + ADR-0003
  Transport Unit 契约与学习选型。Use for: length 前缀数字、路径、环回边界；**产品是否 RTP 仍未决**。
- Repo: `GLOSSARY.md`「Transport Unit」/「本地传输/打包」
  仓库术语。Use for: 课内用词不另造同义词。

## Wisdom (Communities)

- [Hydrogenaudio Forums](https://hydrogenaud.io/index.php)
  编码器/容器边界讨论较密。Use for: 「文件能播」vs「网上传的是裸 packet」。
- 可选：组内对照 WebRTC 文档里 Opus payload / RTP —— 实战检验「学习 Transport Unit」vs「产品 RTP」。

## Gaps

- WebRTC 对照短文尚未写（Learning 硬毕业另票）；RTP 深挖等对照课再开。
- jitter buffer / 弱网自适应动手实验属加分包，本教室只预告位置。
