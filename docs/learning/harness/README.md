# Learning harness 总览

Learning Project 的文档与实验地基。正式 Client AV SDK / Media Server 产品实现不在此范围。

## 主测试缝

每一 Learning Pipeline Stage（以及本 harness 占位）提供**各自可执行入口**；按对应短文步骤在 macOS 上手动跑通，以短文写明的可观察结果验收。不做统一 pipeline runner。

可自动化的可观察契约另由 **Catch2 + CTest** 护栏覆盖（见下）。需麦克风/摄像头等人工确认的判据仍以短文为准。

## 目录约定

| 用途 | 路径 |
| --- | --- |
| CMake 工程根 | `experiments/` |
| 构建目录 | `experiments/build/`（增量复用；迭代间不要删） |
| FetchContent 缓存 | `experiments/.fetchcontent/`（Catch2 源码；清 build 后可免重下） |
| 可观察产物 | 仓库根下 `out/`（运行时 **cwd＝仓库根**） |
| 短文根 / 模板 | `docs/learning/`、`docs/learning/_template.md`（怎么跑、怎么验收） |
| 课记模板 | `docs/learning/_notes_template.md`（概念与代码讲解） |
| 自动化测试 | `experiments/tests/`（Catch2） |
| 一键护栏 | `scripts/check-learning.sh` |
| 阶段实验（02+） | `experiments/{audio\|video}/{capture,encode,local-transport,decode,render}/` |
| 阶段短文（02+） | `docs/learning/{audio\|video}/` 同名 `.md` |
| 阶段课记（02+） | 同目录 `{stage}.notes.md`（与短文成对） |

术语以仓库根 `GLOSSARY.md` 为准。短文 / 课记：**正文中文**；路径、命令、目标名、GLOSSARY 术语与格式词（PCM、WAV、AudioQueue 等）**保留英文**（见根 `AGENTS.md` Doc language）。Agent 搜索忽略 `experiments/build/`、`experiments/.fetchcontent/`、`out/`（见 `.cursorignore`）。

**短文 vs 课记 vs teach：**

| 层 | 路径 | 权威写什么 |
| --- | --- | --- |
| 短文 | `docs/learning/{audio\|video}/{stage}.md` | 怎么跑、怎么验收（主缝） |
| 课记 | 同目录 `{stage}.notes.md` | **本阶段契约 + 代码地图**（改格式数字先改这里） |
| teach 教室 | `teach/`（见 [`teach/README.md`](../../../teach/README.md)） | 个人进修：MISSION / lessons / reference；**非** Learning 硬毕业交付 |

Learning Pipeline Stage 票 `/implement` 收官必写课记（六栏）；harness 占位不要求课记。契约与概念不要在短文/课记/教室三处各写一套：短文引用课记；`teach/lessons` 链课记；速查镜像课记并注明以课记为准。

## 配置与编译

在仓库根执行：

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build
```

要求：CMake ≥ 3.20，C++17。实验代码**无第三方媒体库**；**系统框架允许**（如 AudioToolbox / Core Audio）。测试经 FetchContent 拉取 Catch2。

## 自动化护栏

```bash
./scripts/check-learning.sh
```

等价于 configure + 并行 build + `ctest --test-dir experiments/build --output-on-failure`。该命令**不**访问麦克风/摄像头。

**Commit / CI：**

- 本机：`./scripts/install-git-hooks.sh`（设置 `core.hooksPath=scripts/githooks`，pre-commit 跑上述护栏）
- GitHub Actions：`.github/workflows/learning.yml`（macOS）跑同一脚本

**Agent 非沙箱（必读）：** Cursor agent 跑 `check-learning.sh`、或 `git commit`（触发 pre-commit）时须 `required_permissions: ["all"]`。原因：`experiments/build/` 与 `.fetchcontent/` 在 `.cursorignore` 里，沙箱常无法写入；且 Catch2 编码等测缝会调用系统 **AudioConverter** / **AudioFile**，沙箱内可能 `fmt?` 失败。CI 与本机终端不受此限。

## 人工媒体烟测（agent）

下列情况须在**非沙箱**跑（`required_permissions: ["all"]`）；`check-learning.sh` 不能代替听感/画面验收：

- 录麦 / 开摄像头的可执行文件（并已给终端麦克风/摄像头权限）
- 任意依赖 **AudioConverter** / **AudioFile** 编解码的本机烟测（例如 `learning_audio_encode` + `afplay`）

## 写 Opus CAF（导航）

音频编码产物约定见 [encode.notes.md](../audio/encode.notes.md)。实现要点：`AudioConverter` + `AudioFileWritePackets`；创建 CAF 时带 `kAudioFileFlags_DontPageAlignAudioData` 并写 magic cookie，否则易留下百 KB 级 `free` chunk。Catch2 对约 1 s 产物有体积上界，防回归。

## 跑通占位（验证地基）

见 [placeholder.md](./placeholder.md)。
