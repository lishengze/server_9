# CMakeLists.txt 优化记录

## 一、任务目标
以 `/home/lsz/code/work/grc_trunk/CMakeLists.txt` 为模板，优化整个系统的 CMakeLists.txt。
与 grc_trunk 不同，当前代码依赖的外部库很少，因此大幅精简了外部库相关配置。

## 二、优化后的文件结构

```
api_trunk/
├── CMakeLists.txt                  # 顶层入口（统一全局配置 + add_subdirectory）
├── CMakeLists.txt.bak              # 原根 CMakeLists.txt 备份
└── trunk/NewAPI/
    ├── CMakeLists.txt              # 编排层（common + solarflare(可选) + gone/api）
    ├── common/CMakeLists.txt       # liblbcommon.a 静态库
    ├── solarflare/CMakeLists.txt   # libzyslfch.a（可选，需 Solarflare SDK）
    └── gone/api/CMakeLists.txt     # liblbapi.so 动态库
```

## 三、独立编译机制（与 grc_trunk 一致）

**核心模式**：用 `if (NOT DEFINED BUILD_SUM_COUNT)` 判断编译方式。

- **通过父模块编译**：父模块（根 CMakeLists 或 trunk/NewAPI）已定义 `BUILD_SUM_COUNT`，子模块跳过全局配置，沿用父模块的编译选项/输出目录。
- **独立编译**：`BUILD_SUM_COUNT` 未定义，子模块自行设置完整全局配置（构建类型、C++ 标准、编译优化 FLAGS、输出目录）。

各级模块（common / gone/api / solarflare / trunk/NewAPI）均采用此模式，因此**既能独立编译，也能通过父模块编译**。

- 根 CMakeLists 通过 `set(BUILD_SUM_COUNT 0)` 标记"通过父模块编译"。
- gone/api 独立编译时，会自动 `add_subdirectory` 构建依赖的 common 库（须显式指定 binary 目录 `common_build`，因为 common 不在其子目录下）。
- 各模块在统计文件数时通过 `set(BUILD_SUM_COUNT ... PARENT_SCOPE)` 向父层累加；独立编译时用 `set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)` 抑制"无父作用域"警告。

## 四、各文件改动说明

### 1. 根 CMakeLists.txt（重写）
- 统一设置：默认 Debug 构建类型、全局编译优化 FLAGS（`-march=native` 等低延迟参数）、Debug/Release 配置、PIC、输出目录（`build/lib`、`build/bin`）、RPATH、`CMAKE_EXPORT_COMPILE_COMMANDS`。
- 新增 `NEWAPI_ROOT` 指向 `trunk/NewAPI`，通过 `add_subdirectory` 引入整个 NewAPI 工程。
- **大幅精简外部库依赖**：删除 grc_trunk 中的 boost、odbc、grpc、AMI、ssh 等外部库检测与链接。

### 2. trunk/NewAPI/CMakeLists.txt（重写为编排层）
- 移除冗余的全局编译选项（根已统一处理）。
- **Solarflare 默认关闭**（`ENABLE_SOLARFLARE OFF`），仅当检测到 SDK 时才编译 solarflare 子模块。
- **默认跳过 fgw**（FGW 网关程序依赖外部 odbc/third_odbc，本工程不依赖外部库）。

### 3. common/CMakeLists.txt（精简）
- 移除 `set(CMAKE_SYSTEM_NAME Linux)`（避免构建时强制系统类型）。
- 输出目录改为 `build/lib`（原为源代码目录下的 `lib`，会污染源码树）。
- 移除冗余全局标志，仅保留 lbcommon 静态库目标定义。

### 4. gone/api/CMakeLists.txt（精简 + 条件编译 + 独立编译 Solarflare 检测）
- include 路径改为相对路径（`../../common/include` 等），适配根目录作为顶层入口。
- **TCPDirect 源文件条件编译**：未启用 Solarflare 时排除 `tcpdir_link.cpp`、`tcpdirect_engine.cpp`（它们依赖 SDK 头文件 `tcpdir_stack.h`）。
- **独立编译时 Solarflare 检测**：`HAS_SOLARFLARE` 未定义时（独立编译），自行执行完整的 Solarflare SDK 检测（`ENABLE_SOLARFLARE` 选项 + SDK 路径探测），而非简单地默认禁用。通过父模块编译时，`HAS_SOLARFLARE` 已由父模块定义，直接沿用，不重复检测。

### 5. solarflare/CMakeLists.txt（独立编译支持）
- 加 `BUILD_SUM_COUNT` 守卫，独立编译时自行设置全局配置。
- 无 Solarflare SDK 时通过 `return()` 跳过，不阻塞编译。

## 四、编译过程中遇到的问题与解决

### 问题 1：`stdatomic.h` 中 `_Atomic` 不是类型
- **现象**：`mthread.cpp`、`api_instance.cpp` 编译报错 `'_Atomic' does not name a type`。
- **原因**：`mthread.h` 和 `api_instance.cpp` 直接 `#include <stdatomic.h>`（C11 头文件），其中使用 `_Atomic` 关键字在 C++ 模式下不是有效类型。代码实际使用的是 `matomic.h` 中的自定义原子函数（`__atomic_*` 内置函数），并不需要 stdatomic.h。
- **解决**：移除 `mthread.h` 和 `api_instance.cpp` 中不必要的 `#include <stdatomic.h>`。
- **结果**：编译通过。

### 问题 2：`tcpdir_stack.h` 头文件不存在
- **现象**：`api_instance.cpp` 编译报 `fatal error: tcpdir_stack.h: 没有那个文件或目录`。
- **原因**：`api_instance.h` 无条件包含 `tcpdir_link.h` 和 `tcpdirect_engine.h`，而它们依赖 Solarflare SDK 头文件 `tcpdir_stack.h`。本机未安装 Solarflare SDK。
- **解决**（三处联动）：
  1. `gone/api/CMakeLists.txt`：未启用 Solarflare 时用 `list(REMOVE_ITEM ...)` 排除 `tcpdir_link.cpp`、`tcpdirect_engine.cpp`。
  2. `api_instance.h` / `api_instance.cpp`：用 `#ifdef HAS_TCPDIRECT` 保护 TCPDirect 相关 include。
  3. `api_instance.cpp`：工厂函数中两个 tcpdirect 分支、以及两处 `tcpdirect_engine` 模板实例化，均用 `#ifdef HAS_TCPDIRECT` 保护。
- **结果**：未启用 Solarflare 时，tcpdirect 配置返回 `LBAPI_ERR_UNSUPPORT_LINK`，其余配置正常编译。

### 问题 3：C++11 标准兼容性（`std::is_integral_v`）
- **现象**：用 C++11 标准编译时，`mutils.h:220` 报错 `'is_integral_v' is not a member of 'std'`。
- **原因**：`mutils.h` 的 `is_power_2` 模板函数中使用了 `std::is_integral_v<T>`，这是 C++17 的变量模板特性（`_v` 后缀），C++11/14 不支持。
- **解决**：改为 C++11 兼容的 `std::is_integral<T>::value`。
- **结果**：修复后 C++11 编译通过（lbcommon + lbapi 均构建成功），C++17 编译同样通过（该写法两个标准均兼容）。
- **结论**：当前代码整体基于 C++11 特性编写，仅此一处 C++17 特性，修复后即可用 C++11 正常编译。

## 五、编译结果
- 编译命令：`mkdir -p build && cd build && cmake .. && make -j4`
- 注意：需 `env -u LD_LIBRARY_PATH` 运行（LD_LIBRARY_PATH 指向 VSCode 扩展旧 libstdc++，会导致 cmake/make 库冲突）。
- **编译标准**：**C++11**（`CMAKE_CXX_STANDARD 11`，FLAGS 中 `--std=c++11`）。
- **构建产物**：
  - `build/lib/liblbcommon.a`（lb_common 基础库，静态库）
  - `build/lib/liblbapi.so`（交易 API 动态库）
- **状态**：✅ 编译通过（lbcommon + lbapi 均 Built target）。

### C++11 标准切换（第三轮调整）
应需求将整个项目编译标准从 C++17 统一降为 **C++11**：
- 修改所有 CMakeLists.txt（根、trunk/NewAPI、common、gone/api、solarflare）的 `CMAKE_CXX_STANDARD 17 → 11` 及 `--std=c++17 → --std=c++11`。
- 配套修复唯一一处 C++17 特性：`mutils.h` 的 `std::is_integral_v<T>` → `std::is_integral<T>::value`。
- 验证结果（均以 C++11 编译通过）：

| 编译方式 | 结果 |
|---------|------|
| 根目录整体编译 | ✅ lbcommon + lbapi |
| trunk/NewAPI 独立编译 | ✅ lbcommon + lbapi |
| common 独立编译 | ✅ lbcommon |
| gone/api 独立编译 | ✅ 自动构建 common + lbapi |

### 独立编译验证（第二轮优化）
本轮将各子模块改造成可独立编译（参考 grc_trunk 的 `BUILD_SUM_COUNT` 机制），验证结果：

| 编译方式 | 命令 | 结果 |
|---------|------|------|
| 根目录整体编译 | `cd api_trunk && cmake -B build && make` | ✅ lbcommon + lbapi |
| trunk/NewAPI 独立编译 | `cd trunk/NewAPI && cmake -B build && make` | ✅ lbcommon + lbapi |
| common 独立编译 | `cd common && cmake -B build && make` | ✅ lbcommon（无警告） |
| gone/api 独立编译 | `cd gone/api && cmake -B build && make` | ✅ 自动构建 common + lbapi |
| solarflare 独立编译 | `cd solarflare && cmake -B build` | ✅ 无 SDK 时正确跳过 |

## 六、使用方式

### 方式一：一键编译脚本 build.sh（推荐）
项目根目录提供 `build.sh` 脚本，在 `build_cmake` 文件夹下完成所有编译：
```bash
cd /home/lsz/code/work/api_trunk
./build.sh              # Debug 模式编译（产物在 build_cmake/lib/）
./build.sh Release      # Release 模式编译
./build.sh clean        # 清理 build_cmake 目录
./build.sh rebuild      # 清理后重新编译（Debug）
./build.sh -j8          # 指定 8 线程并行编译
./build.sh Release -j8  # Release + 8 线程
```
脚本自动处理 LD_LIBRARY_PATH 冲突，产物位于 `build_cmake/lib/`。

### 方式二：手动编译（根目录）
```bash
cd /home/lsz/code/work/api_trunk
mkdir -p build && cd build
cmake ..          # 如需 Release：cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
# 产物：build/lib/liblbapi.so、build/lib/liblbcommon.a
```

### 方式三：子模块独立编译（以 gone/api 为例，自动构建依赖的 common）
```bash
cd /home/lsz/code/work/api_trunk/trunk/NewAPI/gone/api
mkdir -p build && cd build
cmake ..
make -j4
# 产物：lib/liblbapi.so、lib/liblbcommon.a
```