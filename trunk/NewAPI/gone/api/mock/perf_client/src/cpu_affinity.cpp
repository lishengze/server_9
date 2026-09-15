// cpu_affinity.cpp - CPU 隔离/绑定实现

#include "cpu_affinity.h"

#include <sched.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

namespace perf {
namespace cpu_affinity {

bool bind_cpu(int cpu_id) {
  if (cpu_id < 0) {
    // 未配置绑定目标，跳过
    return true;
  }

  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(static_cast<int>(cpu_id), &set);

  if (sched_setaffinity(0, sizeof(set), &set) != 0) {
    return false;
  }
  return true;
}

std::string get_current_cpu() {
  cpu_set_t set;
  CPU_ZERO(&set);
  if (sched_getaffinity(0, sizeof(set), &set) != 0) {
    return "unknown";
  }

  std::ostringstream oss;
  bool first = true;
  for (int i = 0; i < CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &set)) {
      if (!first) {
        oss << ",";
      }
      oss << i;
      first = false;
    }
  }
  return oss.str();
}

} // namespace cpu_affinity
} // namespace perf