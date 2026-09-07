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
    ├── gone/api/CMakeLists.txt     # liblbapi.so 动态库
    └── solarflare/CMakeLists.txt   # 可选，需 Solarflare SDK
```

## 三、各文件改动说明

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

### 4. gone/api/CMakeLists.txt（精简 + 条件编译）
- include 路径改为相对路径（`../../common/include` 等），适配根目录作为顶层入口。
- **TCPDirect 源文件条件编译**：未启用 Solarflare 时排除 `tcpdir_link.cpp`、`tcpdirect_engine.cpp`（它们依赖 SDK 头文件 `tcpdir_stack.h`）。

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

## 五、编译结果
- 编译命令：`mkdir -p build && cd build && cmake .. && make -j4`
- 注意：需 `env -u LD_LIBRARY_PATH` 运行（LD_LIBRARY_PATH 指向 VSCode 扩展旧 libstdc++，会导致 cmake/make 库冲突）。
- **构建产物**：
  - `build/lib/liblbcommon.a`（lb_common 基础库，静态库）
  - `build/lib/liblbapi.so`（交易 API 动态库）
- **状态**：✅ 编译通过（lbcommon + lbapi 均 Built target）。

## 六、使用方式
```bash
cd /home/lsz/code/work/api_trunk
mkdir -p build && cd build
cmake ..          # 如需 Release：cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
# 产物：build/lib/liblbapi.so、build/lib/liblbcommon.a
```