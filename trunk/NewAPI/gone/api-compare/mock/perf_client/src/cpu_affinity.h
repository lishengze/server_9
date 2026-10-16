// cpu_affinity.h - CPU 隔离/绑定（通用）
//
// 职责：为性能测试进程提供 CPU 亲和性操作，确保测试进程独占/绑定到指定 CPU，
//   避免被调度到其他核导致测量抖动。
//   - bind_cpu(): 用 sched_setaffinity 将当前进程绑定到单个 CPU
//   - get_current_cpu(): 查询当前进程允许运行的 CPU 列表（用于日志确认已绑核）
//   - 通用化：cpu_id 通过配置传入，可在不同服务器上指定不同的核。
//
// 注意：CPU 隔离（isolcpus / cpuset / 关闭其他任务）通常需要 root 权限，
//   可在进程外由 scripts/cpu_affinity.sh 完成；进程内 bind_cpu() 负责绑定自身。

#pragma once

#include <string>

namespace perf {

namespace cpu_affinity {

/// 绑定当前进程到指定 CPU。
/// @param cpu_id 目标 CPU 编号；<0 表示不绑定（返回 true）。
/// @return 绑定成功返回 true；失败返回 false。
bool bind_cpu(int cpu_id);

/// 查询当前进程允许运行的 CPU 列表，格式如 "0,2,4"。
/// 失败返回 "unknown"。
std::string get_current_cpu();

} // namespace cpu_affinity

} // namespace perf