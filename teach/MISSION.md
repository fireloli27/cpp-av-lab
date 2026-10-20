# Mission: PCM / WAV for Learning Pipeline「采集」

## Why

你是 C++ Client AV SDK 学习者：issue 02 已把麦克风采成 `out/audio/capture/capture.wav`。要能**自己讲清**「文件里到底是什么」、为何契约是 48 kHz mono s16le，并在 03「编码」时知道该吃 PCM 还是误把 WAV 当码流。

## Success looks like

- 能用自己的话区分 **PCM（样本布局）** 与 **WAV（RIFF 容器）**
- 能对照 `capture_wav.cpp` / Catch2 指出 `fmt` 与 `data` 各管什么
- 能口头算出：1 秒本课契约 PCM 大约多少字节
- 知道 **G.711 电话 PCM** 与本课 **线性 PCM** 不是同一回事（避免后面编码课混概念）

## Constraints

- 绑在 Learning Project / issue 02；优先短课 + 可检索 cheat sheet
- 中文讲解；路径与术语（PCM、WAV、ASBD）不硬翻
- 知识须能指向 `RESOURCES.md` 里的高可信来源

## Out of scope（本轮）

- Opus/AAC 编码细节（留给 03）
- 视频像素格式、RTP 打包
- 自行实现重采样 / AEC
