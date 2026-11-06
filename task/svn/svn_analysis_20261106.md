# SVN 更新冲突与待办分析报告（r17→r22）

> **生成日期**：2026-11-06
> **分析对象**：`trunk/NewAPI`（SVN 工作副本 `/mnt/work/api_trunk/trunk`，URL `https://jyjs2.svn.gtja.net/svn/NewAPI/trunk`，当前 r22）
> **输入依据**：`svn status` / `svn log -v -r 17:22` / `svn diff -r 20:21`、`.mine/.r17/.r22` 三份临时文件、上游两份说明文档（`开发者升级说明.txt`、`个微和98柜台-消息组包解包对比分析.txt`）
> **结论摘要**：7 个文件、11 处文本冲突，**全部集中在 `gone/api/mock/client/src/`**，本质不是"文字撞车"，而是**上游 r21 重构了公共接口（含库名/回调签名/结构字段）**，与我们本地对 mock 客户端的增强（test_plan/logger/性能打点/查询测试）正面重叠。另有 **47 处 `待确认/todo`**（28 处 `src/` + 19 处 `include/`），全部是**协议字典与联调语义未定**，而非功能缺失。

---

## 一、事实基线

### 1.1 版本时间线

| 版本 | 时间(UTC) | 作者 | 内容 | 对本仓库的影响 |
|------|-----------|------|------|---------------|
| r17 | 2026-10-13 07:55 | zouyi024572 | 新增 `gone/api-compare/`（另一个开发者的 API 完整副本，含 counter98 协议库） | 仅新增目录 |
| r18~r20 | 2026-10-19 09:18~09:19 | 122098（本侧） | GCC4.8.5 兼容 + 三 counter 文档 + 缓存类 + mock 用例；stop/close 分离 + 回调事件扩展；counter98 采线上 r17 + 撤单优化 + GwLoginCache | 我们的三次提交 |
| **r21** | **2026-11-05 01:33** | **zouyi024572** | **统一 API 委托入口，完善柜台登录/重连/回报处理**（含 counter98 正式协议库、`test/` 离线用例、`开发者升级说明.txt`、`个微和98柜台-消息组包解包对比分析.txt`，并**适配 mock 客户端**） | **接口大改：7 文件冲突的直接来源** |
| **r22** | **2026-11-05 07:21** | zouyi024572 | `aio_socket_link.cpp` 链接 family 设置错误修复 | 无冲突（本地未改该文件） |

> 本地首次冲突发生在更新到 r17 时（留下 `.r17`），随后更新到 r22 再次冲突（留下 `.r22`），两轮冲突**均未解决即被提交进 git**（提交 `bc03c3b "S:更新代码"`，2026-11-05 17:54，作者 `lsz_nuc_linux`）。

### 1.2 当前工作副本状态

```
?  35 项   未版本化（mock/*/build、*.log、result/ 等构建与日志产物）
C   7 项   文本冲突（全部在 gone/api/mock/client/src/）
M   6 项   本地已改、无冲突（`metric_stats.cpp`、`metric_stats.h`、`mock_client.h`、`perf_runner.h`、`test_report.cpp`、`test_report.h`）
```

### 1.3 上游 r21 的改动面（非 api-compare）

| 类别 | 文件 |
|------|------|
| 新增 | `gone/api/counter98/`（正式 98 协议库，含 `messages_agw_external.h` 等 20+ 文件）、`gone/api/src/gw_head.h`、`gone/include/gw_head.h`、`gone/api/test/`（`order_insert_test.cpp` / `test_api_config.cpp` / `test_order_dispatch.cpp` + `order_insert_test_support.h` + `run_order_insert_test.sh`）、两份说明 txt |
| 修改（include） | `api_callback.h`、`api_config.h`、`api_errno.h`、`api_interface.h`、`order_trade_type.h` 等 |
| 修改（src） | `api_instance`、`counter98`、`gw_counter_direct`、`fpga_counter_*`、`multi_socket_engine`、`single_socket_engine`、`tcpdirect_engine`、`callback_manager`、`link_timer_op`、`c98_session_cache.h`、`gw_login_cache.h`、`api_event_msg.h` 等 30+ 文件 |
| 删除 | `gone/api/api-requirements-prompt.md`（r21 已删除，本地已不存在） |
| 构建 | `gone/api/CMakeLists.txt`：产出库 **`liblbapi.so` → `libg1api.so`**（另按 Solarflare 环境追加 `libg1api_sf.so`） |

**接口级不兼容清单（来自 `开发者升级说明.txt`，已逐条核对源码）**：

| 维度 | 旧（r20 及本地） | 新（r21/r22） |
|------|-----------------|---------------|
| 库名 | `liblbapi.so` | **`libg1api.so`**（旧库/旧头文件不可混用） |
| 委托入口 | `order_insert` + `etf_order_insert` + `bse_order_insert` | **统一 `order_insert`**（ETF 用 `side='D'/'E'`，北交所填市场 109） |
| 错误回调 | `on_error(err_event_type, ...)` | **`on_event(api_event_type, ...)`** |
| 链接回调 | `on_link_status(int32_t counter_type, ...)` | **`on_link_status(counter_type, ...)`**（枚举） |
| 新增回调 | — | `on_logout`、`on_counter_status` |
| 柜台地址配置 | `set_attr("speed_counter_addr", "ip:port")` | **`set_attr(name, lb_api::net_addr{ip[..], port})`** |
| 多客户配置 | `single_cust_per_link` | **`need_multi_cust`** + 柜台类型 `gewei=1/fpga=2` 拆分 |
| 市场字典 | 1(沪)/2(深)/3 | **101(沪)/102(深)/109(北交所)** |
| 数量口径 | 股 | **股×100（100 股 ⇒ 10000）** |
| 委托计时字段 | `OrderReq.api_arrive_time_ns/api_leave_time_ns` | **已移除** |
| 其他 | `CreditMaxOrderQtyInfo.zero_reason` 为 string | **改定长字符数组** |

---

## 二、冲突分析

### 2.1 冲突总览

| # | 文件 | 冲突块 | 本地(.mine) | 上游(r22) | 冲突性质 |
|---|------|:-----:|:----------:|:---------:|---------|
| 1 | `callback_handler.cpp` | 3 | 249 行 | 179 行 | 回调签名漂移 + 本地链接状态跟踪 |
| 2 | `callback_handler.h` | 1 | 106 行 | 66 行 | 访问器声明形态 + 成员集差异 |
| 3 | `main.cpp` | 1 | 199 行 | 115 行 | 命令行帮助文本（本地新增参数） |
| 4 | `mock_client.cpp` | 3 | 416 行 | 241 行 | 配置写入方式（string→net_addr）+ 校验策略 |
| 5 | `perf_runner.cpp` | 1 | 238 行 | 222 行 | 计时字段下线 → 时延统计口径 |
| 6 | `test_case_runner.cpp` | 1 | 983 行 | 517 行 | 统一委托入口 + 异步成交等待逻辑 |
| 7 | `test_case_runner.h` | 1 | 135 行 | 111 行 | TestCaseType 枚举（上游删除 10 个查询类型） |

### 2.2 逐文件详解

#### ① `callback_handler.cpp`（3 块）

| 块 | 位置 | 本地意图 | 上游意图 | 建议 |
|----|------|---------|---------|------|
| 1 | L87 | `on_link_status(int32_t counter_type, ...)` + 注释说明"业务链接状态供委托/撤单前判断就绪" | `on_link_status(lb_api::counter_type counter, ...)` | **取上游签名**（接口权威），本地注释保留 |
| 2 | L98 | 记录 `LINK_TYPE_SPEED_TRADE` → `last_trade_link_status_`；日志用 `LOG_INFO` | 仅打印 `static_cast<int32_t>(counter)`；日志用 `std::cout` | **上游签名 + 本地业务逻辑**（保留 trade 链接跟踪与 LOG_* 宏），counter 参数做 `static_cast` |
| 3 | L116 | `LOG_ERROR("[Callback] on_error: ...")` | `std::cerr << "[Callback] on_event: ..."` | 合为 `LOG_ERROR("[Callback] on_event: ...")` |

#### ② `callback_handler.h`（1 块）

- 本地：**内联返回引用**（`const lb_api::LoginAns& last_login_ans() const`），含 `trade_link_ready()` 与 `last_trade_link_status_` 成员。
- 上游：**按值返回声明**（`lb_api::LoginAns last_login_ans() const`），另有 `last_error_desc()`。
- **建议**：以本地内联+引用版本为骨架（少拷贝、易用），补齐上游的 `last_error_desc()`；新增 `last_trade_link_status_` 成员保留；不改 `on_event` 的 `api_event_type` 参数类型。

#### ③ `main.cpp`（1 块）

- 本地：`--plan`（test_plan 主配置，推荐模式）、`--log`、`--analysis`、默认路径 `result/`。
- 上游：仅 `--config/--lib/--testcase/--testdir/--report`，`--lib` 默认 `libg1api.so`。
- **建议**：保留本地全部参数与默认 `result/` 路径，**仅把 `--lib` 帮助文本与默认值改成 `libg1api.so`**（库已改名，不改会误导后续使用者）。

#### ④ `mock_client.cpp`（3 块，最重）

| 块 | 本地 | 上游 | 建议 |
|----|------|------|------|
| 1 | `set_attr(...)` 逐个调用，失败仅 `LOG_DEBUG` | `check_attr` lambda：失败即 `throw std::runtime_error` | **取上游**（配置错误必须致命，避免"静默降级"造成假通过） |
| 2 | `set_attr("speed_counter_addr", "ip:port")` 字符串 | `set_address()` lambda：组 `lb_api::net_addr{ip,port}` + 合法性校验（空/超长/端口越界） | **取上游**（新接口只接受 net_addr，字符串写法编译不过） |
| 3 | 本地额外配置项（`heartbeat_interval`、`98agw_user`、日志等级等） | 上游增加 `need_multi_cust`、`agw_user_login_timeout`、`log_level` | **并集**：以"上游 check_attr 框架 + 双方全部配置项"重写该段，逐项核对 `api_config.h` 的 known_attrs 是否都存在 |

#### ⑤ `perf_runner.cpp`（1 块）

- 本地：`lat = req.api_leave_time_ns - req.api_arrive_time_ns`（API 内部打点）。
- 上游：该两字段已随结构下线，写死"latencies 保持空，compute 对空样本输出零值"。
- **建议**：**不要采用上游的"空统计"**（会让性能报告恒为 0，形成假数据）。改为：
  1. **首选**：mock 侧自建时间戳——发送前记 `steady_clock::now()`，回调到达时记结束（端到端"API→柜台→回报"延迟，口径变化需在报告中标注）；
  2. **次选**：若必须保留"纯 API 框架延迟"口径，需在 API 侧（`order_insert` 入队/发送点）**重新引入打点字段**，属 API 代码改动，需与上游确认；
  3. 网卡抓包关联映射（`net_time_map_`）同样依赖 `api_arrive_time_ns`，需改用 `client_seq_id + 发送时刻`（客户端时钟）重建映射。

#### ⑥ `test_case_runner.cpp`（1 块）

- 本地：异步成交回报(2005)等待逻辑 + `etf_order_insert` 分支（L448 `api_->etf_order_insert(req)`）。
- 上游：统一入口——`OrderInsert/EtfOrderInsert` 都走 `order_insert`，仅"空字段旧用例"才跳过发送。
- **建议**：**取上游结构**（统一入口 + 空字段兼容），在此基础上保留本地日志与结果分析钩子；`etf_order_insert` 分支改为 `order_insert` + `side='D'/'E'`。

#### ⑦ `test_case_runner.h`（1 块）

- 本地：枚举含 10 个查询类型（`OrderQuery/TradeQuery/FundQuery/PositionQuery/MaxOrderQtyQuery/TotalFundAssetQuery/FundMaxdrawQuery/CustMarginAvlQuery/AssetDebtsQuery/CreditMaxOrderQtyQuery`）。
- 上游：**整段删除这 10 个类型**（r21 的 mock 只留登录/委托/撤单/成交/心跳）。
- **建议**：**保留本地查询测试能力**（API 侧 `on_*_query_ans` 回调仍完整存在，删除只是 mock 侧功能退化）。需按新接口适配：查询请求字段 `user_seq_id` → `client_seq_id`；委托/成交批量查询改走 `order_batch_query`/`trade_batch_query`。

### 2.3 根因归纳（4 类）

| # | 根因 | 表现 | 说明 |
|---|------|------|------|
| R1 | **公共接口重构 vs 本地增强正面重叠** | 7 文件全部涉及回调签名/配置写入/委托入口 | r21 是一次 breaking change，本地 mock 客户端恰好是"围绕旧接口深度增强"的产物 |
| R2 | **上游删除本地已有功能** | 查询测试类型被删、性能打点被移除 | 上游为快速适配走"精简路线"，我方为可观测性走"增强路线" |
| R3 | **同一文件两侧大改，三点合并失败** | `test_case_runner.cpp`(983 vs 517)、`mock_client.cpp`(416 vs 241) | 两侧改动区域重叠超过 50%，SVN 行级合并必然冲突 |
| R4 | **流程缺失：冲突未解决即入库** | `.mine/.r17/.r22` 与冲突标记被提交进 git（`bc03c3b`） | 导致后续 `git diff` 看不到"真正的改动前状态"，并让编译长期处于坏状态 |

---

## 三、冲突解决方案

### 3.1 总体策略：**接口跟上游，能力保本地**（三层原则）

| 层 | 原则 | 适用 |
|----|------|------|
| **接口层** | 无条件采用上游（签名、枚举、库名、结构字段、配置方式） | `api_callback.h`/`api_interface.h`/`api_config.h`/`order_trade_type.h` 及全部 override |
| **能力层** | 保留本地增强（test_plan、logger、result_analysis、trade 链接就绪判断、查询测试、结果统计） | mock 客户端自有逻辑 |
| **口径层** | 上游删掉的度量能力**重设计而非删除**（如性能打点） | perf_runner / 网卡抓包映射 |

> 判断依据：`gone/api/include/*` 是**对外契约**，被其它下游（真实客户 SDK、test/）共用，不能为兼容 mock 而回退；`mock/client/**` 是**我方测试工具**，其增强没有外部依赖。

### 3.2 逐文件取舍表

| 文件 | 上游接口 | 本地增强 | 处置 |
|------|:--------:|:--------:|------|
| callback_handler.cpp | ✅ 全取 | 保留 LOG_* + trade 链接跟踪 | 手工合并（3 块均小） |
| callback_handler.h | 部分（`last_error_desc`） | ✅ 保留内联引用访问器 + `trade_link_ready` | 以本地为骨架补齐 |
| main.cpp | 仅 `--lib` 默认值 | ✅ 全保留 | 改 2 处字符串 |
| mock_client.cpp | ✅ 取 check_attr/set_address/need_multi_cust | ✅ 保留 test_plan/perf/analysis 流程 | 重写配置段（并集） |
| `perf_runner.cpp`、`perf_runner.h` | 字段下线事实 | ✅ 打点能力需重建 | 改口径（见 3.4 P1-3） |
| test_case_runner.cpp | ✅ 统一委托入口 | ✅ 保留查询测试与日志 | 合并 + 改 `etf_order_insert` |
| test_case_runner.h | 上游删 10 类型 | ✅ 保留 10 个查询类型 | 保留本地枚举 |

### 3.3 执行步骤（建议按序，每步可独立验证）

| 步 | 动作 | 验证 |
|:--:|------|------|
| S0 | **冻结现状**：记录 `svn status` 与冲突文件清单；确认 `.mine/.r17/.r22` 三份副本已归档（`task/svn/evidence/` 或 git 已跟踪） | 清单与本报告一致（7 文件 11 块） |
| S1 | **判定基线**：明确"以 r22 为内容基线，本地增强逐项移植"，不做 `svn revert`（会丢本地增强） | 评审确认 |
| S2 | 处理 `include/` 侧无冲突但语义变化的引用：market 1/2/3→101/102/109、`user_seq_id`→`client_seq_id`、`zero_reason` 定长 | `grep -rn` 清单归零 |
| S3 | 逐文件解决 11 处冲突（按 3.2 表），**每解一个立即 `svn resolve --accept working <file>`** | `svn status` 的 C 逐项转 M |
| S4 | 删除临时文件：`.mine/.r17/.r22`（解决后 `svn resolve` 会清理，或手工 `rm` 并从 git 索引移除） | `svn status` 无 `?` 于 src/ |
| S5 | 全量编译：`./build.sh Release -DBUILD_MOCK=ON`（库名 `libg1api.so`） | 0 error；`build_cmake/lib/libg1api.so` 存在 |
| S6 | 离线回归：`gone/api/test/run_order_insert_test.sh`（上游称 **743 项检查**） | 全绿 |
| S7 | 功能回归：mock_client `--plan` FTE / GOne 两套（登录/委托/成交/撤单/心跳 5/5） | 5/5 PASS |
| S8 | 性能回归：确认新计时口径并在报告中标注口径变化（避免与历史数据误比） | 报告含口径说明 |
| S9 | 提交：**先 git（含冲突解决记录）后 svn**；提交前 `git diff --check` + 禁止冲突标记入库 | 见 §6 流程红线 |

### 3.4 优先级建议

| 优先级 | 事项 | 理由 |
|:------:|------|------|
| **P0** | S2+S3+S5（接口对齐 + 解决冲突 + 编译通过） | 当前**编译处于坏状态**（冲突标记 + 已下线符号），一切验证无从谈起 |
| **P1-1** | `etf_order_insert` 分支改统一入口（side D/E） | 否则 ETF 用例必挂 |
| **P1-2** | 市场字典 101/102/109 全量替换 | 否则委托被拒/回报错乱，且难定位 |
| **P1-3** | 性能打点重建（客户端时间戳口径） | 否则性能报告恒 0，掩盖回归 |
| **P2** | 查询测试类型适配（`client_seq_id`、批量查询） | 能力恢复，非阻塞主链路 |
| **P2** | 35 项未版本化产物加 `svn:ignore`（build/、*.log、result/） | 消除 `svn status` 噪声，防误提交 |
| **P3** | 把"冲突不得入库"写入 `task_rule.md` §13 提交前检查 | 流程红线（见 §6） |

---

## 四、非冲突但受影响的"隐藏地雷"

> 这些位置**不在冲突列表里**（自动合并成功），但语义已变，**编译或运行必然出问题**。必须在 S2 阶段一并处理。

| # | 位置 | 问题 | 处置 |
|---|------|------|------|
| 1 | `mock/client/src/perf_runner.cpp:120,127` | `req.api_arrive_time_ns/api_leave_time_ns` 字段已不存在 | 改客户端时间戳口径（P1-3） |
| 2 | `mock/client/src/test_case_runner.cpp:448` | `api_->etf_order_insert(req)` 接口已删 | 统一 `order_insert` + side D/E |
| 3 | `mock/client/config/*.json` | 上游已把 `perf_runner.h` 默认值改为 `order_market_type = 101`（自动合并生效），但**5 个连接配置里有 4 个仍写 `market_type: 1`**，`cases/*_expected.json`（login/order/cancel）也仍期望 `market_type: 1`；且 `single_cust_per_link` 键已被 `need_multi_cust` 取代 | 逐项改 `101`（沪）/`102`（深）；`connection_config.json` 已由上游更新为 101，可作为样板；`single_cust_per_link` → `need_multi_cust`（`true`→`0`、`false`→`1`，语义相反需注意） |
| 4 | `mock/client/src/*` 全部 override | `on_error`/`int32_t counter_type` 已废 | 改 `on_event(api_event_type)` / `counter_type` |
| 5 | 构建脚本与文档 | 仍写 `liblbapi.so`、`build_cmake/lib/liblbapi.so` | 统一 `libg1api.so`（`build.sh`、各 `.md`、`study/*`） |
| 6 | 配置样例 `config/*.json` | 缺 `need_multi_cust`、`agw_user_login_timeout`、`log_level`，且 `fast_counter_type=3` 已废 | 按升级说明：旧网关配 3 → `fast_counter_type=2` + `need_multi_cust=1` + `speed_link_type=0` |
| 7 | 数量口径 | 委托/回报统一"股×100"（100 股 = 10000） | 检查 mock 用例与预期文件中的数量值，防"同一笔乘两次 100" |
| 8 | 撤单三号码 | `CancelReq.client_req_no`（本次撤单号）/`client_seq_id`（原委托客户号）/`order_sys_no`（原委托柜台号） | 用"三者互不相同"的用例验证关联 |

---

## 五、`待确认 / todo` 分析（47 处）

### 5.1 分布

| 文件 | 数量 | 主题 |
|------|:----:|------|
| `src/counter98.cpp` | 16 | 98 协议字典映射、登录语义、回报分类、ETF 业务类型 |
| `include/struct_req.h` | 14 | 14 类业务委托**回报未知**（协议待补） |
| `src/gw_counter_direct.cpp` | 9 | FTE 字典（side/order_type）、order_sys_no 来源、ETF 固定 '2'、拒绝范围 |
| `src/counter98.h` | 2 | 回报委托类型暂填 0、定长填充约定 |
| `include/common_struct.h` | 2 | `transact_time` 语义与精度 |
| `include/order_trade_type.h` | 2 | 卖券还款 `side='X'`、98 回报 order_type 暂填 0 |
| `include/api_interface.h` | 1 | 卖券还款暂不支持 |
| `src/fpga_counter_base.cpp` | 1 | 已在 r21 解决（注释标"原 todo 已解"） |

> 说明：`mock/` 与 `api-compare/` 内的匹配均为注释里的 `\uXXXX` 转义说明等误报，**无真实待办**；`api-compare/` 属另一开发者副本，不在本次范围。

### 5.2 A 组：98 柜台字典映射（P0，联调首验）

| 位置 | 现状 | 风险 | 建议方案 |
|------|------|------|---------|
| `counter98.cpp:290` | 委托市价类型"如何映射 98 字典"未定 | 市价单发错报文类型，柜台拒单 | 依升级说明：**出站** 限价 2→2、本方最优 4→U、五档剩转限价 3→K、其余已定义市价 1/5/6/7→1；落表为 `static constexpr` 映射函数 + 单测 |
| `counter98.cpp:320` | 98 委托方向/业务类型 → API 方向字典未定 | 回报方向错乱，风控误判 | 建"98 side/business_type → API side"反向映射表；先按报文实测字典对齐，缺项显式 `LOG_WARN` 而非静默填 0 |
| `counter98.cpp:339`、`1943`、`1970`、`counter98.h:323` | 回报中 `order_type` 暂填 0 | 调用方把 0 当有效类型 | **不改回报 0，而是定义"0 = 未知"语义**并在文档/头注释声明；调用方从原委托记录取类型（升级说明已明确此口径） |
| `counter98.cpp:392` | ETF `business_type` 取 12（实时申赎）待确认 | ETF 报文被拒 | 与柜台确认后再固化；当前保留常量 + 注释指向 `AgwBusinessType` 枚举 |
| `counter98.cpp:399` | ETF 只填实例市场股东账号，另一市场留空 | 跨市场 ETF 失败 | 保留现状并记入联调清单；若失败再补另一市场账号 |
| `counter98.h:491` | C 串→定长字段的填充约定（尾部空格 or `\0`）待确认 | 与 FTE 的 pbu 空格填充同类坑（历史已踩） | **复用历史结论**：FTE 侧要求空格填充，98 侧需实测首笔应答；统一成 `pad_copy(p,' ',n)` 一处实现 |

### 5.3 B 组：98 登录与回报语义（P0，最高风险）

| 位置 | 现状 | 风险 | 建议方案 |
|------|------|------|---------|
| `counter98.cpp:819`、`1536` | `AgwMsgLogin` 无错误字段，"收到同号(3)入站消息即视为登录成功" | **登录失败被当成功**，后续全部委托失效 | 三重判定：① 应答消息同号；② 后续心跳/查询可达；③ 升级说明要求"接入失败从 `login()` 返回"——把 AGW 登录结果以返回值而非回调表达 |
| `counter98.cpp:811` | `sender_comp_id` = agw 用户名 | 柜台校验失败 | 与柜台确认；先保留并记联调项 |
| `counter98.cpp:1213`、`1225` | 发起验证时间格式、`pwd_type`（明文）待确认 | 登录失败或安全隐患 | 明文密码先用（柜台内网），`pwd_type` 待协议确认；**禁止把密码写进日志**（当前已有前车之鉴：`task_rule` §7 教训） |
| `counter98.cpp:1821`、`1873` | 用 `orig_cli_ord_no != 0` 区分撤单响应 vs 委托确认；"是否含撤单确认/撤单成交/撤单废单"未定 | 回报归类错误 → 调用方状态机错乱 | 依升级说明："按执行类型分流，**实际成交才进 `on_trade_rtn`**，其余走 `on_order_rtn`"；撤单应答统一 `on_cancel_rsp`；用"撤单成功/撤单废单/撤单成交"三用例验证 |
| `counter98.cpp:1367` | `account_id` 按市场匹配，缺市场字典 | 股东账号填错 | 与 A 组市场字典同一张表解决 |
| `counter98.cpp:8` | `host_to_network=false`（主机序直通）待联调首验 | 字节序错 → 全字段乱码 | 联调第 1 步验证：发一条最小报文，比对柜台抓包字节序 |

### 5.4 C 组：个微（FTE）字段映射（P1）

| 位置 | 现状 | 风险 | 建议方案 |
|------|------|------|---------|
| `gw_counter_direct.cpp:370,371` | FTE `side`/`order_type` 字典"未证实，按与 ATP 相同透传" | 买卖方向/类型被柜台误解析 | 联调用"买/卖 × 限价/市价"四笔最小集验证；差异点集中到 `map_side()/map_order_type()` 两个函数 |
| `gw_counter_direct.cpp:388` | ETF 申购赎回固定填 `'2'`（待确认注释） | ETF 类型错 | 与柜台确认；保留单一常量便于一处修改 |
| `gw_counter_direct.cpp:206,873` | `order_sys_no` 取 `order_id`（待确认"个微 order_id = order_sys_no"） | 撤单定位失败 | **注意历史教训**：FTE 撤单定位只看 `orig_clordno`；r21 后策略为 `OrderRtn.order_sys_no` 直接暴露柜台号并原样回传，需核对 `fte_order_sys_no()` 取值是否与撤单期望一致，用"三号互异"用例验证 |
| `gw_counter_direct.cpp:874` | 撤单回报 `client_seq_id` 是否即撤单请求号 | 撤单应答关联不上 | 同上用例验证；不一致则在 `build_cancel_msg` 处显式记录请求号映射 |
| `gw_counter_direct.cpp:882` | 拒绝消息是否包含撤单等请求 | 撤单失败无回报 → 挂起 | 对齐 r21"按执行类型分流"口径；先按"包含"实现并加超时兜底 |
| `gw_counter_direct.cpp:200,201` | `ord_status`/`market_id` 字典映射待确认 | 状态/市场错 | 状态字典已历史验证过半（0-8→ORDER_STATE_*）；市场按 101/102 重校 |

### 5.5 D 组：通用结构与能力缺口（P2）

| 位置 | 现状 | 建议方案 |
|------|------|---------|
| `common_struct.h:95,162` | `transact_time` 语义与精度 todo | 统一为"交易所成交/回报时间，纳秒或 YYYYMMDDHHMMSSmmm 之一"，与柜台取证后固化，写入 `doc/order_trade_type.md` |
| `order_trade_type.h:48`、`api_interface.h:72` | 卖券还款 `side='X'` 未接入，98 业务映射未定 | 保持"返回不支持"的显式语义（好于静默失败）；接入时补 98 报文映射 |
| `struct_req.h`（14 处） | 网上发行/配股/债券转股回售/要约收购/网络投票/港股通/盘后定价/担保品划转/现券还券/新三板/ETF 网下认购等**回报未知** | 属**协议资料缺口**，非代码缺陷。建议：① 建"业务号→请求/回报消息号"对照表（`doc/`）；② 每条标注"已验证/待协议"，缺协议前不发请求或显式返回不支持 |

### 5.6 待确认项的统一管理建议

1. **集中登记**：新建 `doc/PENDING_CONFIRMATIONS.md`（**待新建**，当前不存在），把 47 处按 A~D 组登记为表格（位置 / 待确认点 / 谁确认 / 验证用例 / 状态），代码里保留 `[待确认]` 标记并附表格编号（如 `[待确认#A3]`）。
2. **联调优先序**：`counter98.cpp:8`（字节序）→ B 组登录语义 → A 组字典 → C 组 FTE → D 组协议。
3. **禁止静默填 0**：所有"暂填 0"必须在回报结构里可区分（0 = 未知），不得进入业务判定分支。

---

## 六、流程红线（本次暴露的工程问题）

| # | 问题 | 影响 | 建议规则 |
|---|------|------|---------|
| 1 | **冲突未解决即入库**（`bc03c3b` 提交了带 `<<<<<<< .mine` 的文件与 `.mine/.r17/.r22`） | 编译长期坏、后续 diff 失去基线、误导他人 | 提交前强制 `git diff --check` + `grep -rnE -e '<<<<<<<' -e '>>>>>>>' <待提交路径>` 必须为空；命中即拒绝提交 |
| 2 | **SVN 更新前未先提交/暂存本地改动** | 两轮冲突叠加（r17 未解决又撞 r22） | 更新前置检查：`svn status` 有 M/C 时先提交或 `svn diff > backup.patch` |
| 3 | **二进制/日志产物未忽略**（35 项 `?`） | `svn status` 噪声，易误 add | 配 `svn:ignore`：`build/`、`*.log`、`result/`、`*.mine`、`*.r[0-9]*` |
| 4 | **库名变更未同步文档/脚本** | 误导（`liblbapi.so` 已不存在） | 库名作为单一事实：从 `CMakeLists.txt` 读取，文档引用处集中列在 `Prompt.md` 代码索引 |

> 建议把 #1、#2 追加进 `task/task_rule.md` §13（git 提交与推送规则）作为硬性检查项，并同步到 `.chat/rules/project_rules.md`。

---

## 七、验证方案（解决后必须全部通过）

| 层 | 命令 / 用例 | 通过标准 |
|----|------------|---------|
| 构建 | `./build.sh Release -DBUILD_MOCK=ON` | 0 error；产出 `libg1api.so` + `mock_client` |
| 离线 | `gone/api/test/run_order_insert_test.sh` | 743 项全绿（上游基线） |
| 冲突清洁 | `grep -rnE -e '<<<<<<<' -e '>>>>>>>' trunk/NewAPI` | 0 命中 |
| 功能 | `mock_client --plan test_plan_gw.json` / `test_plan_gone.json` | 登录/委托/成交/撤单/心跳 5/5 |
| 字段 | 撤单"三号互异"用例；市场 101/102/109；数量 ×100 | 关联成功、回报字典正确 |
| 性能 | `--plan` 的 perf 段 | 报告非 0；口径已在报告标注 |

---

## 八、遗留与风险

| # | 遗留 | 风险等级 | 说明 |
|---|------|:--------:|------|
| 1 | 上游删除的 10 个查询测试类型 | 中 | 本地保留后需适配 `client_seq_id`/批量查询，工作量中等 |
| 2 | 性能打点口径重建 | 中高 | 影响历史数据可比性；建议在报告头写"口径 v2（客户端端到端）" |
| 3 | 47 处待确认 | 高 | 全部依赖**真实柜台联调**，无法离线闭环；离线只能保证"不崩"，不能保证"业务正确" |
| 4 | `api-compare/` 双副本 | 低 | 另一开发者副本，本次未纳入；后续需明确"谁是主干"，避免第三份分叉 |
| 5 | r21 后 `net_time_map_` 抓包关联 | 中 | 依赖 `api_arrive_time_ns` 的映射需重建，否则抓包比对工具失效 |

---

## 附：可复现命令

```bash
# 工作副本状态（必须 LANG，否则中文文件名报 UTF-8 错误）
docker exec otc bash -lc "cd /mnt/work/api_trunk/trunk && export LANG=zh_CN.utf8 && svn status"

# 冲突清单与临时文件
docker exec otc bash -lc "cd /mnt/work/api_trunk/trunk/NewAPI/gone/api/mock/client/src && ls -la *.mine *.r17 *.r22"

# 冲突块逐块查看
grep -nE -e '<<<<<<<' -e '=======' -e '>>>>>>>' trunk/NewAPI/gone/api/mock/client/src/callback_handler.cpp

# 上游改动面
docker exec otc bash -lc "cd /mnt/work/api_trunk/trunk && svn log -r 21 -v | grep -E '^   [MAD] /trunk/NewAPI/gone/api/(include|src)/'"

# 待确认/todo 清单
cd trunk/NewAPI/gone/api && grep -rInE "待确认|\btodo\b" src include | wc -l    # 47
```
