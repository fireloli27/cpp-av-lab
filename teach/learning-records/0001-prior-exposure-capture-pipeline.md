# Prior exposure: capture writes WAV containing PCM

User has already shipped Learning Pipeline「采集」: AudioQueue → `vector<int16_t>` → `write_capture_wav` → `out/audio/capture/capture.wav`, plus Catch2 on WAV headers. They asked for conceptual PCM/WAV teaching, which implies **exposure without fluent distinction** (container vs sample layout, and G.711 vs linear PCM).

**Implications:** First lesson should nail PCM≠WAV and byte-rate arithmetic against the repo contract; skip re-teaching how to run `learning_audio_capture`.
