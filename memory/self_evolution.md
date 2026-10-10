# Agent 自我进化体系（方法论）

> 完整方案见 `task/task_upgrade.md`。本节为可复用的核心结论。

## 核心思想
把每一次任务变成一次"学习 + 沉淀 + 反哺"的机会，形成自我强化的封闭闭环。
**一次任务，三份产出**：① 任务交付物；② 工作日志条目；③ 提炼后的知识条目。

## 三层记忆模型
| 记忆层 | 类比 | 内容 | 位置 |
|--------|------|------|------|
| 工作记忆 | 短期 | 当前任务上下文/临时状态 | 对话上下文 + add_tasks |
| 情景记忆 | 经历 | 每次任务做了什么/遇到什么/怎么解决 | work_log/YYYYMMDD.md |
| 语义记忆 | 知识 | 提炼的模式/架构/坑点/规则 | memory/*.md + study/knowledge_base_system/ |
| 程序记忆 | 技能 | 怎么做某类事的标准流程 | Prompt.md + 执行规范 |

## 知识生命周期
捕获 → 验证（根因明确/可复现）→ 提炼（去噪/抽象/命名）→ 索引（主题文件+更新 MEMORY.md）→ 应用（检索复用）→ 保鲜（更新/合并/淘汰）。
**禁止把未经证实的猜测写进知识库**；宁精勿滥，只沉淀可复用、有区分度的内容。

## 任务五阶段
接单分类 → 规划（add_tasks + 先检索知识库/历史排障）→ 执行验证（改动前读文件、必须验证）→ 复查（多轮，直到某轮零修改）→ 沉淀提交（工作日志+知识+Prompt.md+git+关 docker）。

## 三级提炼法（经验→知识）
原始经验(work_log) → 去噪 → 抽象(现象→根因→解法,给名字) → 索引(主题文件+MEMORY.md+Prompt.md)。

## 索引优先
MEMORY.md 是知识库的"搜索引擎"，必须精简正确最新（>200 行会被截断）。详细笔记放主题文件，MEMORY.md 只放：主题导航表 + 核心结论速查 + 关键路径速查 + 工作准则。

## 错误驱动学习（最重要）
每次报错/失败/返工都是最高价值学习素材：记录报错原文 → 定位根因(看日志/复现,不猜测) → 解法并验证 → 沉淀为可检索知识。

## 跨会话知识迁移（核心机制）
知识不随会话结束丢失，沉淀在仓库里，通过 **Prompt.md** 被下一会话的 Agent 重新加载。每次沉淀后必须同步更新 Prompt.md 的学习路径/学习要求/学习完成标准。

## 量化改进指标
重复问题解决率、知识复用命中率、复查轮次、排障耗时、返工率。

## 外部工具建议
Obsidian(知识图谱)、ripgrep(全文检索)、Mermaid(图表)、知识保鲜脚本(扫描超期未更新主题)。

## 规则与模板（2026-10-09 新增）
- **通用规则**：`task/task_rule.md`（融合 task_all.txt 1-15 行 + 自进化体系），同时写入 `.chat/rules/project_rules.md` 面向所有 agent/会话。
- **对话模板**：`task/task_prompt.md`（六大工作场景：学习文档/研究代码/开发测试/测试执行/代码答疑/问题分析）。
- **工具已安装**：rg/fd/bat/fzf/tree/tmux/pandoc/jq/mineru（安装脚本 `task/tool/ai_tool.sh`，用法 `task/tool/ai_tool_use.md`）。mmdc(按需 npx)。知识检索助手 `task/tool/kb_search.sh`。
- **技能**：pdf/docx/xlsx/pptx/frontend-design/webapp-testing/canvas-design/mcp-builder/skill-creator 等，详见 `study/knowledge_base_system/10_技能与工具.md`。
- **知识库融合**：`knowledge_base`(业务级) + `knowledge_base_system`(系统级) = 统一知识体系的两级维度，详见 `task/task_upgrade.md` §9。
- **四级复盘体系**（参考 agent_upgrade.md）：单任务即时复盘→每日总结→每周迭代→每月能力升级；复盘核心五问 + 复盘成果五转化。详见 `task/task_rule.md` §7。
- **三层学习机制**：任务即时学习 / 短板靶向学习 / 常态化新知学习 + 6 步学习 SOP。详见 `task/task_rule.md` §6。
- **分场景专项规范**：文档/代码开发/测试运维各有专项 SOP。详见 `task/task_rule.md` §8。
- **工程纪律**（参考 lsz-skills-real-engineers）：`study/knowledge_base_system/11_工程纪律.md`。核心：TDD红绿重构、六阶段Bug诊断（先建反馈回路→再假设→再修复）、双轴CodeReview+Fowler坏味道、DeepModule设计、writing-for-agents文档规范、领域建模GLOSSARY+ADR。已融入 `task/task_rule.md`（§3双轴审查/§4.1文档写作/§5.1Bug诊断）。
- **复盘自动化**（cron 半自动）：Agent 被动响应式，无法自醒做日/周/月复盘（无 headless CLI）。方案= cron 跑 `task/review/review_prepare.sh` 自动聚合 work_log 生成复盘输入，用户一键触发 agent 反思。单任务复盘全自动；日/周/月复盘半自动。详见 `task/task_rule.md` §7.4、`task/task_upgrade.md` §7.4。
  - **一句话触发指令**：用户说"进行daily/weekly/monthly复盘"即可。agent 收到后**先主动运行 `bash task/review/review_prepare.sh <周期>` 刷新输入文件**（不依赖 cron 预生成，防过时），再执行核心五问→成果五转化→更新知识库+Prompt.md→写报告。详见 `task/task_prompt.md` §7。