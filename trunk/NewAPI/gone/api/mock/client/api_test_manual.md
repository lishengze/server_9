# NewAPI 异地测试手册（api_test_manual）

> 版本：v2.0
> 日期：2026-09-19
> 适用环境：**远程服务器（x86 / RHEL 7.2 / gcc 4.8.5）** + **开发机（部署 FTE 等服务端组件）**
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

### 1.1 场景说明

本手册面向 **双机异地测试** 场景：

| 角色 | 机器 | 系统 | 运行组件 |
|------|------|------|----------|
| **开发机** | 现有开发环境（含 docker otc） | CentOS 7 / 已有环境 | FTE（gt_trunk）、counter98_mock、gone_counter_mock |
| **远程测试机** | 异地服务器 | **RHEL 7.2 / x86 / gcc 4.8.5** | mock_client、api_net_time_capture |

远程测试机上**编译 API 客户端工具**，连接开发机上部署的柜台服务端，执行功能测试和性能测试。

### 1.2 测试目标

| 测试项 | 说明 |
|------|------|
| 功能测试 | 验证 FTE 协议完整链路（登录/委托/撤单/回报/心跳） |
| 性能测试 | 测量委托 API 内处理耗时（api_arrive_time_ns → api_leave_time_ns） |
| 网卡抓包 | 测量委托从进入 API 到真正从网卡发出的端到端延迟 |

### 1.3 支持的柜台

| 柜台 | 类型 | 配置 fast_counter_type | 服务端 |
|------|------|:---:|------|
| gw（FTE） | gw_counter_direct | 1 | FTE 33001（上海）/ 33002（深圳） |
| GOne（FPGA） | fpga_counter_direct | 2 | gone_counter_mock 44001(GW)/44002(Core) |

---

## 2. 系统架构与测试组件

### 2.1 双机架构

```
                 开发机（服务端）
    ┌──────────────────────────────────────────────┐
    │  FTE（gt_trunk）                              │
    │  上海 33001 │ 深圳 33002                      │
    │  counter98_mock（9001/9003）                   │
    │  gone_counter_mock（44001/44002）              │
    └──────────┬───────────────────────────────────┘
               │ TCP 连接（跨网络）
               ▼
    ┌──────────────────────────────────────────────┐
    │  远程测试机（RHEL 7.2 / x86 / gcc 4.8.5）     │
    │                                               │
    │  ┌──────────────┐  ┌───────────────────────┐ │
    │  │  mock_client  │  │  api_net_time_capture │ │
    │  │  (测试客户端)  │  │  (网卡抓包, root)     │ │
    │  └──────┬───────┘  └───────────┬───────────┘ │
    │         │ 加载 liblbapi.so     │ AF_PACKET    │
    │  ┌──────▼───────┐  ┌──────────▼───────────┐ │
    │  │  liblbapi.so │  │  物理网卡（如 eth0）   │ │
    │  └──────────────┘  └──────────────────────┘ │
    └──────────────────────────────────────────────┘
```

### 2.2 组件与运行位置

| 组件 | 产物 | 运行位置 | 说明 |
|------|------|:--------:|------|
| **liblbapi.so** | `build_cmake/lib/liblbapi.so` | 远程测试机 | API 核心库（被测对象），mock_client 加载 |
| **mock_client** | `build_cmake/bin/mock_client` | 远程测试机 | 测试客户端（功能测试 + 性能测试） |
| **counter98_mock** | `build_cmake/bin/counter98_mock` | 开发机 | 98 柜台模拟（AGW/账户登录） |
| **gone_counter_mock** | `build_cmake/bin/gone_counter_mock` | 开发机 | GOne 柜台模拟（GW 44001 + Core 44002） |
| **api_net_time_capture** | `build_cmake/bin/api_net_time_capture` | 远程测试机 | 网卡抓包分析工具（root 运行） |
| **perf_client** | `build_cmake/bin/perf_client` | 远程测试机 | 独立性能测试客户端（可选） |

### 2.3 FTE 环境（开发机，独立项目 gt_trunk）

| 组件 | 端口 | 说明 |
|------|------|------|
| tgw_simulator | 38140(Bond)/38141(Stock)/39142(ETF) | 模拟交易所 |
| 上海 FTE | **33001** | gw 柜台服务端（连接 38140/38141） |
| 深圳 FTE | **33002** | gw 柜台服务端（连接 39142） |

---

## 3. 环境准备

### 3.1 远程测试机前置依赖（RHEL 7.2）

| 依赖 | 说明 | 安装方式 |
|------|------|----------|
| gcc/g++ 4.8.5 | RHEL 7.2 自带，需确认 `gcc-c++` 包已安装 | `yum install gcc gcc-c++ make` |
| cmake ≥ **3.10** | RHEL 7.2 默认 cmake 2.8.12 **不满足**（根 CMakeLists.txt 要求 3.10） | 见下方 |
| root 权限 | 网卡抓包（AF_PACKET SOCK_RAW）需要 | `sudo` 或 root 用户 |
| git | 拉取代码 | `yum install git` |
| 磁盘空间 | ≥ 2GB（仅 API + mock 编译产物） | — |

#### ⚠️ cmake 版本问题（RHEL 7.2 关键）

根 CMakeLists.txt 要求 `cmake_minimum_required(VERSION 3.10)`，而 RHEL 7.2 默认 cmake 为 2.8.12。

**方案一（推荐）：安装 cmake3（EPEL）**

```bash
# 启用 EPEL 仓库
yum install epel-release

# 安装 cmake3
yum install cmake3

# 使用 cmake3 命令（或创建别名）
alias cmake=cmake3
```

**方案二：编译安装 cmake 3.x**

```bash
# 下载 cmake 3.10+ 源码
wget https://github.com/Kitware/CMake/releases/download/v3.10.3/cmake-3.10.3.tar.gz
tar xzf cmake-3.10.3.tar.gz
cd cmake-3.10.3

# 编译安装（需要 gcc）
./bootstrap --prefix=/usr/local
make -j$(nproc)
sudo make install

# 验证
/usr/local/bin/cmake --version
```

**方案三：使用预编译二进制**

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.10.3/cmake-3.10.3-Linux-x86_64.tar.gz
tar xzf cmake-3.10.3-Linux-x86_64.tar.gz -C /usr/local --strip-components=1
```

> 安装后验证：`cmake --version` 应显示 ≥ 3.10。

### 3.2 gcc 4.8.5 兼容性说明

本项目 CMakeLists.txt 已内置对 gcc 4.8.5 的兼容处理：

| 特性 | 状态 |
|------|------|
| `-mprefer-vector-width=256` | 自动跳过（gcc < 8 不支持） |
| `-std=c++11` | 支持（gcc 4.8.5 默认 C++11） |
| `-march=native` | 支持（编译当前 CPU 指令集） |
| `-mpopcnt` | 支持 |

无需任何特殊修改，直接编译即可。

### 3.3 代码获取

```bash
# 远程测试机上拉取代码（或从开发机 scp）
git clone <api_trunk_repo_url> /home/user/api_trunk
# 或从开发机拷贝
scp -r user@dev-machine:/home/lsz/code/api_trunk /home/user/api_trunk
```

### 3.4 目录结构

```
远程测试机 /home/user/api_trunk/
├── build.sh                 （⚠️ 依赖 docker，远程机上不可用）
├── CMakeLists.txt           （根 CMake，要求 cmake ≥ 3.10）
├── trunk/NewAPI/
│   ├── common/              （lbcommon 基础库）
│   ├── gone/api/
│   │   ├── include/         （API 头文件）
│   │   ├── src/             （API 源码）
│   │   └── mock/
│   │       ├── client/      （mock_client + api_net_time_capture）
│   │       ├── 98_counter/  （counter98_mock）
│   │       └── gone_counter/（gone_counter_mock）
│   └── CMakeLists.txt
└── build_cmake/             （编译产物目录）
```

---

## 4. 代码编译

### 4.1 ⚠️ build.sh 的 docker 依赖

`build.sh` 脚本会检测当前路径：若不在 `/mnt/*` 下，则自动执行 `docker exec otc ...` 转发到容器内编译。

**远程测试机上无 docker，build.sh 不可用**。必须使用**直接 cmake 命令**编译。

### 4.2 远程测试机编译 API 框架（无需 docker）

```bash
cd /home/user/api_trunk

# 创建构建目录
mkdir -p build_cmake && cd build_cmake

# 执行 cmake（Release 模式，启用 mock 组件）
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_MOCK=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 编译（使用所有 CPU 核心）
make -j$(nproc)
```

> 若安装的是 cmake3，用 `cmake3` 替代 `cmake`。

**编译产物**：

| 产物 | 路径 |
|------|------|
| liblbapi.so | `build_cmake/lib/liblbapi.so` |
| mock_client | `build_cmake/bin/mock_client` |
| counter98_mock | `build_cmake/bin/counter98_mock` |
| gone_counter_mock | `build_cmake/bin/gone_counter_mock` |
| api_net_time_capture | `build_cmake/bin/api_net_time_capture` |
| perf_client | `build_cmake/bin/perf_client` |

> **注意**：远程机上编译出的 counter98_mock、gone_counter_mock 也可以运行在远程机上（见 §6 可选），但通常建议在开发机上运行服务端。

### 4.3 编译选项说明

| 选项 | 说明 |
|------|------|
| `-DCMAKE_BUILD_TYPE=Release` | Release 模式（优化级别 -O3） |
| `-DBUILD_MOCK=ON` | 启用 mock 组件（mock_client、counter98_mock、gone_counter_mock、api_net_time_capture） |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | 导出 compile_commands.json（可选） |
| `-j$(nproc)` | 使用所有 CPU 核心并行编译 |

### 4.4 清理与重新编译

```bash
cd /home/user/api_trunk
rm -rf build_cmake
mkdir build_cmake && cd build_cmake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_MOCK=ON
make -j$(nproc)
```

### 4.5 开发机编译 FTE 环境（仅 gw 测试需要）

FTE 环境（gt_trunk）仅在**开发机**上编译部署，远程测试机不需要：

```bash
# 开发机上执行（在 docker otc 容器内）
cd /home/lsz/code/gt_trunk
./compile_fte.sh
```

**产物**：安装到 `${CMAKE_ATP_RES_ROOT}/fte/bin/ute` 与 `fte/lib`。

---

## 5. 配置修改

> **异地测试关键**：远程测试机连接开发机，所有配置中的 IP 地址必须从 `127.0.0.1` 改为**开发机的实际 IP**（如 `192.168.1.100`）。

### 5.1 mock_client 配置（client/config/）

配置文件均在 `trunk/NewAPI/gone/api/mock/client/config/` 目录下。

#### 5.1.1 `connection_config_gw_single.json`（gw 单客户）

| 字段 | 说明 | 异地修改 |
|------|------|----------|
| `api_instance_name` | API 实例名（日志/链接命名） | 可改 |
| `market_type` | 1=上海, 2=深圳 | 一般不动 |
| `fast_counter_type` | **1**=gw | gw 保持 1 |
| `speed_link_type` | 1=single_socket | 一般不动 |
| `speed_counter_addr.ip` | **FTE 地址**（默认 127.0.0.1） | **改为开发机 IP** |
| `speed_counter_addr.port` | **FTE 端口**（默认 33001） | 一般不动 |
| `counter98_addr.ip` | **98 柜台地址**（默认 127.0.0.1） | **改为开发机 IP** |
| `counter98_addr.port` | **98 柜台端口**（默认 9001） | 一般不动 |
| `98agw_user` | AGW 用户 | 与 counter98 server_config 一致 |
| `98agw_password` | AGW 密码 | 与 counter98 server_config 一致 |
| `single_cust_per_link` | true=单客户（推荐）, false=多客户 | 按测试场景 |
| `perf_test.enable` | true=启用性能测试 | 保持 true |
| `perf_test.tps` | 目标 TPS（如 8000） | 按需 |
| `perf_test.duration_sec` | 测试时长（秒） | 按需 |
| `perf_test.report_file` | 报告输出路径 | 可改 |
| `perf_test.net_time_map_file` | 网卡抓包映射文件（可选） | 可改 |
| `perf_test.order.*` | 委托模板（fund_account_id/side 等） | 与柜台账户一致 |

#### 5.1.2 `connection_config_gw_multi.json`（gw 多客户）

与 gw_single 相同，仅 `single_cust_per_link=false`。

#### 5.1.3 `connection_config_gone.json`（GOne）

| 字段 | 说明 | 异地修改 |
|------|------|----------|
| `fast_counter_type` | **2**（GOne） | 保持 2 |
| `speed_counter_addr.ip` | **GOne GW 地址**（默认 127.0.0.1） | **改为开发机 IP** |
| `speed_counter_addr.port` | GOne GW 端口（默认 44001） | 一般不动 |
| `counter98_addr.ip` | **98 柜台地址**（默认 127.0.0.1） | **改为开发机 IP** |
| `counter98_addr.port` | **98 柜台端口**（默认 9003） | 一般不动 |

> GOne 委托实际走 Core 链路 44002（由 GW 登录应答回填），客户端配置只需 GW 地址 44001。

#### 配置修改示例（gw_single，开发机 IP = 192.168.1.100）

```json
{
  "api_instance_name": "gw_single_remote",
  "market_type": 1,
  "fast_counter_type": 1,
  "speed_link_type": 1,
  "speed_counter_addr": {
    "ip": "192.168.1.100",
    "port": 33001
  },
  "counter98_addr": {
    "ip": "192.168.1.100",
    "port": 9001
  },
  "98agw_user": "agw_user_01",
  "98agw_password": "agw_pwd_01",
  "single_cust_per_link": true,
  "perf_test": {
    "enable": true,
    "tps": 8000,
    "duration_sec": 10,
    "warmup_sec": 2,
    "cpu_id": -1,
    "report_file": "/tmp/perf_report_gw_single.txt",
    "net_time_map_file": "/tmp/api_net_time_map_single.txt",
    "order": {
      "fund_account_id": "1000000000000001",
      "branch_id": "0001",
      "side": "1",
      "order_type": "2",
      "security_id": "600007",
      "order_price": 250200,
      "order_qty": 100,
      "stop_price": 0,
      "market_type": 1
    }
  }
}
```

### 5.2 counter98_mock 配置（开发机）

配置文件：`trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json`

| 字段 | 说明 |
|------|------|
| `server.listen_port` | 默认 9002，运行时用 `--port` 覆盖（gw 用 9001，GOne 用 9003） |
| `agw_users` | AGW 用户列表（与 mock_client `98agw_user` 一致） |
| `accounts` | 账户列表（fund_account_id/password/cust_id 等，与 mock_client order 一致） |

### 5.3 gone_counter_mock 配置（开发机）

配置文件：`trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json`

| 字段 | 说明 |
|------|------|
| `server.gw_port` | GW 链路端口（默认 44001） |
| `server.core_port` | Core 链路端口（默认 44002） |
| `accounts` | 账户列表 |

---

## 6. 部署

### 6.1 部署概览

| 组件 | 部署位置 | 启动方式 | 端口 |
|------|:--------:|----------|------|
| FTE | 开发机 | `start_all.sh`（docker otc 内） | 33001/33002 |
| counter98_mock | 开发机 | 直接运行 | 9001/9003 |
| gone_counter_mock | 开发机 | 直接运行 | 44001/44002 |
| mock_client | 远程测试机 | 直接运行 | — |
| api_net_time_capture | 远程测试机 | root 运行 | — |

### 6.2 开发机：启动 FTE 环境（gw 测试需要）

```bash
# 开发机上，进入 docker otc 容器执行
docker exec otc bash -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/start_all.sh'
```

**验证**：

```bash
docker exec otc bash -lc '(ss -tlnp 2>/dev/null || netstat -tlnp) | grep -E "33001|33002|38140|38141|39142"'
```

应看到 33001/33002/38140/38141/39142 全部监听。

> 若 38140(Bond) 进程 defunct，需单独重启（不影响委托测试，委托走 38141 Stock）：
> ```bash
> docker exec otc bash -lc 'cd /mnt/work/gt_trunk/DYS-FRAMEWORK/fte/test_all/tgw_simulator && LD_LIBRARY_PATH=./lib ATP_DATA=./data bin/tgw_simulator -p 38140 -m 7 -n TGWSimulator_Bond -l Info --log-to-console -d'
> ```

### 6.3 开发机：启动 counter98_mock（AGW 登录）

```bash
# 开发机上直接运行（无需 docker）
cd /home/lsz/code/api_trunk

# gw 测试用 9001
nohup ./build_cmake/bin/counter98_mock \
  --config trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json \
  --port 9001 > /tmp/counter98_mock_9001.log 2>&1 &

# GOne 测试用 9003
nohup ./build_cmake/bin/counter98_mock \
  --config trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json \
  --port 9003 > /tmp/counter98_mock_9003.log 2>&1 &
```

### 6.4 开发机：启动 gone_counter_mock（GOne 测试需要）

```bash
# 开发机上直接运行
cd /home/lsz/code/api_trunk

nohup ./build_cmake/bin/gone_counter_mock \
  --config trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json \
  > /tmp/gone_counter_mock.log 2>&1 &
```

监听 44001(GW) + 44002(Core)。

### 6.5 验证开发机所有端口

```bash
ss -tlnp | grep -E "33001|33002|9001|9003|44001|44002|38140|38141|39142"
```

### 6.6 远程测试机：验证到开发机的网络连通性

```bash
# 用开发机实际 IP 替换 192.168.1.100
telnet 192.168.1.100 33001    # FTE 上海
telnet 192.168.1.100 9001     # counter98_mock (gw)
telnet 192.168.1.100 44001    # gone_counter_mock GW
telnet 192.168.1.100 44002    # gone_counter_mock Core
```

所有端口应能成功建立 TCP 连接。

> **注意**：开发机防火墙需放行以上端口。检查方式：
> ```bash
> # 开发机上
> firewall-cmd --list-ports    # 如有 firewalld
> iptables -L -n               # 或检查 iptables
> ```

---

## 7. 测试执行

### 7.1 准备测试用例目录

功能测试通过 `--testdir` 指定用例目录，遍历执行目录下所有 JSON 用例。

> **关键**：FTE 登录（`api_->login()`）由功能测试用例（`Login` 类型）触发。**必须用包含登录用例的目录**，否则 FTE 未登录，perf_test 会全部失败（`trade link not connected`，-22）。

**推荐**：创建只含登录用例的最小目录，快速完成登录后进入 perf_test：

```bash
# 远程测试机上
cd /home/user/api_trunk

# gw 登录用例
mkdir -p /tmp/fte_login_only
cp trunk/NewAPI/gone/api/mock/client/config/test_cases/fte_login.json /tmp/fte_login_only/

# GOne 登录用例
mkdir -p /tmp/gone_login_only
cp trunk/NewAPI/gone/api/mock/client/config/test_cases/gone_login.json /tmp/gone_login_only/
```

> 完整功能测试（含委托/撤单/回报校验）可用完整 `config/test_cases` 目录，但其中 `gone_combo.json`（GOne 委托等待回报）可能卡住，建议分离 gw/GOne 用例。

### 7.2 远程测试机：执行 mock_client

```bash
cd /home/user/api_trunk

# 设置库路径
export LD_LIBRARY_PATH=/home/user/api_trunk/build_cmake/lib:$LD_LIBRARY_PATH

# gw 单客户 8000 TPS
./build_cmake/bin/mock_client \
  --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_single.json \
  --testdir /tmp/fte_login_only

# gw 多客户 8000 TPS
./build_cmake/bin/mock_client \
  --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_multi.json \
  --testdir /tmp/fte_login_only

# GOne 8000 TPS
./build_cmake/bin/mock_client \
  --config trunk/NewAPI/gone/api/mock/client/config/connection_config_gone.json \
  --testdir /tmp/gone_login_only
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

### 8.3 远程测试机：执行抓包（需 root）

> **异地关键**：跨机测试时，抓包接口为**远程测试机的物理网卡**（如 `eth0`），而非回环接口 `lo`。

**流程**：先启动抓包程序（覆盖测试时长）→ 运行 mock_client → 抓包程序结束后读取映射文件输出报告。

```bash
# 远程测试机上，root 运行

# gw（33001 端口，120 秒窗口，物理网卡 eth0）
nohup ./build_cmake/bin/api_net_time_capture \
  --iface eth0 --port 33001 --map /tmp/api_net_time_map_single.txt \
  --proto gw --duration 120 --report /tmp/api_net_time_report_single.txt \
  > /tmp/capture_single.log 2>&1 &

# GOne（44002 Core 端口，120 秒窗口）
nohup ./build_cmake/bin/api_net_time_capture \
  --iface eth0 --port 44002 --map /tmp/api_net_time_map_gone.txt \
  --proto gone --duration 120 --report /tmp/api_net_time_report_gone.txt \
  > /tmp/capture_gone.log 2>&1 &
```

然后按 §7.2 运行 mock_client（在抓包窗口内），等待抓包程序自动结束（`--duration` 超时）。

**参数**：

| 参数 | 说明 |
|------|------|
| `--iface` | 抓包接口（跨机用物理网卡如 `eth0`；同机回环测试用 `lo`） |
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
|------|------|----------|
| API 内处理耗时 | 入队 + 引擎消费（send 前） | gw vs GOne |
| 网卡端到端延迟 | API 进入 → 网卡发出 | gw vs GOne |
| 内核+网卡排队差额 | 端到端 - API 内 | 公共基线，与柜台无关 |

**参考结论（2026-09-18，8000 TPS，同机回环）**：
- API 内：gw P50=290ns 优于 GOne 421ns；但 gw P95=2836ns 高于 GOne 732ns（FTE 背压）
- 端到端：gw P50=2856ns 略优于 GOne 3197ns；gw P95=6626ns 高于 GOne 4699ns
- 内核+网卡排队：两者均 ~2.5-4μs（公共基线）

> **异地跨机测试差异**：跨网络时，端到端延迟会增加网络 RTT（通常数百微秒至毫秒级），API 内处理耗时基本不变（纯本地 CPU 计算）。

---

## 10. 故障排查

| 现象 | 可能原因 | 处理 |
|------|----------|------|
| `cmake: command not found` | cmake 未安装或版本太旧 | 安装 cmake3（见 §3.1） |
| `CMake Error at CMakeLists.txt:2 (cmake_minimum_required): CMake 3.10 or higher is required` | cmake 版本低于 3.10 | 安装 cmake3 或编译安装 3.10+ |
| `api->start() 失败: -18` | counter98_mock 未启动或端口不对 | 检查开发机 9001/9003 监听；确认防火墙放行 |
| `connect_ch ret=-34` | 目标端口未监听或网络不通 | 检查开发机服务端状态；`telnet` 测试连通性 |
| perf_test 全部 `-22`（trade link not connected） | **FTE 未登录**（用空/错误 testdir 跳过登录） | 用含 `fte_login.json` 的目录；确认 FTE 33001 已启动 |
| 抓包捕获委托包 0 | 端口/协议参数错误 | GOne 用 44002 + `--proto gone`；gw 用 33001 + `--proto gw` |
| 抓包程序不退出 | `recvmsg` 阻塞无法检查 duration | 已内置 SO_RCVTIMEO 1s，正常应超时退出；检查是否旧版本 |
| 关联失败数高 | 抓包窗口未覆盖 mock_client 发单期 | 先启动抓包再运行 mock_client；增大 `--duration` |
| 功能测试卡在 GOne 委托 | `gone_combo.json` 等待回报卡住 | 用只含登录用例的目录 |
| 38140 进程 defunct | tgw_simulator 启动失败 | 单独重启 38140（§6.2），不影响委托测试 |
| 抓包需 root | AF_PACKET SOCK_RAW 权限 | 用 root 运行 |
| `make: command not found` | 未安装 build tools | `yum install make` |
| `fatal error: stddef.h: No such file or directory` | 未安装 gcc-c++ | `yum install gcc-c++` |
| 编译时 `-march=native` 报错 | 极旧的 CPU 不支持某些指令 | 可在 CMakeLists.txt 中移除 `-march=native -mtune=native` |
| 远程连接超时 | 防火墙未放行端口 | 开发机上放行 33001/9001/9003/44001/44002 端口 |

---

## 11. 附录

### 11.1 端口清单

| 端口 | 用途 | 所在机器 |
|------|------|:--------:|
| 33001 | 上海 FTE（gw 委托链路） | 开发机 |
| 33002 | 深圳 FTE | 开发机 |
| 9001 | counter98_mock（gw 测试） | 开发机 |
| 9003 | counter98_mock（GOne 测试） | 开发机 |
| 44001 | gone_counter_mock GW 链路（登录/心跳） | 开发机 |
| 44002 | gone_counter_mock Core 链路（委托） | 开发机 |
| 38140 | tgw_simulator Bond | 开发机 |
| 38141 | tgw_simulator Stock | 开发机 |
| 39142 | tgw_simulator ETF | 开发机 |

### 11.2 常用命令速查

| 操作 | 命令 |
|------|------|
| 远程机编译 API | `cd /home/user/api_trunk && mkdir -p build_cmake && cd build_cmake && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_MOCK=ON && make -j$(nproc)` |
| 远程机运行 mock_client | `LD_LIBRARY_PATH=./build_cmake/lib ./build_cmake/bin/mock_client --config ... --testdir ...` |
| 远程机运行抓包 | `sudo ./build_cmake/bin/api_net_time_capture --iface eth0 --port 33001 --proto gw --map ... --duration 120 --report ...` |
| 开发机启动 FTE | `docker exec otc bash -lc 'cd /mnt/work/gt_trunk && source ~/.zshrc && ./DYS-FRAMEWORK/fte/test_all/start_all.sh'` |
| 开发机启动 counter98_mock | `cd /home/lsz/code/api_trunk && nohup ./build_cmake/bin/counter98_mock --config ... --port 9001 > /tmp/counter98_mock.log 2>&1 &` |
| 开发机启动 gone_counter_mock | `cd /home/lsz/code/api_trunk && nohup ./build_cmake/bin/gone_counter_mock --config ... > /tmp/gone_counter_mock.log 2>&1 &` |
| 远程机测试网络连通性 | `telnet <开发机IP> <端口>` |

### 11.3 文件索引

| 文件 | 路径 |
|------|------|
| 本手册 | `trunk/NewAPI/gone/api/mock/client/api_test_manual.md` |
| gw 单客户配置 | `trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_single.json` |
| gw 多客户配置 | `trunk/NewAPI/gone/api/mock/client/config/connection_config_gw_multi.json` |
| GOne 配置 | `trunk/NewAPI/gone/api/mock/client/config/connection_config_gone.json` |
| gw 登录用例 | `trunk/NewAPI/gone/api/mock/client/config/test_cases/fte_login.json` |
| GOne 登录用例 | `trunk/NewAPI/gone/api/mock/client/config/test_cases/gone_login.json` |
| counter98_mock 配置 | `trunk/NewAPI/gone/api/mock/98_counter/config/server_config.json` |
| gone_counter_mock 配置 | `trunk/NewAPI/gone/api/mock/gone_counter/config/server_config.json` |
| CMakeLists（根） | `CMakeLists.txt` |
| CMakeLists（子项目） | `trunk/NewAPI/CMakeLists.txt` |
| mock_client CMakeLists | `trunk/NewAPI/gone/api/mock/client/CMakeLists.txt` |
| build.sh（依赖 docker） | `build.sh` |

### 11.4 RHEL 7.2 环境速查

```bash
# 安装必要工具
yum install -y gcc gcc-c++ make

# 安装 cmake3（EPEL）
yum install -y epel-release
yum install -y cmake3
alias cmake=cmake3
# 或写入 ~/.bashrc: echo 'alias cmake=cmake3' >> ~/.bashrc

# 验证版本
gcc --version        # 应为 4.8.5
cmake --version      # 应为 3.x
```