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
| 短文根 / 模板 | `docs/learning/`、`docs/learning/_template.md` |
| 自动化测试 | `experiments/tests/`（Catch2） |
| 一键护栏 | `scripts/check-learning.sh` |
| 阶段实验（02+） | `experiments/{audio\|video}/{capture,encode,local-transport,decode,render}/` |
| 阶段短文（02+） | `docs/learning/{audio\|video}/` 同名 `.md` |

术语以仓库根 `GLOSSARY.md` 为准。Agent 搜索忽略 `experiments/build/`、`experiments/.fetchcontent/`、`out/`（见 `.cursorignore`）。

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

## 人工媒体烟测（agent）

录麦 / 开摄像头的可执行文件须在**非沙箱**环境跑（Cursor agent：`required_permissions: ["all"]`），并已给终端麦克风/摄像头权限。`check-learning.sh` 不能代替这一步。

## 跑通占位（验证地基）

见 [placeholder.md](./placeholder.md)。
