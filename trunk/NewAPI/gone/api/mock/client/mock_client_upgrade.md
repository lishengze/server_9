
# mock_client 全面复盘与优化建议

> 生成日期：2026-09-21  
> 复盘范围：`trunk/NewAPI/gone/api/mock/client/` 全部源文件（~2800 行）  
> 复盘版本：日志系统（logger.h/cpp）与结果分析（result_analysis.h/cpp）落地后

---

## ✅ 修复状态（2026-09-21 更新）

本复盘提出的问题已按优先级修复并通过回归测试（GW 5/5 + GOne 5/5，性能 0 失败）。

| # | 严重度 | 问题 | 状态 |
|---|---|---|---|
| 1 | 🔴 | load_api() 忽略 --lib | ✅ 已修复（dladdr 定位实际库路径 + 不一致 WARN） |
| 2 | 🔴 | 无链接就绪等待 | ✅ 已修复（wait_link_ready 轮询 last_link_status） |
| 3 | 🟡 | run_test_plan 忽略 perf 失败 | ✅ 已修复（返回 perf 结果，未启用不算失败） |
| 4 | 🟡 | net_time_map_ 含失败委托条目 | ✅ 已修复（仅成功委托记录映射） |
| 5 | 🟡 | Logger::min_level_ 无锁读取 | ✅ 已修复（改 std::atomic<int>） |
| 6 | 🟡 | validate 数组死代码 | ✅ 已移除 |
| 7 | 🟡 | ETF 委托类型未实现 | ✅ 已实现（EtfOrderInsert → etf_order_insert） |
| 8 | 🟢 | 多余 iostream 包含 | ✅ 已移除 |
| 9 | 🟢 | timeout_ms 重复计算 | ✅ 已提取局部变量 |
| 10 | 🟢 | None/Unknown 语义 | ✅ 已加 Unknown 警告 |
| 11 | 🔴 | ~~CallbackHandler 数据竞争~~ | ⚠️ **撤销**：复查确认当前实现已在线程安全（数据赋值在锁内 + wait_for_response 同锁建立 happens-before），无需修改 |

> 说明：原复盘将 #11（CallbackHandler 数据竞争）列为高风险，深入复查后发现各 `on_*` 回调的数据赋值已在 `lock_guard` 锁内，且主线程通过 `wait_for_response()`（持同一把锁）看到标志后读取数据，构成正确的 happens-before 同步。**该问题实际不存在**，故撤销。

---

## ✅ 修复结果与分析（2026-09-21，详细实现）

> 本节逐项说明各修复的具体实现、代码位置、验证方式与结论。修复均经 `--plan` 模式回归验证（GW 功能 5/5 + GOne 功能 5/5，性能 0 失败）。

### 修复 1：load_api() 忽略 --lib → dladdr 定位实际库路径（🔴 高）

**实现**（`src/mock_client.cpp:34-52`）：
```cpp
bool MockClient::load_api(const std::string& lib_path) {
    LOG_INFO("[MockClient] 使用直接链接方式加载 API");
    Dl_info info;
    if (dladdr((void*)&lb_api::api_config::create_config, &info) && info.dli_fname) {
        std::string actual(info.dli_fname);
        LOG_INFO("[MockClient] 实际加载的 API 库: " << actual);
        if (!lib_path.empty() && actual != lib_path) {
            LOG_WARN("[MockClient] --lib 指定路径与实际加载库不一致: 指定=" << lib_path
                      << ", 实际=" << actual << " (直接链接模式下 --lib 不生效)");
        }
    } else {
        LOG_WARN("[MockClient] 无法定位 API 库路径 (dladdr 失败)");
    }
    return true;
}
```

**分析**：
- 采用复盘建议的**方案 B（最小修复）**：mock_client 当前为直接链接方式（编译时已链接 liblbapi.so），无法真正 `dlopen` 指定路径，故用 `dladdr` 反查实际生效的库路径，与 `--lib` 参数比对，不一致时输出 WARN。
- 相比方案 A（改 dlopen），改动最小、不改变链接模型，同时消除了"用户误以为指定路径生效"的隐患。
- **结论**：`--lib` 参数不再静默忽略，路径不一致时明确告警；后续如需真正动态加载可再升级为 dlopen。

### 修复 2：无链接就绪等待 → wait_link_ready（🔴 高）

**实现**（`src/mock_client.cpp:54-74`）：
```cpp
bool MockClient::wait_link_ready(int timeout_ms) {
    if (!callback_) { LOG_WARN("[MockClient] 回调未初始化，跳过链接就绪等待"); return false; }
    auto start = std::chrono::steady_clock::now();
    while (!callback_->last_link_status()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            LOG_WARN("[MockClient] 等待链接就绪超时 (" << timeout_ms << "ms)");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    LOG_INFO("[MockClient] 链接已就绪");
    return true;
}
```

**分析**：
- `api_->start()` 返回成功仅表示 API 实例启动，与柜台（FTE/98）的 TCP 链接是**异步建立**的，由 `on_link_status` 回调通知。
- 修复后每 50ms 轮询 `callback_->last_link_status()`，直到就绪或超时（`run_test_plan` 中调用 `wait_link_ready(10000)`）。
- **结论**：消除了"链接未建立时立即执行测试导致大面积发送失败"的问题，是回归测试稳定通过的关键前置。

### 修复 3：run_test_plan 忽略 perf 失败 → 返回 perf 结果（🟡 中）

**实现**（`src/mock_client.cpp:328-359`）：
```cpp
bool perf_ok = true;
if (test_plan.has("perf_test")) {
    JsonValue perf_node = test_plan["perf_test"];
    // 若配置了 order_file 则加载并注入 order 节点
    if (perf_node.is_object() && perf_node.has("order_file")) {
        JsonValue order_root = JsonParser::parse_file(base_dir + "/" + order_file);
        JsonValue order = order_root.has("order") ? order_root["order"] : order_root;
        JsonValue new_perf = perf_node;
        new_perf.set("order", order);
        perf_node = new_perf;
    }
    bool perf_enabled = perf_node.is_object() && perf_node.has("enable") && perf_node["enable"].as_bool();
    if (perf_enabled) {
        perf_ok = run_perf_test(perf_node);   // ← 关键：纳入返回值
    } else {
        LOG_INFO("[MockClient] 性能测试未开启（perf_test.enable=false）");
    }
}
return perf_ok;
```

**分析**：
- `run_test_plan()` 末尾 `return perf_ok`，`main.cpp` 中 `ok = client.run_test_plan(...)` 现在能正确反映性能测试成败，进程退出码不再误判为 0。
- **关键细节**：仅当 `perf_test.enable=true` 才执行 `run_perf_test`；未配置/未启用 perf 时 `perf_ok` 保持 `true`（**不算失败**），避免"未跑压测也被判失败"。
- **结论**：功能测试与性能测试的成败分开报告，但 `run_test_plan` 返回值反映 perf 状态，符合 CI/CD 判定需求。

### 修复 4：net_time_map_ 含失败委托条目 → 仅成功委托记录（🟡 中）

**实现**（`src/perf_runner.cpp`，`run_benchmark()`）：
```cpp
int32_t ret = api_->order_insert(req);
if (ret == 0) {
    net_time_map_.emplace_back(client_seq_id, api_arrive_time_ns);
}
```

**分析**：
- 失败委托不会实际发出，网络上抓不到对应包；此前无条件记录会导致 `api_net_time_capture` 关联时出现永远无法匹配的条目，`unmatched` 数偏高。
- 修复后仅成功委托（`ret == 0`）记录映射；`api_leave_time_ns` 仍照常统计，不影响延迟统计。
- **结论**：消除了网络抓包关联的虚假 unmatched 条目，统计更干净。

### 修复 5：Logger::min_level_ 无锁读取 → std::atomic<int>（🟡 中）

**实现**（`src/logger.h:76` + `src/logger.cpp`）：
```cpp
std::atomic<int> min_level_;   // logger.h: 最低输出级别（atomic 支持无锁快速路径判级，避免 data race）

// logger.cpp log(): 加锁前先判级
if (static_cast<int>(level) < min_level_.load(std::memory_order_relaxed)) {
    return;
}
// logger.cpp set_level():
min_level_.store(static_cast<int>(level), std::memory_order_relaxed);
```

**分析**：
- 复盘指出 `log()` 在获取锁之前读取 `min_level_`，与 `set_level()` 的写入构成 data race（尽管风险极低）。
- 修复为 `std::atomic<int>`，`load/store` 用 `memory_order_relaxed`（判级无需全序），**仍保持无锁快速路径**，同时消除 data race。
- `level()` getter 同步改为 `min_level_.load(memory_order_relaxed)`（logger.h:53）。
- **结论**：符合复盘建议，无锁判级性能不变，线程安全提升。

### 修复 6：validate 数组死代码 → 移除（🟡 中）

**实现**（`src/test_case_runner.cpp`，`load_single_case()`）：删除 `validate` 数组兼容分支（原 lines 80-89，约 10 行）。

**分析**：
- 旧格式 `validate` 数组已被 `null` 字段标记取代（`cases/` 新格式），此分支实际为死代码，且当 `fields` 缺失时 `fields[field_name]` 会抛异常。
- 所有预期文件已统一使用 `null` 标记动态字段，删除后无影响。
- **结论**：消除死代码与潜在异常路径。

### 修复 7：ETF 委托类型未实现 → EtfOrderInsert 走 etf_order_insert（🟡 中）

**实现**（`src/test_case_runner.cpp:365-401`，`send_request()`）：
```cpp
case TestCaseType::OrderInsert:
case TestCaseType::EtfOrderInsert: {
    // ... 构造 OrderReq ...
    bool is_etf = (tc.request_type == TestCaseType::EtfOrderInsert);
    int32_t ret = is_etf ? api_->etf_order_insert(req) : api_->order_insert(req);
    LOG_INFO("[Runner] " << (is_etf ? "etf_order_insert" : "order_insert") << "() 返回: " << ret);
    return ret == 0;
}
```

**分析**：
- 此前 `EtfOrderInsert` 无 switch 分支，会落到 `default` 报"不支持请求类型"。
- 采用**实现 ETF 委托发送**方案（而非映射到 OrderInsert）：`EtfOrderInsert` 复用与 `OrderInsert` 相同的 `OrderReq` 结构，但调用 `api_->etf_order_insert()`（区分普通委托与 ETF 委托）。
- **结论**：`etf_order_insert` 类型用例可正常执行。

### 修复 8：多余 iostream 包含 → 移除（🟢 低）

**实现**：移除 `test_case_runner.cpp` / `callback_handler.cpp` / `test_report.cpp` / `perf_runner.cpp` / `mock_client.cpp` 中不再使用的 `#include <iostream>`（替换为 `LOG_*` 宏后已不再用 `std::cout`/`std::cerr`）。

**分析**：略微减少预处理时间，影响极小，纯代码整洁性提升。

### 修复 9：timeout_ms 重复计算 → 提取局部变量（🟢 低）

**实现**（`src/test_case_runner.cpp:232`，`execute()`）：
```cpp
int64_t timeout_ms = tc.timeout_ms > 0 ? tc.timeout_ms : 5000;   // 函数开头提取一次
```

**分析**：原 `tc.timeout_ms > 0 ? tc.timeout_ms : 5000` 在 TradeRtn、OrderCancel、wait_for_response 三个分支重复出现，现统一用局部变量，消除重复。

### 修复 10：None/Unknown 语义重叠 → 加 Unknown 警告（🟢 低）

**实现**（`src/test_case_runner.cpp:67-75`，`load_single_case()` 与 `load_plan()`）：
```cpp
if (test_case.request_type == TestCaseType::Unknown) {
    LOG_WARN("[Loader] 用例 '" << test_case.name << "' 的请求类型未知: '" << req["type"].as_string() << "'");
}
if (test_case.response_type == TestCaseType::Unknown) {
    LOG_WARN("[Loader] 用例 '" << test_case.name << "' 的预期回报类型未知: '" << exp["type"].as_string() << "'");
}
```

**分析**：`None`（故意无请求/无预期）与 `Unknown`（配置错误）语义不同，此前混用可能意外跳过校验。现对 `Unknown` 输出 WARN 日志，提示配置错误，避免被误认为通过。

### 撤销项 11：CallbackHandler 数据竞争（⚠️ 撤销）

**复查结论**（`src/callback_handler.h/.cpp`）：
- 各 `on_*` 回调的**数据赋值已在 `lock_guard` 锁内**（`last_login_ans_`/`last_order_rtn_` 等），标志位 `response_received_` 也在同一把锁内设置。
- 主线程通过 `wait_for_response()`（**持同一把锁**）看到标志后读取数据，构成正确的 **happens-before 同步**（锁的获取/释放建立内存屏障）。
- 复盘初稿误以为"数据写在锁外、标志在锁内"，深入复查后确认**该问题实际不存在**，故撤销，未做任何改动。

---

## ✅ 回归测试结果（2026-09-21）

修复后通过 **--plan 主配置模式** 进行功能 + 性能回归：

| 柜台 | 功能测试 | 性能测试 | 结果 |
|------|---------|---------|------|
| **GW (FTE)** | 5/5 通过（登录/委托/成交/撤单/心跳） | 6518 TPS，P50=531ns | 0 失败 ✅ |
| **GOne** | 5/5 通过（登录/委托/成交/撤单/心跳） | 9216 TPS，P50=411ns，P90=641ns | 0 失败 ✅ |

**回归命令**：
```bash
# GW (FTE)
LD_LIBRARY_PATH=build_cmake/lib ./build_cmake/bin/mock_client \
  --plan mock/client/config/test_plan_gw.json
# GOne
LD_LIBRARY_PATH=build_cmake/lib ./build_cmake/bin/mock_client \
  --plan mock/client/config/test_plan_gone.json
```

**回归分析**：
- 功能测试 5/5 通过，验证了登录/委托/成交/撤单/心跳全链路字段校验正确（含 `wait_link_ready` 链接就绪等待、`$last_order_sys_no` 动态引用等）。
- 性能测试 0 失败，验证了 `run_test_plan` 返回 perf 结果、`net_time_map_` 仅成功委托等修复不影响发单路径。
- 结果分析写入 `result_analysis.txt`（功能 + 性能 + 总体结论），供自动化判定。

---

## 目录

1. [概述](#1-概述)
2. [线程安全](#2-线程安全)
3. [错误处理与资源管理](#3-错误处理与资源管理)
4. [设计问题](#4-设计问题)
5. [性能优化](#5-性能优化)
6. [代码整洁度](#6-代码整洁度)
7. [测试覆盖与功能缺口](#7-测试覆盖与功能缺口)
8. [各文件逐项问题清单](#8-各文件逐项问题清单)
9. [优先级建议](#9-优先级建议)

---

## 1. 概述

mock_client 是面向 NewAPI 框架的测试客户端，支持 JSON 配置化的功能测试（登录/委托/撤单/心跳/成交回报逐字段校验）与性能测试（匀速发单+统计+TPS 控制）。代码整体质量较好：职责分离清晰（MockClient / TestCaseRunner / CallbackHandler / PerfRunner / Logger / ResultAnalysis）、异常处理覆盖主要路径、资源管理基本正确。

本次复盘识别出 **6 类共 18 项** 优化点，按严重程度分为：
- **🔴 高（4 项）**：可能导致数据竞争、资源泄漏或功能错误
- **🟡 中（8 项）**：设计缺陷或健壮性不足，特定条件下可能出问题
- **🟢 低（6 项）**：代码整洁、可维护性提升

---

## 2. 线程安全

### 🔴 2.1 CallbackHandler 数据竞争（高）

**问题**：`CallbackHandler` 的回报数据（`last_login_ans_` / `last_order_rtn_` / `last_cancel_rsp_` / `last_trade_rtn_`）由 API 内部回调线程写入，由 `TestCaseRunner::extract_response_fields()` 在主线程读取，**两者之间没有同步机制**。

**当前保护**：`cv_`/`mtx_` 仅保护 `response_received_` / `cancel_rsp_received_` 标志位。回调线程先写数据，后设标志；主线程等标志、读数据。**顺序上正确**，但无内存屏障保证。

**风险**：
- 编译器/CPU 可能重排回调线程的写操作（数据写 vs 标志写）
- 主线程看到标志为 true 时，数据可能尚未完全写入
- 在 ARM 或弱内存序架构上风险更大（当前 x86 下 TSO 模型相对安全，但非标准保证）

**建议**：
```cpp
// 在回调线程中：写数据后加内存屏障再设标志
std::lock_guard<std::mutex> lock(mtx_);
last_order_rtn_ = rtn;   // 写数据
response_received_ = true; // 设标志（在锁保护内，天然有 happens-before）
cv_.notify_one();
```
当前 `on_order_rtn()` 等回调在锁外写数据、锁内设标志——数据写在锁外，标志在锁内，**无 happens-before 关系**。修复：将数据写也移到锁内。

**修复方案**：`callback_handler.cpp` 中各回调方法将数据赋值移到 `lock_guard` 作用域内。

---

### 🟡 2.2 Logger::min_level_ 无锁读取（中）

**问题**：`Logger::log()` 在获取锁之前读取 `min_level_`：
```cpp
if (static_cast<int>(level) < static_cast<int>(min_level_)) return; // 无锁
std::lock_guard<std::mutex> lock(mutex_);
```
`set_level()` 在锁保护下写入。这是 deliberate optimization（避免锁竞争），但构成 data race。

**风险**：极低。`set_level()` 通常只在初始化时调用一次，之后 `min_level_` 只读。即使 race，后果最多是多打或少打一条日志，不会 crash。

**建议**：将 `min_level_` 声明为 `std::atomic<LogLevel>` 即可消除 data race，且仍保持无锁快速路径。

---

### 🟡 2.3 Logger::level() 无锁读取（中）

**问题**：`LogLevel level() const { return min_level_; }` 无锁读取，与 `set_level()` 的锁保护写入冲突。

**建议**：同上，改为 `std::atomic<LogLevel>`。

---

## 3. 错误处理与资源管理

### 🔴 3.1 load_api() 静默忽略 --lib 参数（高）

**问题**：`MockClient::load_api()` 是空操作（空函数体），`lib_path` 参数被完全忽略。用户在命令行指定 `--lib /path/to/liblbapi.so` 时，程序静默接受但不生效。

**影响**：用户以为在加载指定路径的动态库，实际用的是编译时链接的版本。如果指定路径与编译时链接的不同，测试结果可能与预期不符。

**建议**：
- 方案 A（推荐）：改为真正的 `dlopen` 动态加载，按 `lib_path` 加载指定 `.so`
- 方案 B（最小修复）：在 `load_api()` 中打印实际生效的库路径（可通过 `/proc/self/maps` 或 `dladdr` 获取），并在路径不同时输出 WARN 日志

---

### 🟡 3.2 run_test_plan() 忽略 perf 测试失败（中）

**问题**：`MockClient::run_test_plan()` 末尾调用 `run_perf_test(perf_node)` 但**忽略其返回值**。即使性能测试失败，函数仍返回 `true`。

**影响**：main.cpp 中 `ok = client.run_test_plan(...)` 在 perf 失败时仍为 true，进程退出码为 0（成功）。CI/CD 流水线可能误判。

**建议**：将 perf 返回值纳入判断：
```cpp
bool perf_ok = run_perf_test(perf_node);
return perf_ok;  // 或组合功能测试结果
```
注意：perf 失败可能是环境问题（FTE 过载），功能测试可能已通过。建议将功能测试与性能测试的成败分开报告，但 `run_test_plan` 的返回值应反映 perf 状态。

---

### 🟡 3.3 net_time_map_ 包含失败委托条目（中）

**问题**：`PerfRunner::run_benchmark()` 中，无论 `order_insert()` 是否成功，都将 `(client_seq_id, api_arrive_time_ns)` 加入 `net_time_map_`。失败的委托不会实际发出，因此网络上不会抓到对应包，导致映射文件中存在永远无法匹配的条目。

**影响**：`api_net_time_capture` 关联时会有 unmatched 条目，干扰统计（unmatched 数偏高）。

**建议**：
```cpp
int32_t ret = api_->order_insert(req);
if (ret == 0) {
    net_time_map_.emplace_back(client_seq_id, api_arrive_time_ns);
}
```
（`api_leave_time_ns` 仍可统计，仅映射条目按成功记录）

---

### 🟢 3.4 init_from_json() 配置缺失时异常未处理（低）

**问题**：`init_from_json()` 中直接通过 `config["api_instance_name"].as_string()` 等访问配置字段，若 JSON 中缺少必填字段，`JsonValue::operator[]` 抛异常，由外层 `try/catch` 捕获后返回 false。

**影响**：错误信息是笼统的 `初始化异常: JsonValue: key not found: xxx`，用户难以定位具体哪个配置字段缺失。

**建议**：在访问前用 `config.has("xxx")` 检查，对必填字段缺失输出具体错误信息：
```cpp
if (!config.has("api_instance_name")) {
    LOG_ERROR("[MockClient] 配置缺少必填字段: api_instance_name");
    return false;
}
```

---

## 4. 设计问题

### 🟡 4.1 旧模式 load_single_case() 的 validate 数组格式（中）

**问题**：`TestCaseRunner::load_single_case()` 中兼容旧格式 `validate` 数组（lines 80-89）：
```cpp
JsonValue validate = exp["validate"];
for (size_t i = 0; i < validate.size(); i++) {
    std::string field_name = validate[i].as_string();
    FieldMatch fm;
    fm.field_name = field_name;
    fm.expected_value = fields[field_name];  // 若 fields 缺失则抛异常
    fm.required = true;
    test_case.expected_fields.push_back(fm);
}
```
如果 `validate` 是数组但 `fields` 缺失/非对象，`fields[field_name]` 会抛异常。此外，`validate` 格式已被 `null` 字段标记取代（`cases/` 新格式），此分支实际已成为**死代码**。

**建议**：移除 `validate` 数组兼容分支（约 10 行）。所有预期文件已统一使用 `null` 字段标记动态字段。

---

### 🟡 4.2 execute_all() 不重置 handler 状态（中）

**问题**：`execute_all()` 按顺序执行多个测试用例。`send_request()` 在每个用例开始时调用 `handler_->reset()`，但 `reset()` **不清除** `last_error_desc_` 和 `last_link_status_`。

**影响**：如果前一个用例触发了 `on_error` 回调，`last_error_desc_` 会残留到下一个用例。虽然当前用例不直接读取该字段，但若未来扩展了相关校验，可能误判。

**建议**：在 `CallbackHandler::reset()` 中增加 `last_error_desc_.clear()` 和 `last_link_status_ = false` 的清理（后者需谨慎——`last_link_status_` 可能用于跨用例的心跳监控）。

---

### 🟢 4.3 TestCaseType::None 与 Unknown 语义重叠（低）

**问题**：`TestCaseType` 枚举中 `None` 和 `Unknown` 都表示"无类型"，但语义不同：
- `None`：故意无请求（如异步 TradeRtn）或无预期字段
- `Unknown`：`parse_type()` 遇到不认识的类型字符串

**影响**：代码中 `response_type == TestCaseType::None` 用于判断是否跳过校验，与 `Unknown` 混用可能导致意外跳过。

**建议**：为 `Unknown` 添加日志警告，或将其视为错误：
```cpp
case TestCaseType::Unknown:
    LOG_WARN("[Loader] 未知的测试类型字符串，跳过校验");
    // fall through to None
```

---

### 🟢 4.4 perf 委托模板文件格式不一致（低）

**问题**：`run_test_plan` 中加载 perf order_file 时，支持两种格式：
```cpp
JsonValue order = order_root.has("order") ? order_root["order"] : order_root;
```
即 `{"order": {...}}` 或直接 `{...}`。而 `connection_config_gw_single.json` 中 perf 的 order 是内联对象（无外层 `order` 键）。两种格式并存增加了理解成本。

**建议**：统一为 `{"order": {...}}` 格式（当前 `perf_order_request.json` 已采用），并在文档中明确说明。

---

## 5. 性能优化

### 🟢 5.1 perf_runner 中 interval 整数截断（低）

**问题**：`PerfRunner::run_benchmark()` 中：
```cpp
int64_t interval_ns = 1000000000 / cfg_.tps;
```
整数除法截断。对于常见 TPS（8000/10000）精确整除无误差，但对于非整除 TPS（如 3333），`interval_ns = 300030`，理论值应为 `300030.003...`，10 秒累计误差约 100ns。

**影响**：可忽略。100ns 在 10 秒量级上误差可忽略不计。

**建议**：如需精确，使用 `double` 累积：
```cpp
double next_time = 0;
// 循环内：
next_time += 1.0e9 / cfg_.tps;
int64_t target_ns = static_cast<int64_t>(next_time);
```

---

### 🟢 5.2 多余的头文件包含（低）

多个源文件在替换为 `LOG_*` 宏后不再使用 `std::cout`/`std::cerr`，但仍保留 `#include <iostream>`：

| 文件 | 可移除 |
|---|---|
| `test_case_runner.cpp` | `<iostream>` |
| `callback_handler.cpp` | `<iostream>` |
| `test_report.cpp` | `<iostream>` |
| `perf_runner.cpp` | `<iostream>` |
| `mock_client.cpp` | `<iostream>` |

移除可略微减少预处理时间，但影响极小。

---

## 6. 代码整洁度

### 🟢 6.1 init_from_json() 中 set_attr 重复模式（低）

**问题**：`init_from_json()` 中有 15+ 组重复的 `set_attr` 调用模式：
```cpp
ret_attr = cfg->set_attr("key", value);
if (ret_attr) LOG_DEBUG("set_attr(key)=" << ret_attr);
```

**建议**：提取辅助宏或 lambda：
```cpp
auto set_cfg = [&](const char* key, auto val) -> bool {
    int32_t r = cfg->set_attr(key, val);
    if (r) LOG_DEBUG("set_attr(" << key << ")=" << r);
    return r == 0;
};
set_cfg("api_instance_name", config["api_instance_name"].as_string().c_str());
set_cfg("market_type", static_cast<int8_t>(config["market_type"].as_int()));
// ...
```
减少约 40 行重复代码。

---

### 🟢 6.2 execute() 中 timeout_ms 重复计算（低）

**问题**：`TestCaseRunner::execute()` 中 `tc.timeout_ms > 0 ? tc.timeout_ms : 5000` 出现了 3 次（TradeRtn 分支、OrderCancel 分支、wait_for_response 分支）。

**建议**：在函数开头提取局部变量：
```cpp
int64_t effective_timeout = tc.timeout_ms > 0 ? tc.timeout_ms : 5000;
```

---

### 🟢 6.3 trim_fixed() 中 std::string 构造可读性（低）

**问题**：`trim_fixed()` 中 `std::string(" \\0", 2)` 构造一个含空格和 NUL 的字符串用于 `find_last_not_of`。写法正确但容易误解——乍看像 C 字符串 `" \\0"`（空格+反斜杠+零）。

**建议**：使用更清晰的写法：
```cpp
static const std::string kTrimChars(" \0", 2);
```
或直接：
```cpp
size_t e = s.find_last_not_of(" \0");  // 依赖 C 字符串含隐式 \0
```
但后者在 `find_last_not_of` 中遇到嵌入 NUL 时行为可能不符合预期。当前写法是最安全的，建议保留并加注释。

---

## 7. 测试覆盖与功能缺口

### 🔴 7.1 无断线重连与登录重试机制（高）

**问题**：当前 mock_client 在以下场景中不做重试：
- `api_->start()` 失败 → 直接返回 false，整个测试终止
- `api_->login()` 失败 → 用例 FAIL，后续场景继续执行（但依赖登录的委托/撤单场景必然失败）
- FTE 运行中断链 → `on_link_status` 回调记录断链，但无重连逻辑

**影响**：在 FTE 环境不稳定时（常见于开发调试），测试可能大面积失败，难以区分是环境问题还是业务逻辑问题。

**建议**：
- 添加 `--retry-login <N>` 参数，登录失败时重试 N 次（间隔 1s）
- 添加 `--wait-ready <seconds>` 参数，`start()` 后等待链接就绪再执行测试
- 断链后自动重连（可选，复杂度较高，可作为独立 feature）

---

### 🟡 7.2 无链接就绪等待机制（中）

**问题**：`api_->start()` 返回成功仅表示 API 实例启动，不保证与 FTE 的 TCP 链接已建立。实际建链是异步的，由 `on_link_status` 回调通知。当前代码在 `start()` 后立即执行测试，可能因链接未就绪而失败。

**建议**：在 `init_from_json()` 末尾或 `run_test_plan()` 开始时，等待 `on_link_status` 回调确认链接就绪：
```cpp
// 等待链接就绪（最多 wait_seconds 秒）
for (int i = 0; i < wait_seconds; i++) {
    if (callback_->last_link_status()) break;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
if (!callback_->last_link_status()) {
    LOG_WARN("[MockClient] 链接未就绪，继续执行（可能失败）");
}
```

---

### 🟡 7.3 无 ETF 委托测试场景（中）

**问题**：`TestCaseType::EtfOrderInsert` 已定义，`parse_type()` 也支持 `"etf_order_insert"` 字符串映射，但 `send_request()` 的 `switch` 中**没有** `EtfOrderInsert` 分支，会落到 `default` 报错。

**影响**：如果测试配置使用了 `"etf_order_insert"` 类型，会得到"不支持请求类型"错误。

**建议**：要么实现 ETF 委托发送逻辑，要么在 `parse_type()` 中将 `"etf_order_insert"` 映射到 `OrderInsert`（共用相同的 OrderReq 结构），并在日志中提示。

---

### 🟡 7.4 无 98 柜台查询测试（中）

**问题**：当前 mock_client 仅测试 FTE（快速柜台）链路，未覆盖 98 柜台查询（资金查询/持仓查询/委托查询等）。根据 `api_dev_task.txt`，查询只走 98 柜台。

**建议**：添加查询测试场景支持（`QueryType` 枚举 + `query_fields` 配置 + `api_->query()` 调用 + 查询回报校验）。

---

## 8. 各文件逐项问题清单

### `src/mock_client.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 1 | 🔴 | `load_api()` 空操作，`lib_path` 被忽略 | 35-38 |
| 2 | 🟡 | `run_test_plan()` 忽略 perf 返回值 | 309 |
| 3 | 🟢 | `#include <iostream>` 多余 | 16 |
| 4 | 🟢 | `set_attr` 重复模式可提取 lambda | 71-117 |

### `src/test_case_runner.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 5 | 🟡 | `load_single_case()` validate 数组兼容分支是死代码 | 80-89 |
| 6 | 🟡 | `execute_all()` 不清理 handler 的 error_desc | 201-213 |
| 7 | 🟢 | `execute()` timeout_ms 重复计算 | 227/276/291 |
| 8 | 🟢 | `#include <iostream>` 多余 | 6 |

### `src/callback_handler.h/.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 9 | 🔴 | 回报数据在锁外写入，与读取线程无 happens-before | 各 on_* 方法 |
| 10 | 🟡 | `reset()` 不清除 `last_error_desc_` | reset() |
| 11 | 🟢 | `#include <iostream>` 多余 | callback_handler.cpp |

### `src/perf_runner.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 12 | 🟡 | `net_time_map_` 包含失败委托条目 | run_benchmark() |
| 13 | 🟢 | `#include <iostream>` 多余 | 5 |

### `src/logger.h/.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 14 | 🟡 | `min_level_` 在 `log()` 中无锁读取（data race） | logger.cpp:56 |
| 15 | 🟡 | `level()` getter 无锁 | logger.h:37 |

### `src/test_report.cpp`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 16 | 🟢 | `#include <iostream>` 多余 | 3 |

### `src/test_case_runner.h`
| # | 严重度 | 问题 | 行号 |
|---|---|---|---|
| 17 | 🟢 | `None` 与 `Unknown` 语义重叠 | 28-29 |

### 全项目
| # | 严重度 | 问题 | 备注 |
|---|---|---|---|
| 18 | 🔴 | 无登录重试与链接就绪等待 | 环境不稳定时大面积失败 |
| 19 | 🟡 | ETF 委托类型未实现 | `EtfOrderInsert` 无 switch 分支 |
| 20 | 🟡 | 无 98 柜台查询测试 | 查询链路未覆盖 |

---

## 9. 优先级建议

### 第一优先级（建议立即修复）
1. **🔴 CallbackHandler 数据竞争**（#9）—— 将数据赋值移到锁内，消除 data race
2. **🔴 load_api() 忽略 --lib**（#1）—— 至少打印警告或改为 dlopen
3. **🔴 无链接就绪等待**（#18）—— 登录前确认链接状态

### 第二优先级（建议近期修复）
4. **🟡 run_test_plan() 忽略 perf 失败**（#2）
5. **🟡 net_time_map_ 包含失败条目**（#12）
6. **🟡 Logger::min_level_ 无锁读取**（#14/#15）
7. **🟡 移除 validate 数组死代码**（#5）
8. **🟡 ETF 委托类型实现**（#19）

### 第三优先级（可后续优化）
9. **🟢 多余 iostream 包含**（#3/#8/#11/#13/#16）
10. **🟢 set_attr 重复模式提取 lambda**（#4）
11. **🟢 timeout_ms 重复计算**（#7）
12. **🟢 None/Unknown 语义清理**（#17）

---

> **说明**：本次复盘聚焦于代码质量、正确性与健壮性，未涉及业务功能扩展（如 98 柜台查询、更多协议支持等），后者属于产品规划范畴。