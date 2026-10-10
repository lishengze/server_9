# 复盘功能包（Review Kit）— 跨项目复用说明

> 本目录是 **Agent 复盘功能** 的可复用标准包。复制到任意项目即可启用"每日/每周/每月复盘 + 自我进化闭环"，无需改代码。

---

## 一、复盘功能包是什么

复盘机制解决一个问题：**Agent 是被动响应式的，无法自行唤醒做复盘**。本功能包用"cron 自动备数据 + 用户一句话触发 agent 反思"的半自动方案，让 Agent 定期沉淀经验、反哺知识库，形成自我进化闭环。

完整链路：

```
work_log（数据源）
   │  review_prepare.sh 聚合
   ▼
复盘输入文件（task/review/<周期>_<日期>_input.md）
   │  用户说"进行daily复盘" 触发
   ▼
agent 反思（核心五问 → 成果五转化）
   │
   ▼
知识库沉淀（memory/ + study/ + Prompt.md）+ 复盘报告
```

---

## 二、功能包包含哪些文件

| 文件 | 作用 | 是否必需 |
|------|------|:--------:|
| `task/review/review_prepare.sh` | 聚合 work_log，生成复盘输入 + 触发 prompt | ✅ 必需 |
| `task/review/README.md` | 本文档（依赖关系 + 部署 + cron） | 说明用 |
| `task/task_rule.md` | 工作规则：**§8.1 日志模板**（保证 work_log 格式可被聚合）+ **§7 复盘体系定义** | ✅ 必需 |
| `task/task_prompt.md` | **§7 复盘触发模板**（让 agent 理解"进行daily复盘"= 完整流程） | ✅ 必需 |
| `memory/self_evolution.md` | 固化复盘机制 + 一句话触发指令（每次会话自动加载） | ✅ 必需 |
| `work_log/` | 复盘数据源，文件格式 `YYYYMMDD.md` | ✅ 必需 |
| `memory/` + `study/knowledge_base_system/` + `Prompt.md` | 复盘成果五转化的**沉淀目标** | ✅ 必需 |
| cron 定时任务 | 自动备数据（兜底） | ⭕ 可选 |

> ⚠️ **关键点**：`review_prepare.sh` 靠 work_log 里的**固定标题**聚合数据（`## 任务`、`### 经验教训`、`### 结论`、`待办`/`遗留问题`）。因此 work_log 必须沿用 task_rule.md §8.1 的日志模板，否则脚本抓不到内容。

---

## 三、一键部署步骤（复制到新项目）

### 第 1 步：复制功能包文件

把以下文件/目录从本仓库复制到新项目对应位置（保持相对路径一致）：

```bash
# 在源项目根目录执行，TARGET 换成新项目根目录
TARGET=/path/to/新项目

mkdir -p "$TARGET/task/review" "$TARGET/memory" "$TARGET/study" "$TARGET/work_log"

# 复盘脚本 + 说明
cp task/review/review_prepare.sh  "$TARGET/task/review/"
cp task/review/README.md          "$TARGET/task/review/"

# 规则 + 模板 + 记忆
cp task/task_rule.md              "$TARGET/task/"
cp task/task_prompt.md            "$TARGET/task/"
cp memory/self_evolution.md       "$TARGET/memory/"

# 知识库骨架（成果沉淀目标，可按项目裁剪）
cp -r study/knowledge_base_system "$TARGET/study/"
```

### 第 2 步：校验脚本可用

```bash
cd "$TARGET"
bash task/review/review_prepare.sh daily   # 应输出"周期: daily"并生成 input/prompt 文件
```

> `review_prepare.sh` 用 `dirname $0` 相对定位 `work_log/`，**不硬编码路径**，复制后无需改代码。

### 第 3 步：让 agent 认知复盘指令

确保 `memory/self_evolution.md` 被项目的会话记忆机制自动加载（本仓库为 `MEMORY.md` 索引 + 每次会话注入），让任意会话的 agent 都理解"进行daily复盘"= 完整复盘流程。

### 第 4 步：配置 cron（可选，兜底自动备数据）

```bash
crontab -e
# 每日 18:00 准备当日复盘数据；每周五 18:00 周复盘；每月 28 日 18:00 月复盘
0 18 * * *  bash /path/to/新项目/task/review/review_prepare.sh daily
0 18 * * 5  bash /path/to/新项目/task/review/review_prepare.sh weekly
0 18 28 * * bash /path/to/新项目/task/review/review_prepare.sh monthly
```

> 即使不配 cron 也能用：agent 收到"进行daily复盘"时会**主动重跑脚本**刷新数据，cron 仅作兜底。

---

## 四、使用方式

### 一句话触发（推荐）

用户对 agent 说：

> **进行daily复盘**（或 weekly / monthly）

agent 收到后自动执行：
1. 主动运行 `bash task/review/review_prepare.sh <周期>` 刷新输入文件（防数据过时）
2. 执行复盘核心五问
3. 执行复盘成果五转化
4. 更新知识库 + Prompt.md
5. 写复盘报告到 `task/review/<周期>_<日期>_report.md`

### 手动触发

```bash
bash task/review/review_prepare.sh daily 2026-10-10   # 指定日期
```
然后把生成的 `task/review/daily_2026-10-10_prompt.txt` 内容复制给 agent。

---

## 五、复盘依赖关系图

```
                    ┌─────────────────────────────────────┐
                    │  用户说"进行daily复盘"（一句话触发）  │
                    └───────────────┬─────────────────────┘
                                    │
        ┌───────────────────────────▼───────────────────────────┐
        │  agent 反思执行（需 task_prompt.md §7 模板 + 能力）     │
        │  ① 核心五问  ② 成果五转化                              │
        └───────┬───────────────────────────────┬───────────────┘
                │                               │
                │ 读                        成果沉淀
                ▼                               ▼
   ┌──────────────────────┐        ┌──────────────────────────────┐
   │ 复盘输入文件          │        │ 知识库（memory/ + study/ +    │
   │ task/review/xxx.md   │        │        Prompt.md）            │
   └──────────▲───────────┘        └──────────────────────────────┘
              │ 生成
   ┌──────────┴───────────┐
   │ review_prepare.sh    │  ← cron 可定时触发（兜底）
   │ （聚合 work_log）     │
   └──────────▲───────────┘
              │ 读取
   ┌──────────┴───────────┐
   │ work_log/YYYYMMDD.md │  ← 格式须符合 task_rule.md §8.1 模板
   └──────────────────────┘
```

---

## 六、常见问题

**Q1：只复制 review_prepare.sh + work_log 够吗？**
不够。缺 task_prompt.md §7（agent 不知道怎么复盘）和知识库（成果没地方沉淀），复盘会变成"只出报告、不反哺"。

**Q2：work_log 格式必须一样吗？**
必须。脚本靠 `## 任务`/`### 经验教训`/`### 结论`/`待办` 等固定标题聚合，格式不同则抓不到内容。

**Q3：不配 cron 能用吗？**
能。agent 收到"进行daily复盘"会主动重跑脚本刷新数据；cron 只是自动备数据的兜底。

**Q4：知识库目录可以裁剪吗？**
可以。若新项目已有自己的知识库，只需保证复盘成果有地方沉淀（改脚本或改 agent 的沉淀目标即可），`study/knowledge_base_system` 骨架可按需精简。

---

## 附：本功能包在 otc_src 仓库的原始出处
- 脚本：`task/review/review_prepare.sh`
- 规则：`task/task_rule.md`（§7 复盘体系、§8.1 日志模板）
- 模板：`task/task_prompt.md`（§7 复盘触发模板）
- 记忆：`memory/self_evolution.md`
- 方法论：`task/task_upgrade.md`（§7.4 复盘自动化分析）