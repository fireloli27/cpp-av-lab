# Coding standards

Judgement calls for `/code-review`（Standards 轴）。机械规则进护栏（Catch2 / `check-learning.sh`），不写在这里。

## Learning Pipeline Stage：API 名与实现一致

短文「选用库」、课记「关键 API / 决策」、公开头文件注释里点名的系统 API / 库，须与最终 `.cpp` 实际调用的符号同名。中途换实现（例如 ExtAudioFile → `AudioFileWritePackets`）后，上述三处未同步视为未完成——不是风格偏好，是读者会按错 API 排查。
