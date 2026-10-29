# NewAPI 系统级知识库入口索引

> 本目录为 **NewAPI 交易 API 框架** 的系统级知识库（L1，工程/系统维度），由原 `study/knowledge_base` 迁移融合而来。
> 统一入口：本文件；统一引导：`Prompt.md`。

## 导航

| 文件 | 定位 | 触发分支 |
|------|------|---------|
| `NewAPI_知识库.md` | **知识库主文档**（§1~§41，架构/设计/数据流/性能/排障/待办） | 任何 NewAPI 架构、协议、性能、排障问题 |
| `Prompt.md` | **引导 Prompt**（角色/加载指引/分析框架/回答风格） | 新会话加载知识库前先读此文件 |
| `体系总览_工作规范与知识库体系.md` | **元层文档**：工作规范 / 自我进化 / 知识库体系 + 复盘运行手册 + 本机环境缺口清单 | 需要知道"按什么规矩干活、学到的东西放哪、复盘怎么跑、本机有什么坑"时 |
| `MEMORY.md` | 本索引 | 入口 |

## 核心结论速查（体系层）

- **一次任务三份产出**：任务交付物 + work_log 条目 + 提炼后的知识条目，缺一不可。
- **复盘一句话触发**：说"进行 daily/weekly/monthly 复盘" → 先跑 `review_prepare.sh` 刷新输入（本机须用 Git bash）→ 核心五问 → 成果五转化 → 更新知识库/`Prompt.md` → 写报告 → 知识库融合核查。
- **本机（Windows）只能做文档类工作**：无 docker/svn/WSL，项目依赖 `sys/epoll.h` 无法用 mingw 编译；编译/回归/性能测试需 Linux 主机或 WSL2+docker。

## 知识库两级维度（融合方案）
- **系统级（本目录）**：回答"怎么在系统上干活"——架构、开发、编译、部署、测试、排障、性能调优。由 `NewAPI_知识库.md` + `Prompt.md` 承载。
- **业务级**：若后续产生独立业务知识（如各柜台具体业务细节），可新建 `study/knowledge_base/` 承载，并在本索引增加导航。

## 使用流程
1. 新会话先读 `Prompt.md`（角色 + 加载指引 + 分析框架）。
2. 再读 `NewAPI_知识库.md`（按目录定位到具体章节）。
3. 需要工作规范/复盘/环境事实时读 `体系总览_工作规范与知识库体系.md`。
4. 记忆速查见会话记忆目录 `MEMORY.md`（本机：`C:\Users\lsz-work\.chat\projects\code-work-api_trunk\memory\MEMORY.md`；旧主机为 `/home/lsz/.chat/projects/home-lsz-code-work-api_trunk/memory/MEMORY.md`）。