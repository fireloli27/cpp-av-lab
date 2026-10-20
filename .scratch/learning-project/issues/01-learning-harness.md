# 01: Learning harness 地基

**What to build:** macOS 上可编译的 Learning Project 地基：约定实验与短文的组织方式、CMake 能编出一个占位实验（非真实管线阶段），并写清「如何按短文步骤手动跑通」。不引入自动化测试框架；不建共享 C++ 库；不预建空的音/视阶段目录。实现本票时同步把约定落盘到本票 checklist/Comments，并更新 `spec-learning.md`（收口原未决 7/8/10/11）。

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

## Acceptance

- [x] `experiments/` 为 CMake 工程根（CMake ≥ 3.20，C++17，无 FetchContent/第三方）；`cmake -S experiments -B experiments/build && cmake --build experiments/build` 在 macOS 可配置并编译
- [x] 占位可执行目标 `learning_placeholder` 位于 `experiments/harness/placeholder/`；成功时在仓库根 cwd 下写出 `out/harness/placeholder-ok.txt`；不调媒体 API、不选采集/编解码库
- [x] 短文模板 `docs/learning/_template.md` 含固定栏：目标、前置、选用库、运行步骤、可观察成功判据、失败时仍记录什么、与主缝关系
- [x] 样例短文 `docs/learning/harness/placeholder.md` 按模板写实，演示手动跑通占位实验
- [x] harness 总览 `docs/learning/harness/README.md`：CMake 步骤、cwd＝仓库根、`out/` 约定、主缝一句话（每阶段各自入口 + 人工按短文验收）；不写排期、不拍板产品未决
- [x] 仓库根 `.gitignore` 忽略 `build/`、`experiments/build/`、`out/`、`.DS_Store`
- [x] 术语与根目录 `GLOSSARY.md` 一致；短文正文中文，路径/命令/目标名/GLOSSARY 术语不硬翻
- [x] 更新 `spec-learning.md`：原未决 7/8/10/11 标为已由本票收口；Comments 留指针

## 约定（grilling 2026-10-20）

- **阶段目录（02+ 再建，本票不预建空目录）**
  - 实验：`experiments/{audio|video}/{capture,encode,local-transport,decode,render}/`
  - 短文：`docs/learning/{audio|video}/` 同名 `.md`
- **占位独立于管线**：`experiments/harness/placeholder/` + `docs/learning/harness/placeholder.md`
- **主缝**：每阶段各自入口 + 人工按短文验收；不引入自动化测试框架；不做统一 pipeline runner
- **不抽** `experiments/common`；真出现第三次重复再另开票
- **非目标**：真实采集、产品未决（信令承载 / SFU 选型 / 部署等）

## Comments

- 2026-10-20: `/grill-me` 定稿布局、占位契约、CMake/C++ 基线、模板栏、gitignore、`out/` 与 cwd、短文语言；确认后落盘票面与 spec。
- 2026-10-20: `/implement` 落地 CMake 占位、`docs/learning` 模板/总览/样例短文、根 `.gitignore`；本机按短文跑通写出 `out/harness/placeholder-ok.txt`；Acceptance 勾选完成。
