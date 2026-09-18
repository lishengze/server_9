# NewAPI 异地测试手册（api_test_manual）

> 版本：v1.0
> 日期：2026-09-18
> 适用范围：在**其他主机**上编译、部署、执行 NewAPI（gw_counter_direct / fpga_counter_direct）性能与功能测试
> 归属：trunk/NewAPI/gone/api/mock/client

---

## 目录

1. [概述](#1-概述)
2. [系统架构与测试组件](#2-系统架构与测试组件)
3. [环境准备](#3-环境准备)
4. [代码编译](#4-代码编译)
5. [配置修改](#5-配置修改)
6. [部署](#6-部署)
7. [测试执行](#7-测试执行)
8. [网卡抓包时间分析](#8-网卡抓包时间分析)
9. [测试结果说明](#9-测试结果说明)
10. [故障排查](#10-故障排查)
11. [附录](#11-附录)

---

## 1. 概述

本手册指导在**新主机**上完成 NewAPI 测试的全流程，包括：

- **编译**：API 核心库（liblbapi.so）、各 mock 测试组件、网卡抓包工具
- **部署**：模拟交易所、FTE（gw 柜台服务端）、98 柜台模拟、GOne 柜台模拟
- **测试**：功能测试（登录/委托/撤单/回报）、性能测试（perf_test）、网卡抓包时间分析

### 1.1 测试目标

| 测试项 | 说明 |
|------|------|
| 功能测试 | 验证 FTE 协议完整链路（登录/委托/撤单/回报/心跳） |
| 性能测试 | 测量委托 API 内处理耗时（api_arrive_time_ns → api_leave_time_ns） |
| 网卡抓包 | 测量委托从进入 API 到真正从网卡发出的端到端延迟 |

### 1.2 支持的柜台

| 柜台 | 类型 | 配置 fast_counter_type | 服务端 |
|------|------|:---:|------|
| gw（FTE） | gw_counter_direct | 1 | FTE 33001（上海）/ 33002（深圳） |
| GOne（FPGA） | fpga_counter_direct | 2 | gone_counter_mock 44001(GW)/44002(Core) |

---

## 2. 系统架构与测试组件

### 2.1 架构

```
┌─────────────────────────────────────────────────────┐
│                   测试主机（docker otc 容器）          │
│                                                     │
│  ┌──────────────┐   ┌───────────────────────────┐  │
│  │  mock_client  │   │  api_net_time_capture     │  │
│  │   (测试客户端)  │   │   (网卡抓包工具, root)      │  │
│  └──────┬───────┘   └────────────┬──────────────┘  │
│         │ 加载 liblbapi.so        │ AF_PACKET 抓包    │
│  ┌──────▼───────┐   ┌────────────▼──────────────┐  │
│  │   liblbapi.so│   │  网卡 (lo / 物理网卡)        │  │
│  └──────┬───────┘   └───────────────────────────┘  │
│         │ TCP 连接                                   │
│  ┌──────▼───────────────────────────────────────┐  │
│  │  柜台服务端                                     │  │
│  │  FTE(33001) / gone_counter_mock(44001/44002)  │  │
│  └──────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────┐  │
│  │  counter98_mock (9001/9003)  AGW 登录           │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 2.2 需要编译的组件

| 组件 | 产物 | 说明 |
|------|------|------|
| **liblbapi.so** | `build_cmake/lib/liblbapi.so` | API 核心库（被测对象），mock_client 加载 |
| **mock_client** | `build_cmake/bin/mock_client` | 测试客户端（功能测试 + 性能测试） |
| **counter98_mock** | `build_cmake/bin/counter98_mock` | 98 柜台模拟（AGW/账户登录） |
| **gone_counter_mock** | `build_cmake/bin/gone_counter_mock` | GOne 柜台模拟（GW 44001 + Core 44002） |
| **api_net_time_capture** | `build_cmake/bin/api_net_time_capture` | 网卡抓包分析工具（root 运行） |
| **perf_client** | `build_cmake/bin/perf_client` | 独立性能测试客户端（可选） |

### 2.3 FTE 环境（gw 柜台服务端，独立项目 gt_trunk）

| 组件 | 端口 | 说明 |
|------|------|------|
| tgw_simulator | 38140(Bond)/38141(Stock)/39142(ETF) | 模拟交易所 |
| 上海 FTE | **33001** | gw 柜台服务端（连接 38140/38141） |
| 深圳 FTE | **33002** | gw 柜台服务端（连接 39142） |

---

## 3. 环境准备

### 3.1 前置依赖

| 依赖 | 说明 |
|------|------|
| Docker | FTE 编译/运行需要 docker 容器 `otc` |
| gcc/g++ | 容器内 gcc 4.8.5（CMakeLists 已跳过高版本 flag） |
| root 权限 | 网卡抓包（AF_PACKET SOCK_RAW）需要 root |
| 磁盘空间 | 建议 ≥ 10GB（FTE + API 编译产物） |

### 3.2 目录结构

```
宿主机 /home/lsz/code/         （挂载到容器 /mnt）
├── api_trunk/                 （NewAPI 项目）
│   └── trunk/NewAPI/
│       ├── build.sh           （编译入口）
│       ├── build_cmake/       （编译产物）
│       └── gone/api/mock/
│           ├── client/        （mock_client + api_net_time_capture）
│           ├── 98_counter/    （counter98_mock）
│           └── gone_counter/  （gone_counter_mock）
├── gt_trunk/                  （FTE 项目，独立）
│   └── DYS-FRAMEWORK/
│       ├── fte/test_all/      （FTE 启动脚本）
│       └── api/gw_api_boost/  （旧 API，参考）
```

> 容器内路径：宿主机 `/home/lsz/code` 挂载到容器 `/mnt`，即宿主机 `/home/lsz/code/api_trunk` = 容器 `/mnt/work/api_trunk`。

### 3.3 检查 docker 容器

```bash
# 检查容器 otc 是否运行
docker ps --filter name=otc

# 若未运行则启动
docker start otc

# 验证容器内目录挂载
docker exec otc test -d /mnt/work/api_trunk && echo "api_trunk 挂载 OK"
docker exec otc test -d /mnt/work/gt_trunk && echo "gt_trunk 挂载 OK"
```

---

## 4. 代码编译

### 4.1 编译 API 框架（liblbapi.so + 所有 mock 组件）

在**宿主机**项目根目录执行（build.sh 自动进入 docker otc 编译）：

```bash
cd /home/lsz/code/api_trunk

# 编译 Release 版本，启用 mock 组件
./build.sh Release -DBUILD_MOCK=ON -j8
```

**编译产物**：

| 产物 | 路径 |
|------|------|
| liblbapi.so | `build_cmake/lib/liblbapi.so` |
| mock_client | `build_cmake/bin/mock_client` |
| counter98_mock | `build_cmake/bin/counter98_mock` |
| gone_counter_mock | `build_cmake/bin/gone_counter_mock` |
| api_net_time_capture | `build_cmake/bin/api_net_time_capture` |
| perf_client | `build_cmake/bin/perf_client` |

> build.sh 参数：
> - `Debug`/`Release`：编译类型（默认 Release）
> - `-jN`：并行数
> - `-D<option>=<value>`：CMake 选项透传（如 `-DBUILD_MOCK=ON`）
> - `clean`/`rebuild`：清理/重建

### 4.2 编译 FTE 环境（gt_trunk，仅 gw 测试需要）

```bash
cd /home/lsz/code/gt_trunk

# Release 编译（自动进入 docker otc）
./compile_fte.sh
# 或: ./compile_fte.sh -r
```

**产物**：安装到 `${CMAKE_ATP_RES_ROOT}/fte/bin/ute` 与 `fte/lib`。

### 4.3 验证编译产物

```bash
docker exec otc ls -la /mnt/work/api_trunk/build_cmake/bin/
docker exec otc ls -la /mnt/work/api_trunk/build_cmake/lib/liblbapi.so
docker exec otc ls -la /mnt/work/gt_trunk/work_atp/cmake/fte/bin/ute
```

---

## 5. 配置修改

> **异地测试关键**：所有 `127.0.0.1` 需改为**实际主机 IP**（如 FTE 与客户端同机可保留 127.0.0.1，跨机则改 IP）。

### 5.1 mock_client 配置（client/config/）

#### 5.1.1 `connection_config_gw_single.json`（gw 单客户）

| 字段 | 说明 | 异地修改 |
|------|------|------|
| `api_instance_name` | API 实例名（日志/链接命名） | 可改 |
| `market_type` | 1=上海, 2=深圳 | 一般不动 |
| `fast_counter_type` | 1=gw, 2=GOne | gw 保持 1 |
| `speed_link_type` | 1=single_socket | 一般不动 |
| `speed_counter_addr.ip/port` | **FTE 地址**（默认 127.0.0.1:33001） | **改 IP** |
| `counter98_addr.ip/port` | **98 柜台地址**（默认 127.0.0.1:9001） | **改 IP** |
| `98agw_user/password` | AGW 用户 | 与 counter98 server_config 一致 |
| `single_cust_per_link` | true=单客户（推荐）, false=多客户 | 按测试场景 |
| `perf_test.tps` | 目标 TPS | 按需 |
| `perf_test.duration_sec` | 测试时长（秒） | 按需 |
| `perf_test.report_file` | 报告输出 | 可改 |
| `perf_test.net_time_map_file` | 网卡抓包映射文件（可选） | 可改 |
| `perf_test.order.*` | 委托模板 | 与柜台账户一致 |

#### 5.1.2 `connection_config_gw_multi.json`（gw 多客户）

- 与 gw_single 相同，仅 `single_cust_per_link=false`。
- **注意**：多客户模式需在功能测试中完成 FTE 登录（见 §7.2）。

#### 5.1.3 `connection_config_gone.json`（GOne）

| 字段 | 说明 |
|------|------|
| `fast_counter_type` | **2**（GOne） |
| `speed_counter_addr.ip/port` | **GOne GW 地址**（默认 127.0.0.1:44001） |
| `counter98_addr.ip/port` | **98 柜台地址**（默认 127.0.0.1:9003） |

> GOne 委托实际走 Core 链路 44002（由 GW 登录应答回填），客户端配置只需 GW 地址 44001。

### 5.2 counter98_mock 配置（98_counter/config/server_config.json）

| 字段 | 说明 | 异地修改 |
|------|------|------|
| `server.listen_port` | 默认 9002 | 运行时用 `--port` 覆盖（gw 用 9001，GOne 用 9003） |
| `agw_users` | AGW 用户列表 | 与 mock_client `98agw_user` 一致 |
| `accounts` | 账户列表（fund_account_id/password/cust_id 等） | 与 mock_client `order.*` 一致 |

### 5.3 gone_counter_mock 配置（gone_counter/config/server_config.json）

| 字段 | 说明 |
|------|------|
| `server.gw_port` | GW 链路端口（默认 44001） |
| `server.core_port` | Core 链路端口（默认 44002） |
| `accounts` | 账户列表 |

---

## 6. 部署

### 6.1 启动 FTE 环境（gw 测试需要）

```bash
docker exec otc bash -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/start_all.sh'
```

脚本依次启动：
1. 模拟交易所（tgw_simulator：38140/38141/39142）
2. 上海 FTE（33001）
3. 深圳 FTE（33002）

**验证**：

```bash
docker exec otc bash -lc '(ss -tlnp 2>/dev/null || netstat -tlnp) | grep -E "33001|33002|38140|38141|39142"'
```

应看到 33001/33002/38140/38141/39142 全部监听。

> 若 38140(Bond) 进程 defunct，需单独重启（不影响委托测试，委托走 38141 Stock）：
> ```bash
> docker exec otc bash -lc 'cd /mnt/work/gt_trunk/DYS-FRAMEWORK/fte/test_all/tgw_simulator && LD_LIBRARY_PATH=./lib ATP_DATA=./data bin/tgw_simulator -p 38140 -m 7 -n TGWSimulator_Bond -l Info --log-to-console -d'
> ```

### 6.2 启动 counter98_mock（AGW 登录）

```bash
# gw 测试用 9001（或 GOne 用 9003），两个可同时启动
docker exec otc bash -lc 'cd /mnt/work/api_trunk && nohup ./build_cmake/bin/counter98_mock \
  --config trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json \
  --port 9001 > /tmp/counter98_mock_9001.log 2>&1 &'

docker exec otc bash -lc 'cd /mnt/work/api_trunk && nohup ./build_cmake/bin/counter98_mock \
  --config trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json \
  --port 9003 > /tmp/counter98_mock_9003.log 2>&1 &'
```

### 6.3 启动 gone_counter_mock（GOne 测试需要）

```bash
docker exec otc bash -lc 'cd /mnt/work/api_trunk && nohup ./build_cmake/bin/gone_counter_mock \
  --config trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json \
  > /tmp/gone_counter_mock.log 2>&1 &'
```

监听 44001(GW) + 44002(Core)。

### 6.4 端口验证汇总

```bash
docker exec otc bash -lc '(ss -tlnp 2>/dev/null || netstat -tlnp) | grep -E "33001|33002|9001|9003|44001|44002|38140|38141|39142"'
```

---

## 7. 测试执行

### 7.1 准备测试用例目录

功能测试通过 `--testdir` 指定用例目录，遍历执行目录下所有 JSON 用例。

> **关键**：FTE 登录（`api_->login()`）由功能测试用例（`Login` 类型）触发。**必须用包含登录用例的目录**，否则 FTE 未登录，perf_test 会全部失败（`trade link not connected`，-22）。

**推荐**：创建只含登录用例的最小目录，快速完成登录后进入 perf_test：

```bash
# gw 登录用例
docker exec otc bash -lc 'mkdir -p /tmp/fte_login_only && cp \
  /mnt/work/api_trunk/trunk/NewAPI/gone/api/mock/client/config/test_cases/fte_login.json \
  /tmp/fte_login_only/'

# GOne 登录用例（使用 config/test_cases/gone_login.json 模板）
docker exec otc bash -lc 'mkdir -p /tmp/gone_login_only && cp \
  /mnt/work/api_trunk/trunk/NewAPI/gone/api/mock/client/config/test_cases/gone_login.json \
  /tmp/gone_login_only/'
```

> 完整功能测试（含委托/撤单/回报校验）可用完整 `config/test_cases` 目录，但其中 `gone_combo.json`（GOne 委托等待回报）可能卡住，建议分离 gw/GOne 用例。

### 7.2 执行 mock_client（功能测试 + 性能测试）

```bash
# gw 单客户 8000 TPS
docker exec otc bash -lc 'cd /mnt/work/api_trunk && \
  export LD_LIBRARY_PATH=/mnt/work/api_trunk/build_cmake/lib:$LD_LIBRARY_PATH && \
  ./build_cmake/bin/mock_client \
    --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_single.json \
    --testdir /tmp/fte_login_only'

# gw 多客户 8000 TPS
docker exec otc bash -lc 'cd /mnt/work/api_trunk && \
  export LD_LIBRARY_PATH=/mnt/work/api_trunk/build_cmake/lib:$LD_LIBRARY_PATH && \
  ./build_cmake/bin/mock_client \
    --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_multi.json \
    --testdir /tmp/fte_login_only'

# GOne 8000 TPS
docker exec otc bash -lc 'cd /mnt/work/api_trunk && \
  export LD_LIBRARY_PATH=/mnt/work/api_trunk/build_cmake/lib:$LD_LIBRARY_PATH && \
  ./build_cmake/bin/mock_client \
    --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gone.json \
    --testdir /tmp/gone_login_only'
```

**mock_client 参数**：
| 参数 | 说明 |
|------|------|
| `--config <json>` | 连接配置（必填） |
| `--testdir <dir>` | 功能测试用例目录（必填） |
| `--lib <path>` | liblbapi.so 路径（默认从 LD_LIBRARY_PATH 加载） |

**执行流程**：功能测试（遍历 testdir 用例）→ 若配置 `perf_test.enable=true` → perf_test（预热 → 匀速发单 → 统计）。

**输出**：
- 功能测试报告：`test_report.txt`
- 性能测试报告：`perf_report_*.txt`（由配置 `report_file` 指定）
- 网卡抓包映射：`/tmp/api_net_time_map_*.txt`（由配置 `net_time_map_file` 指定）

### 7.3 性能测试报告解读

```
========== 性能测试报告（FTE）==========
实际 TPS      : 7797.97
样本数        : 77980
平均值        : ...
P50/P75/P90/P95/Max/Min/标准差
失败返回码统计 : （0 失败则无此段）
========================================
```

指标为 **API 内处理耗时**（`api_leave_time_ns - api_arrive_time_ns`，即入队 + 引擎消费）。

---

## 8. 网卡抓包时间分析

### 8.1 原理

委托包不含 `api_arrive_time_ns`，通过 **client_seq_id 关联**：
1. mock_client 记录 `(client_seq_id → api_arrive_time_ns)` 映射，写出映射文件（配置 `net_time_map_file`）。
2. `api_net_time_capture` 抓取发出的委托包，解析 `client_seq_id`，从映射文件查 `api_arrive_time_ns`。
3. 计算 `lat = 网卡发出时间戳 - api_arrive_time_ns`，统计指标。

### 8.2 协议差异（gw vs GOne）

| 项 | gw (FTE) | GOne (FPGA) |
|------|------|------|
| 包结构 | PktNewHeader 8B + TradeOrderReq 106B + 校验和 4B = 118B | g1_msg_head 16B + order_req 48B = 64B |
| msg_id | 1003（大端） | 1001（小端） |
| client_seq_id 偏移 | 62 | 40 |
| client_seq_id 字节序 | 小端 | 小端 |
| 委托端口 | 33001 | **44002（Core）** |
| `--proto` | `gw` | `gone` |

### 8.3 执行抓包（需 root）

**流程**：先启动抓包程序（覆盖测试时长）→ 运行 mock_client → 抓包程序结束后读取映射文件输出报告。

```bash
# gw（33001 端口，120 秒窗口）
docker exec otc bash -lc 'cd /mnt/work/api_trunk && \
  nohup ./build_cmake/bin/api_net_time_capture \
    --iface lo --port 33001 --map /tmp/api_net_time_map_single.txt \
    --proto gw --duration 120 --report /tmp/api_net_time_report_single.txt \
    > /tmp/capture_single.log 2>&1 &'

# GOne（44002 Core 端口，120 秒窗口）
docker exec otc bash -lc 'cd /mnt/work/api_trunk && \
  nohup ./build_cmake/bin/api_net_time_capture \
    --iface lo --port 44002 --map /tmp/api_net_time_map_gone.txt \
    --proto gone --duration 120 --report /tmp/api_net_time_report_gone.txt \
    > /tmp/capture_gone.log 2>&1 &'
```

然后按 §7.2 运行 mock_client（在抓包窗口内），等待抓包程序自动结束（`--duration` 超时）。

**参数**：
| 参数 | 说明 |
|------|------|
| `--iface` | 抓包接口（回环测试用 `lo`，跨机用实际网卡如 `eth0`） |
| `--port` | 目标端口（gw=33001，GOne=44002） |
| `--map` | mock_client 产出的映射文件 |
| `--proto` | `gw` 或 `gone`（协议解析） |
| `--duration` | 抓包时长（秒），超时自动退出 |
| `--report` | 报告输出文件 |

### 8.4 抓包报告解读

```
========== 网卡抓包时间分析报告 ==========
协议类型   : gw (FTE)
捕获委托包 : 77780
关联成功   : 77780 (100%)
关联失败   : 0
---------------- 延迟指标（网卡发出时间 - api_arrive_time_ns, 纳秒）----------------
P50/P75/P90/P95/Max/Min/平均值/标准差
==========================================
```

**指标含义**：委托从进入 API 到真正从网卡发出的端到端延迟。与 API 内处理耗时（§7.3）对比，差额 ≈ 内核协议栈 + 网卡排队时间。

---

## 9. 测试结果说明

| 指标 | 含义 | 对比对象 |
|------|------|------|
| API 内处理耗时 | 入队 + 引擎消费（send 前） | gw vs GOne |
| 网卡端到端延迟 | API 进入 → 网卡发出 | gw vs GOne |
| 内核+网卡排队差额 | 端到端 - API 内 | 公共基线，与柜台无关 |

**参考结论（2026-09-18，8000 TPS）**：
- API 内：gw P50=290ns 优于 GOne 421ns；但 gw P95=2836ns 高于 GOne 732ns（FTE 背压）
- 端到端：gw P50=2856ns 略优于 GOne 3197ns；gw P95=6626ns 高于 GOne 4699ns
- 内核+网卡排队：两者均 ~2.5-4μs（公共基线）

---

## 10. 故障排查

| 现象 | 可能原因 | 处理 |
|------|------|------|
| `api->start() 失败: -18` | counter98_mock 未启动或端口不对 | 检查 9001/9003 监听；确认配置 `counter98_addr.port` |
| perf_test 全部 `-22`（trade link not connected） | **FTE 未登录**（用空/错误 testdir 跳过登录） | 用含 `fte_login.json` 的目录；确认 FTE 33001 已启动 |
| `connect_ch ret=-34` | 目标端口未监听 | 检查 FTE/gone_counter_mock 是否启动 |
| 抓包捕获委托包 0 | 端口/协议参数错误 | GOne 用 44002 + `--proto gone`；gw 用 33001 + `--proto gw` |
| 抓包程序不退出 | `recvmsg` 阻塞无法检查 duration | 已内置 SO_RCVTIMEO 1s，正常应超时退出；检查是否旧版本 |
| 关联失败数高 | 抓包窗口未覆盖 mock_client 发单期 | 先启动抓包再运行 mock_client；增大 `--duration` |
| 功能测试卡在 GOne 委托 | `gone_combo.json` 等待回报卡住 | 用只含登录用例的目录 |
| 38140 进程 defunct | tgw_simulator 启动失败 | 单独重启 38140（§6.1），不影响委托测试 |
| 抓包需 root | AF_PACKET SOCK_RAW 权限 | 用 root 运行；容器内默认 root |

---

## 11. 附录

### 11.1 端口清单

| 端口 | 用途 |
|------|------|
| 33001 | 上海 FTE（gw 委托链路） |
| 33002 | 深圳 FTE |
| 9001 | counter98_mock（gw 测试） |
| 9003 | counter98_mock（GOne 测试） |
| 44001 | gone_counter_mock GW 链路（登录/心跳） |
| 44002 | gone_counter_mock Core 链路（委托） |
| 38140/38141/39142 | tgw_simulator 模拟交易所 |

### 11.2 常用命令速查

```bash
# 编译 API
cd /home/lsz/code/api_trunk && ./build.sh Release -DBUILD_MOCK=ON -j8

# 编译 FTE
cd /home/lsz/code/gt_trunk && ./compile_fte.sh

# 启动 FTE 环境
docker exec otc bash -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/start_all.sh'

# 启动 counter98_mock（9001）
docker exec otc bash -lc 'cd /mnt/work/api_trunk && nohup ./build_cmake/bin/counter98_mock --config trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json --port 9001 > /tmp/c98_9001.log 2>&1 &'

# 启动 gone_counter_mock
docker exec otc bash -lc 'cd /mnt/work/api_trunk && nohup ./build_cmake/bin/gone_counter_mock --config trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json > /tmp/gone.log 2>&1 &'

# 运行 gw 单客户测试
docker exec otc bash -lc 'cd /mnt/work/api_trunk && LD_LIBRARY_PATH=./build_cmake/lib ./build_cmake/bin/mock_client --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_single.json --testdir /tmp/fte_login_only'

# 网卡抓包（gw）
docker exec otc bash -lc 'cd /mnt/work/api_trunk && ./build_cmake/bin/api_net_time_capture --iface lo --port 33001 --map /tmp/api_net_time_map_single.txt --proto gw --duration 120 --report /tmp/report.txt'

# 停止 FTE 环境
docker exec otc bash -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/stop_all.sh'
```

### 11.3 关键文件索引

| 文件 | 说明 |
|------|------|
| `trunk/NewAPI/gone/api/mock/client/api_net_time_design.md` | 网卡抓包方案设计 |
| `trunk/NewAPI/gone/api/mock/client/api_net_time_common.h` | 协议常量 + 映射读写 |
| `trunk/NewAPI/gone/api/mock/client/api_net_time_capture.cpp` | 抓包程序（--proto gw/gone） |
| `trunk/NewAPI/gone/api/mock/client/src/perf_runner.h/.cpp` | 性能测试 + 映射记录 |
| `trunk/NewAPI/gone/api/mock/client/config/connection_config_*.json` | 客户端连接配置 |
| `trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json` | counter98_mock 配置 |
| `trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json` | gone_counter_mock 配置 |
| `task/api_dev/time_ana.md` | 性能分析文档（含 §9 GOne vs gw 对比） |