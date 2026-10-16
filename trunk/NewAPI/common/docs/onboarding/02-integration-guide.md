---
purpose: INTEGRATION-GUIDE
audience: AI agents and humans integrating lbcommon into a business project
source-of-truth: yes (for CMake/link/namespace conventions)
last-reviewed: 2026-08-31
---

# Integration Guide：把 lbcommon 接入业务项目

> 业务项目与 `common/` 仓库同级，通过 CMake `add_subdirectory` 把 lbcommon 作为子模块静态链接。
> 本文件给出**已验证**的接入模板与避坑。

## 1. 推荐目录结构

```
projects/
├── common/             ← lbcommon 仓库（git clone）
│   ├── CMakeLists.txt
│   ├── include/
│   ├── src/
│   └── lib/liblbcommon.a   (构建产物)
├── my_app/             ← 业务项目
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── ...
```

> **关键点**：`common/` 和业务项目**同级目录**，`add_subdirectory(../common ...)` 用**相对路径**。不要把 `common/` 复制到业务项目里——会丢版本控制。

## 2. CMake 模板（业务项目）

### 2.1 最简可工作版本

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入 lbcommon（用相对路径）
add_subdirectory(../common ${CMAKE_BINARY_DIR}/lbcommon_build)

add_executable(my_app main.cpp)

# 关键：链接 lbcommon（自动带入头文件路径与 pthread/rt/dl）
target_link_libraries(my_app PRIVATE lbcommon)

# 推荐：与 lbcommon 一致的编译标志（避免 O 级别不一致触发 UB）
target_compile_options(my_app PRIVATE
  -march=native -m64 -D_LINUX -DLINUX -Wall -Wextra)
```

### 2.2 关键机制

| 机制 | 来源 | 作用 |
|------|------|------|
| `target_include_directories(lbcommon PUBLIC include/)` | common/CMakeLists.txt | 自动给业务项目加 `-I../common/include` |
| `target_link_libraries(lbcommon PUBLIC pthread rt dl)` | common/CMakeLists.txt | 自动链接系统库 |
| `add_subdirectory` 自动传播 | CMake | 头文件路径与系统库依赖随 `target_link_libraries(my_app lbcommon)` 自动传递 |

### 2.3 不需要做的事

| 不要做 | 原因 |
|--------|------|
| ❌ `include_directories(../common/include)` | 已通过 target 传递 |
| ❌ 显式 `target_link_libraries(my_app pthread rt dl)` | 已通过 lbcommon PUBLIC 传递 |
| ❌ 自己 `add_library` lbcommon | 会与子目录冲突 |
| ❌ 把 `common/lib/liblbcommon.a` 直接 `target_link_libraries` | 会丢头文件路径 |
| ❌ 复制 `common/include` 到业务项目 | 丢版本控制；更新 lbcommon 不方便 |

## 3. 命名空间

**所有 lbcommon 类型都在 `lb_common` 命名空间内**。业务代码三种合法写法：

| 写法 | 风格 | 适用 |
|------|------|------|
| `using namespace lb_common;` | 一次性引入 | 单文件业务 |
| `using lb_common::tcp_buf_ch;` | 选择性引入 | 中型项目 |
| `lb_common::tcp_buf_ch ch;` | 全限定 | 大型项目/避免冲突 |

**注意**：`lb_common` 是 lbcommon 仓库约定的命名空间前缀，**不要改写或省略**。

## 4. 链接与 ABI 注意

### 4.1 静态库（默认）

`lbcommon` 默认编译为**静态库** `lib/liblbcommon.a`。业务项目链接后：
- ✅ 业务可执行文件是单个二进制，部署简单
- ⚠️ 多个业务项目链接同一 lbcommon 不会冲突（每个可执行文件自带一份）

### 4.2 共享库（按需）

如需 `.so`：

```cmake
# 在 common/CMakeLists.txt 中：
add_library(lbcommon SHARED ${COMMON_SRC})   # 而不是 STATIC
```

需要注意：
- 当前 `add_library(lbcommon STATIC ...)` 默认开了 `-fPIC`（见 common/CMakeLists.txt L56），可直接改成 SHARED
- ABI 不保证稳定——升级 lbcommon 后业务项目**必须重新编译**
- 不要在多个动态库中混合不同版本的 lbcommon

### 4.3 ABI 约束（共享内存场景）

跨进程通信使用 `shmem_pool`、`shm_block_pool`、`shm_data_addr` 时：

> **强制约束**：`shm_data_addr` 的内存布局是 `area_id → data_id → block_id`（结构体声明顺序）。**所有进程必须用同一份 lbcommon 头文件编译**，否则跨进程寻址错乱。

详见 [内存池.md §5.3](../使用范式/内存池.md#53-三级寻址-shm_data_addr)。

## 5. 编译选项对齐

业务项目**应当**继承 lbcommon 的关键编译选项，避免 O 级别/优化不一致触发 UB：

| 选项 | 值 | 原因 |
|------|-----|------|
| `-march=native` | 必须 | lbcommon 用 SIMD（`_mm_*`）需同指令集 |
| `-m64` | 必须 | 显式 64 位（lbcommon 内有 int64 位运算） |
| `-D_LINUX -DLINUX` | 必须 | lbcommon 内部条件编译依赖 |
| `-O2` 或 `-O3` | 必须 | `atomic_*` 内置在 `-O0` 下未优化 |
| `-std=c++17` | 必须 | lbcommon 用到 inline variable / structured binding |

> 业务项目用不同 `-march` 编译会导致 lbcommon 的内联函数指令集不一致，触发 illegal instruction 或 UB。**业务项目与 lbcommon 必须用相同 `-march`**。

## 6. 信号处理与全局停止

`lbcommon` 多个线程循环会查全局变量 `g_signal_stopctl`。**主线程应早调**：

```cpp
#include <thread_comm.h>

int main() {
  lb_common::install_signal_hand();   // SIGTERM/SIGQUIT → 优雅停止
  // ... 业务初始化 ...
}
```

详见 [线程.md §6](../使用范式/线程.md#6-生命周期统一)。

## 7. 日志集成（可选）

`lb_log` 提供同步/异步日志。最小集成：

```cpp
#include <mlog.h>

lb_common::lb_log g_log;
lb_common::lb_log_hand g_hand;

int main() {
  lb_common::lb_log_attr lattr{};
  lattr.logdir = "./log";      // 日志目录
  lattr.lognamemax = 100;      // 单文件大小 MB
  lattr.loglevel = 1;          // 1=INFO 2=WARN 3=ERROR 4=FATAL
  lattr.isasync = 1;           // 异步日志
  g_log.init_log(lattr);
  g_hand.bind_log(&g_log);

  // 流式 API
  lb_common::info_log(g_hand) << "服务启动, port=" << 9000 << lb_common::end_log;
}
```

详见 `include/mlog.h` 与 README §09（日志组无 使用范式 手册，README 直接给出源码地图）。

## 8. 完整模板项目骨架

```
my_app/
├── CMakeLists.txt
├── main.cpp
├── src/
│   ├── server.cpp            ← 业务模块
│   └── server.h
└── log/                       ← 日志目录（运行时创建）
```

最小 CMakeLists：

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../common ${CMAKE_BINARY_DIR}/lbcommon_build)

file(GLOB APP_SRC src/*.cpp main.cpp)
add_executable(my_app ${APP_SRC})
target_link_libraries(my_app PRIVATE lbcommon)
target_compile_options(my_app PRIVATE
  -march=native -m64 -D_LINUX -DLINUX -Wall -Wextra)
```

## 9. 故障排查

| 现象 | 原因 | 解决 |
|------|------|------|
| `fatal error: aio_tcp.h: No such file` | include 路径未传 | 检查 `target_link_libraries(... lbcommon)` 是否设置；不要用 `target_link_libraries(... liblbcommon.a)` |
| `undefined reference to pthread_create` | 链 lib 时丢 pthread | 不要绕过 `target_link_libraries(lbcommon)` 直接链 `.a` |
| `Illegal instruction` 运行时崩溃 | `-march` 与 lbcommon 不一致 | 业务项目加 `-march=native` 与 lbcommon 对齐 |
| `multiple definition of lb_common::xxx` | 业务项目把 lbcommon 头文件多次 include 进不同 TU | lbcommon 头文件用 `#pragma once`；如仍冲突，检查业务项目内是否手 wrap |
| 跨进程通信时数据错乱 | 不同进程用了不同版本的 lbcommon 头文件 | 所有进程用同一 git commit |

## 10. 下一步

- 第一段代码：[01-getting-started.md](./01-getting-started.md)
- 业务组装模板：[05-business-recipes.md](./05-business-recipes.md)
- 完整 API：`../common/include/*.h`（47 个 header-only / inline 头文件）
