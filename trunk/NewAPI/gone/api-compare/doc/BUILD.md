# api 编译方法说明

> 适用：api 模块（libg1api / libg1api_sf）双场景编译。构建逻辑唯一权威 = [api/CMakeLists.txt](../CMakeLists.txt)，本文件是其使用说明。

## 1. 产物

| 产物 | 内容 | 产出条件 |
|---|---|---|
| `api/lib/libg1api.so` | **socket-only** 变体（single/multi socket 引擎 + 98 柜台） | 恒产出 |
| `api/lib/libg1api_sf.so` | **solarflare tcpdirect** 变体（同源 + tcpdir 引擎 + zf 封装层） | 检测到 TCPDirect 开发环境时追加产出 |

两库同源码同接口：区别仅在于 `HAS_TCPDIRECT` 宏裁剪——socket-only 变体不含任何 tcpdir/zf 符号（`nm` 实测为零），配置误选 `speed_link_type=tcpdirect` 时在**配置校验层与 factory 层双防线**拒绝（`validate()` 直接调用返回 `LBAPI_ERR_UNSUPPORT_LINK`；标准 `create_instance` 流程统一聚合为 `LBAPI_ERR_CFG_INVALID`）。

## 2. 快速开始（零参数，自动检测）

```bash
# 独立编译（推荐）
cmake -S api -B build-api
cmake --build build-api -j

# 或全仓根构建（g1api / g1api_sf 目标随 api 子目录）
cmake -S . -B build && cmake --build build -j
```

configure 输出会明示检测结果：

```
-- TCPDirect: ENABLED  → 额外产出 libg1api_sf.so (zf_inc=/usr/include, libs=...)
-- TCPDirect: DISABLED → 仅产出 libg1api.so (socket-only, ...)
```

## 3. 场景矩阵

| 场景 | 命令 | 结果 |
|---|---|---|
| 无 solarflare（客户机/普通开发机） | `cmake -S api -B build-api` | 只产出 libg1api.so |
| 有 solarflare 开发环境 | 同上（自动检测） | 产出 libg1api.so + libg1api_sf.so |
| 有环境但只要 socket-only | `cmake -S api -B build-api -DWITH_TCPDIRECT=OFF` | 只产出 libg1api.so |
| 强制要求 sf 变体（发布机/CI 防呆） | `cmake -S api -B build-api -DWITH_TCPDIRECT=ON` | 环境缺失 → configure 立即 FATAL_ERROR |

## 4. 环境检测机制（官方口径）

- **编译期检测的是开发文件**，不是网卡：`/usr/include/zf/zf.h` 真头 +
  `libonload_zf_static.a + libciul1.a`（静态，优先）或 `libonload_zf.so`（动态，回退）。
  搜索路径覆盖两类发行版布局：`/usr/lib/x86_64-linux-gnu`（Debian/Ubuntu）、`/usr/lib64`（RHEL 系）等。
- **网卡与驱动是运行期概念**：编译机无需 Solarflare 网卡。运行期 `zf_init`/建栈失败
  由引擎 init 返回错误（如 `-ENOKEY` 未许可）。运行环境诊断：`lsmod | grep onload`、
  `/proc/driver/onload`、`lspci -d 1924:`。
- 仓内 `solarflare/zf/` 是仅供无环境编译的**桩头**：sf 变体用 `target_include_directories
  (BEFORE)` 把系统真头插到搜索序最前，桩头绝不进入 tcpdirect 变体；socket-only 变体
  因宏裁剪根本不包含 zf 头。
- 安装来源：onload/TCPDirect 官方包（[Xilinx-CNS/onload](https://github.com/Xilinx-CNS/onload)，
  AMD/Solarflare 文档站）。ARM(aarch64) 为版本依赖的渐进支持（onload v6.1 起 preview，
  见 [onload-arm](https://github.com/IntelligentCompute/onload-arm)），ARM 部署前须按目标
  版本核对官方 support matrix，并为 CMake 检测路径补 `/usr/lib/aarch64-linux-gnu`。

## 5. 依赖与常见问题

- **依赖**：`common/lib/liblbcommon.a` 须先构建（根构建自动带上；独立编译需确保存在）。
- 链接形态（用户裁决）：**静态优先**——`libonload_zf_static.a + libciul1.a`（Onload
  UG1586 背书的静态组合，部署侧免装 .so）；缺失时回退动态 `libonload_zf.so`。
- `counter98` 为 vendor 库（零改动裁决）：其 `string_util.cpp` 的 2 处 `-Wtype-limits`
  恒假告警在构建侧抑制（per-file `-Wno-type-limits`），vendor 文件不动。
- 下游消费方（mock_client / perf_client）按 `g1api` 名链接（`find_library(g1api ...)`）。
