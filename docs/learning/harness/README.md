# Learning harness 总览

Learning Project 的文档与实验地基。正式 Client AV SDK / Media Server 产品实现不在此范围。

## 主测试缝

每一 Learning Pipeline Stage（以及本 harness 占位）提供**各自可执行入口**；按对应短文步骤在 macOS 上手动跑通，以短文写明的可观察结果验收。不引入自动化测试框架；不做统一 pipeline runner。

## 目录约定

| 用途 | 路径 |
| --- | --- |
| CMake 工程根 | `experiments/` |
| 构建目录 | `experiments/build/` |
| 可观察产物 | 仓库根下 `out/`（运行时 **cwd＝仓库根**） |
| 短文根 / 模板 | `docs/learning/`、`docs/learning/_template.md` |
| 阶段实验（02+） | `experiments/{audio\|video}/{capture,encode,local-transport,decode,render}/` |
| 阶段短文（02+） | `docs/learning/{audio\|video}/` 同名 `.md` |

术语以仓库根 `GLOSSARY.md` 为准。

## 配置与编译

在仓库根执行：

```bash
cmake -S experiments -B experiments/build
cmake --build experiments/build
```

要求：CMake ≥ 3.20，C++17。当前地基无第三方依赖。

## 跑通占位（验证地基）

见 [placeholder.md](./placeholder.md)。
