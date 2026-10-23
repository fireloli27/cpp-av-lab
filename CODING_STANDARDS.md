# Coding standards

Judgement calls for `/code-review`（Standards 轴）。机械规则进护栏（Catch2 / `check-learning.sh`），不写在这里。

## Learning Pipeline Stage：API 名与实现一致

短文「选用库」、课记「关键 API / 决策」、公开头文件注释里点名的系统 API / 库，须与最终 `.cpp` 实际调用的符号同名。中途换实现（例如 ExtAudioFile → `AudioFileWritePackets`）后，上述三处未同步视为未完成——不是风格偏好，是读者会按错 API 排查。

## Learning Pipeline Stage：短文成功日志字段与 stdout 一致

短文「可观察成功判据」里用反引号标出的日志字段名（如 `units_sent`、`payload_bytes_recv`），须出现在该阶段可执行文件**成功路径**的 stdout 中（同名 token）。判据写了发送与接收两侧时，两侧字段都要打出，不能只打一个合计名冒充。字段语义以课记契约为准；本条管的是「短文承诺的 token 能否在日志里对上」。
