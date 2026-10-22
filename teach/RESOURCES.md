# 音频编码 / Opus · CAF Resources

## Knowledge

- [RFC 6716: Definition of the Opus Audio Codec](https://www.rfc-editor.org/rfc/rfc6716.html)
  IETF 标准：交互式语音/音频编解码器；约 6 kbit/s–510 kbit/s 量级。Use for: 「Opus 是什么、为何适合通话」的一手定义，不必通读实现附录。
- [RFC 6716 §2.1.1 Bitrate](https://www.rfc-editor.org/rfc/rfc6716.html#section-2.1.1)
  码率控制参数与 sweet spots（语音/音乐量级）。Use for: 讲清「64 kbps」是压缩预算，以及它在 Opus 范围里偏哪一档。
- [ITU-T G.711](https://www.itu.int/rec/T-REC-G.711/en)
  8 kHz × 8 bit → 64 kbit/s 电话 PCM。Use for: **对照**——同数字「64k」不是 Opus 压缩目标码率。
- [Apple: Core Audio Format (CAF) Overview](https://developer.apple.com/library/archive/documentation/MusicAudio/Reference/CAFSpec/CAF_overview/CAF_overview.html)
  CAF 是容器；必有 Description + Data；VBR/VFR 还需 Packet Table。Use for: 分清「盒子」与里面的 Opus 包。
- [Apple: CAF Specification](https://developer.apple.com/library/archive/documentation/MusicAudio/Reference/CAFSpec/CAF_spec/CAF_spec.html)
  chunk 布局（`desc` / `pakt` / `data` / `kuki` 等）。Use for: 对照 `afinfo` / 课记踩坑里的 `free`。
- [Apple: AudioConverter](https://developer.apple.com/documentation/audiotoolbox/audioconverter-swift.struct)
  系统 PCM ↔ 压缩格式转换入口。Use for: 理解本仓库为何用 `AudioConverter` 而不是 libopus C API。
- Repo: `docs/learning/audio/encode.notes.md` + ADR-0002
  本仓库编码契约与选型。Use for: 课内数字与路径；教室不另写一套。
- Repo: `docs/learning/audio/capture.notes.md`
  上游 PCM 契约。Use for: 编码输入从哪来。

## Wisdom (Communities)

- [Hydrogenaudio Forums](https://hydrogenaud.io/index.php)
  编码器/容器边界讨论较密。Use for: 「文件能播」vs「网上传的是裸 packet」这类概念题。
- 可选：组内对照 WebRTC 文档里 Opus payload —— 实战检验「容器 vs 网上单元」。

## Gaps

- Opus 官方实现与 RFC 附录对照精读（深挖 SILK/CELT 时再开）。
- WebRTC 对照短文尚未写（Learning 硬毕业另票）。
