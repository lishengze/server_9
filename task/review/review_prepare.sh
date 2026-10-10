#!/usr/bin/env bash
# =============================================================================
# review_prepare.sh — 复盘数据准备脚本（cron 驱动，半自动复盘）
#
# 背景：Agent 是被动响应式，无法自行唤醒做每日/每周/每月复盘。
#       本脚本由 cron 定时触发，负责"数据准备"这一可自动化的部分：
#         1. 聚合 work_log/ 对应周期的条目
#         2. 统计任务数、提取"经验教训"、"结论"、"待验证假设"
#         3. 生成复盘输入文件 task/review/<周期>_<日期>_input.md
#         4. 生成一个"复盘触发 prompt"文件，供用户一键复制给 agent
#       用户随后用固定复盘 prompt 触发 agent 执行真正的"反思"环节
#       （分析模式、更新知识库、更新 Prompt.md、写复盘报告）。
#
# 用法：
#   bash review_prepare.sh daily    # 聚合今天（或当前日期前一天）的日志
#   bash review_prepare.sh weekly   # 聚合最近 7 天
#   bash review_prepare.sh monthly  # 聚合最近 30 天
#   bash review_prepare.sh daily 2026-10-09   # 指定日期
#   bash review_prepare.sh -h       # 帮助
#
# cron 示例（写入 crontab -e）：
#   # 每日 18:00 准备当日复盘数据
#   0 18 * * *  bash /home/lsz/code/work/otc_src/task/review/review_prepare.sh daily
#   # 每周五 18:00 准备周复盘数据
#   0 18 * * 5  bash /home/lsz/code/work/otc_src/task/review/review_prepare.sh weekly
#   # 每月最后一天 18:00 准备月复盘数据（用 28 号近似，或单独脚本算最后一天）
#   0 18 28 * * bash /home/lsz/code/work/otc_src/task/review/review_prepare.sh monthly
# =============================================================================
set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORKLOG="$ROOT/work_log"
REVIEWDIR="$ROOT/task/review"
mkdir -p "$REVIEWDIR"

usage() { sed -n '1,35p' "$0" | grep -E '^#|用法|cron 示例' ; exit 0; }

PERIOD="${1:-daily}"
if [ "$PERIOD" = "-h" ]; then usage; fi

# 计算日期范围
case "$PERIOD" in
    daily)
        END_DATE="${2:-$(date +%F)}"
        START_DATE="$END_DATE"
        ;;
    weekly)
        END_DATE="${2:-$(date +%F)}"
        START_DATE="$(date -d "$END_DATE -6 days" +%F)"
        ;;
    monthly)
        END_DATE="${2:-$(date +%F)}"
        START_DATE="$(date -d "$END_DATE -29 days" +%F)"
        ;;
    *)
        echo "未知周期: $PERIOD (可用 daily/weekly/monthly)"; exit 1;;
esac

OUTFILE="$REVIEWDIR/${PERIOD}_${END_DATE}_input.md"
TRIGGER="$REVIEWDIR/${PERIOD}_${END_DATE}_prompt.txt"

# 收集周期内的日志文件
FILES=()
for d in $(seq 0 30); do
    fdate="$(date -d "$START_DATE +$d days" +%Y%m%d)"
    [ "$(date -d "$fdate" +%F)" \> "$END_DATE" ] && break
    f="$WORKLOG/$fdate.md"
    [ -f "$f" ] && FILES+=("$f")
done

echo "周期: ${PERIOD} (${START_DATE} ~ ${END_DATE})"
echo "日志文件数: ${#FILES[@]}"

{
    echo "# ${PERIOD} 复盘输入（${START_DATE} ~ ${END_DATE}）"
    echo
    echo "> 由 review_prepare.sh 自动生成，供 agent 执行复盘。"
    echo "> 统计信息："
    echo "- 覆盖日志文件：${#FILES[@]} 个"
    echo "- 任务条目：$(grep -hc '^## 任务' "${FILES[@]}" 2>/dev/null | awk '{s+=$1} END{print s+0}')"
    echo "- 经验教训条目：$(grep -hc '^### 经验教训' "${FILES[@]}" 2>/dev/null | awk '{s+=$1} END{print s+0}')"
    echo
    echo "## 一、任务清单"
    echo
    for f in "${FILES[@]}"; do
        echo "### 来源：$(basename "$f")"
        grep -n '^## 任务' "$f" 2>/dev/null | sed 's/^/  /'
        echo
    done
    echo "## 二、经验教训汇总（反思素材）"
    echo
    for f in "${FILES[@]}"; do
        echo "### 来源：$(basename "$f")"
        awk '/^### 经验教训/{flag=1;next}/^## /{if(flag)flag=0}flag' "$f" 2>/dev/null | grep -v '^### 经验教训' | sed 's/^/  /'
        echo
    done
    echo "## 三、待验证假设 / 遗留问题（若有）"
    echo
    for f in "${FILES[@]}"; do
        awk '/待验证假设|遗留问题|待办/{flag=1;next}/^## /{if(flag)flag=0}flag' "$f" 2>/dev/null | grep -vE '待验证假设|遗留问题|待办' | sed 's/^/  /'
        echo
    done
} > "$OUTFILE"

# 生成复盘触发 prompt
cat > "$TRIGGER" <<EOF
【复盘触发】请对 ${START_DATE}~${END_DATE} 的 ${PERIOD} 工作进行复盘。

复盘输入已生成在：task/review/${PERIOD}_${END_DATE}_input.md
请读取该文件，执行复盘核心五问：
1. 各任务是否达标？有哪些瑕疵？
2. 哪些步骤高效、可固化复用？
3. 出现了哪些报错/卡点/低效问题？根因是什么？
4. 下次同类任务如何优化流程、规避问题？
5. 需要新增/优化哪些 Skill、知识库条目？

然后执行复盘成果五转化：
- 高效经验 → 标准化 Skill/流程模板
- 报错踩坑 → 问题解决方案知识库
- 流程瑕疵 → 优化通用工作 SOP
- 能力短板 → 靶向学习清单
- 优质案例 → 标杆案例库

最后：更新知识库 + Prompt.md，写复盘报告到 task/review/${PERIOD}_${END_DATE}_report.md。
EOF

echo "已生成："
echo "  复盘输入: $OUTFILE"
echo "  触发prompt: $TRIGGER"
echo
echo "下一步：复制 $TRIGGER 内容给 agent，或直接对 agent 说："
echo "  '进行${PERIOD}复盘，输入在 task/review/${PERIOD}_${END_DATE}_input.md'"