# PCM / WAV Resources

## Knowledge

- [Microsoft Learn: Resource Interchange File Format (RIFF)](https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff-)
  RIFF chunk 模型；WAV 里常见的 `RIFF` / `fmt ` / `data` FOURCC。Use for: 打开任意 `.wav` 时在脑子里画容器结构。
- [Microsoft Learn: WAVEFORMATEX](https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatex)
  `nChannels`、`nSamplesPerSec`、`wBitsPerSample`、`nBlockAlign`、`nAvgBytesPerSec` 定义；PCM 时 `wFormatTag = WAVE_FORMAT_PCM`。Use for: 与本仓库 `fmt` 字段一一对照。
- [Apple: AudioStreamBasicDescription](https://developer.apple.com/documentation/coreaudiotypes/audiostreambasicdescription)
  Core Audio 对 sample / frame / packet 的定义；Linear PCM 时 `mFramesPerPacket = 1`。Use for: 读懂 issue 02 的 ASBD。
- [ITU-T G.711](https://www.itu.int/rec/T-REC-G.711)
  电话频带 PCM：8 kHz、8 bit、A-law/µ-law。Use for: **对照**——别把「PCM」只理解成 G.711；本课是线性 PCM。
- Repo: `docs/learning/audio/capture.notes.md` + `experiments/audio/capture/capture_wav.cpp`
  本仓库契约与实现（教室在 `teach/`，契约仍在课记）。Use for: 课内练习与验收挂钩。

## Wisdom (Communities)

- [Hydrogenaudio Forums](https://hydrogenaud.io/index.php)
  音频格式与编码器讨论，信号质量较高。Use for: 格式边界问题（何时用 float PCM、何时 WAV vs CAF）。
- 可选：公司内音视频组同事 / code review——把「这文件是 PCM 还是容器」说清楚即为实战检验。

## Gaps

- 尚无单独精读「Nyquist 采样定理」一手教科书章节（后续课若讲抗混叠再补）。
