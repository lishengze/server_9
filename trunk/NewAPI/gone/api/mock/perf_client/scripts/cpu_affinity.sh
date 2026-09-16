#!/usr/bin/env bash
# cpu_affinity.sh - CPU 隔离 / 绑定通用脚本
#
# 用途：为性能测试进程提供 CPU 亲和性控制，确保测试进程独占/绑定到指定 CPU，
#       避免被调度到其他核导致测量抖动，保证性能测试结果准确。
#
# 通用性：CPU 编号 / 进程 PID / 命令均可通过参数指定，可在不同服务器上复用。
#
# 用法：
#   1) 绑定已运行的进程（按 PID）：
#        cpu_affinity.sh bind <pid> <cpu_id>
#        例：cpu_affinity.sh bind 12345 0
#
#   2) 在指定 CPU 上启动并运行命令（绑定后执行）：
#        cpu_affinity.sh run <cpu_id> -- <command...>
#        例：cpu_affinity.sh run 0 -- ./perf_client --config perf_config.json
#
#   3) 隔离 CPU（将指定 CPU 从系统调度中隔离，需 root）：
#        cpu_affinity.sh isolate <cpu_id>
#        例：cpu_affinity.sh isolate 0
#        注：隔离后该 CPU 不再参与默认调度，需用 taskset/本脚本绑定进程到它。
#
# 说明：
#   - 绑定采用 taskset（sched_setaffinity 的 shell 封装）。
#   - 隔离 CPU 需 root 权限，且依赖内核 cpuset/isolcpus 支持。
#   - 建议：先用 isolate 隔离目标核，再用 run/bind 把测试进程绑定上去。

set -euo pipefail

usage() {
    cat <<'EOF'
用法:
  cpu_affinity.sh bind <pid> <cpu_id>       绑定运行中的进程到指定 CPU
  cpu_affinity.sh run <cpu_id> -- <cmd...>  在指定 CPU 上启动并运行命令
  cpu_affinity.sh isolate <cpu_id>          隔离指定 CPU（需 root）
  cpu_affinity.sh status <pid>              查看进程当前 CPU 亲和性
EOF
    exit 1
}

# 校验 CPU 编号
check_cpu() {
    local cpu="$1"
    case "$cpu" in
        ''|*[!0-9]*) echo "[ERROR] 无效 CPU 编号: $cpu" >&2; exit 1 ;;
    esac
}

cmd_bind() {
    local pid="$1" cpu="$2"
    check_cpu "$cpu"
    if ! kill -0 "$pid" 2>/dev/null; then
        echo "[ERROR] 进程不存在: $pid" >&2
        exit 1
    fi
    taskset -pc "$cpu" "$pid"
    echo "[OK] 进程 $pid 已绑定到 CPU $cpu"
    taskset -pc "$pid"
}

cmd_run() {
    local cpu="$1"
    check_cpu "$cpu"
    shift
    # 去掉 "--" 分隔符
    [ "${1:-}" = "--" ] && shift
    echo "[INFO] 在 CPU $cpu 上执行: $*"
    taskset -c "$cpu" "$@"
}

cmd_isolate() {
    local cpu="$1"
    check_cpu "$cpu"
    local cpuset_dir="/sys/fs/cgroup/cpuset"
    if [ ! -d "$cpuset_dir" ]; then
        echo "[WARN] 未检测到 cpuset（$cpuset_dir），尝试 isolcpus 内核参数方式。" >&2
        echo "[INFO] 请确认内核启动参数含 isolcpus=$cpu，或手动配置 cpuset。" >&2
        exit 0
    fi
    # 在 cpuset 中创建一个 exclusive 子集，仅包含该 CPU
    local dir="$cpuset_dir/perf_isolated"
    [ -d "$dir" ] && rmdir "$dir" 2>/dev/null || true
    mkdir -p "$dir"
    echo "$cpu" > "$dir/cpuset.cpus"
    echo 1 > "$dir/cpuset.cpu_exclusive"
    # 将当前 shell 移入该子集（使后续在此 shell 启动的进程使用该 CPU）
    echo $$ > "$dir/tasks"
    echo "[OK] 已隔离 CPU $cpu，当前 shell 已移入专属 cpuset"
}

cmd_status() {
    local pid="$1"
    taskset -pc "$pid"
}

[ $# -lt 1 ] && usage
cmd="$1"; shift

case "$cmd" in
    bind)    [ $# -eq 2 ] || usage; cmd_bind "$1" "$2" ;;
    run)     [ $# -ge 2 ] || usage; cmd_run "$@" ;;
    isolate) [ $# -eq 1 ] || usage; cmd_isolate "$1" ;;
    status)  [ $# -eq 1 ] || usage; cmd_status "$1" ;;
    *)       usage ;;
esac