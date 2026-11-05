#!/usr/bin/env bash
# =============================================================================
# sync_rules.sh — 规范同步器（消除 task_rule.md 与 project_rules.md 双份维护漂移）
#
# 背景：task/task_rule.md 是唯一权威源；.chat/rules/project_rules.md 是面向所有
#       agent/会话的副本。历史上两处手工双写导致副本缺规则（§6 问题解决准则、
#       §8.4 知识库融合核查）。本脚本让副本"生成而非手写"。
#
# 用法：
#   bash task/sync_rules.sh        # 从 task_rule.md 生成/更新 project_rules.md
#   bash task/sync_rules.sh -c     # 只校验是否同步（退出码 0=一致，1=不一致）
#   bash task/sync_rules.sh -h     # 帮助
#
# 规则：改规范 → 只改 task/task_rule.md → 跑本脚本 → 自检清单核对。
# =============================================================================
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/task/task_rule.md"
DST="$ROOT/.chat/rules/project_rules.md"

if [ "${1:-}" = "-h" ]; then sed -n '2,16p' "$0"; exit 0; fi
[ -f "$SRC" ] || { echo "✗ 缺少权威源: $SRC"; exit 1; }

render() {
    echo "<!-- 本文件由 task/sync_rules.sh 从 task/task_rule.md 自动生成，请勿手改。"
    echo "     修改规范请改 task/task_rule.md，然后执行: bash task/sync_rules.sh -->"
    echo
    sed 's|^> \*\*单一事实来源\*\*.*$|> **本文件是自动生成的副本**：权威源为 `task/task_rule.md`，修改请改源头后执行 `bash task/sync_rules.sh`。|' "$SRC"
}

if [ "${1:-}" = "-c" ]; then
    if diff -q <(render) "$DST" >/dev/null 2>&1; then
        echo "✓ 规范已同步（task_rule.md == project_rules.md 正文）"
        exit 0
    else
        echo "✗ 规范不同步！请执行: bash task/sync_rules.sh"
        diff <(render) "$DST" | head -20
        exit 1
    fi
fi

mkdir -p "$(dirname "$DST")"
render > "$DST"
echo "✓ 已同步: $SRC"
echo "        -> $DST"
echo "  正文行数: $(wc -l < "$DST")"
