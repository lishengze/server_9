# SVN 提交分析报告（2026-10-19）

> 分析环境：docker `otc` 容器，SVN 工作副本 `/mnt/work/api_trunk/trunk`
> SVN 仓库：`https://jyjs2.svn.gtja.net/svn/NewAPI`，Revision 17
> 分析范围：`trunk/NewAPI/` 下所有 M/A/? 状态文件

---

## 一、总览

| 状态                            | 数量                 | 说明                       |
| ------------------------------- | -------------------- | -------------------------- |
| **M（已修改）**           | 63 个                | 已在 SVN 管理下，需提交    |
| **A（已 add）**           | 19 个                | 已执行 `svn add`，需提交 |
| **?（未版本化，需 add）** | 11 个路径（含目录）  | 需逐一判断是否提交         |
| **总计需提交**            | **~90 个文件** | 含目录展开                 |

---

## 二、M 状态文件（已修改，需提交）

### 2.1 Common 层（GCC 4.8.5 兼容，3 文件）

| 文件                                | 修改点                                                                            | diff 行数 |
| ----------------------------------- | --------------------------------------------------------------------------------- | --------- |
| `NewAPI/common/include/mlog.h`    | ① 新增 `#include <string>` ② 新增 `operator<<(const std::string&)` 流式输出 | 10 行     |
| `NewAPI/common/include/mthread.h` | `<stdatomic.h>` → `<atomic>`（GCC 4.8.5 兼容，C++11 标准）                   | 4 行      |
| `NewAPI/common/include/mutils.h`  | `std::is_integral_v<T>` → `std::is_integral<T>::value`（GCC 4.8.5 兼容）     | 4 行      |

### 2.2 核心 API 源码（24 文件）

| 文件                             | 修改点                                                                                                                                                                                                                                                         | diff 行数 |
| -------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------- |
| `src/aio_socket_link.h`        | 新增 `reset_remote()` 方法（支持 fpga Core 链接地址覆盖）                                                                                                                                                                                                    | 18        |
| `src/api_config.cpp`           | `known_attrs` 注册表新增 `single_cust_per_link`(bool)、`counter98_mode`(int32)、`counter98_report_sync`(map)；`copy_attrs` 新增 bool/map 复制块                                                                                                      | 43        |
| `src/api_config_impl.h`        | 新增 `get_single_cust_per_link()`/`get_counter98_mode()`/`get_counter98_report_sync()` getter + 对应成员变量                                                                                                                                             | 5         |
| `src/api_event_msg.h`          | 事件消息结构修改（关联 stop/close 分离）                                                                                                                                                                                                                       | 142       |
| `src/api_instance.cpp`         | **stop/close 分离**：`stop()` 轻量（停引擎+回调，保留 log）+ `close()` 完整销毁（stop + 关 log）；析构调 `close()`                                                                                                                                 | 39        |
| `src/api_instance.h`           | 新增 `close()` 声明；`~api_impl()` 改为调 `close()`                                                                                                                                                                                                      | 6         |
| `src/callback_manager.cpp`     | 新增 `on_logout`/`on_trade_counter` 实现（direct 直调 + queued 入队）；`stop()`/`close()` 分离                                                                                                                                                         | 87        |
| `src/callback_manager.h`       | `cb_event_type` 新增 `logout=16`/`trade_counter=17`；新增 `close()`、`on_logout`、`on_trade_counter`                                                                                                                                               | 19        |
| `src/counter98.cpp`            | **整文件重写（冲突解决）**：采用线上 .r17 完整实现（2602 行），含完整 98 协议登录/委托/查询/回报处理                                                                                                                                                     | 535       |
| `src/counter98.h`              | **整文件重写（冲突解决）**：采用线上 .r17 完整声明（359 行），含全部查询方法                                                                                                                                                                             | 34        |
| `src/fpga_counter_base.cpp`    | FPGA 基类修改（关联 stop/close 分离 + 字段截断修复）                                                                                                                                                                                                           | 41        |
| `src/fpga_counter_base.h`      | FPGA 基类声明修改                                                                                                                                                                                                                                              | 11        |
| `src/fpga_counter_direct.cpp`  | FPGA direct 模式修改（stop/close 分离 + 双链路架构）                                                                                                                                                                                                           | 63        |
| `src/fpga_counter_direct.h`    | FPGA direct 声明修改                                                                                                                                                                                                                                           | 4         |
| `src/fpga_counter_gateway.cpp` | FPGA gateway 模式修改（stop/close 分离 + 断链重登）                                                                                                                                                                                                            | 16        |
| `src/gw_counter_direct.cpp`    | ① **撤单优化**：`order_sys_no` 直接返回 FTE clordno，移除 `record_order_locator`；`build_cancel_msg` 字段顺序修正 ② **GwLoginCache 替换**：单客户 `local_session_` / 多客户 `login_cache_` ③ **日志降噪**：高频回报降级为 debug | 253       |
| `src/gw_counter_direct.h`      | 新增 `GwLoginCache`/`GwLoginInfo`/`local_session_`/`fa_key_cache_`/`single_cust_per_link_` 成员 + `get_session_for_order()` 方法                                                                                                                   | 17        |
| `src/gw_session_cache.h`       | **移除全局锁**：`create_session`/`fill_session_from_ans`/`get_session`/`record_order_locator`/`get_clordno`/`get_orig_client_seq_id` 中删除 `lock_guard`（适配实例缓存模式）                                                               | 10        |
| `src/idle_engine.h`            | 新增 `void close(){}` no-op（兼容 stop/close 双接口）                                                                                                                                                                                                        | 4         |
| `src/link_timer_op.h`          | 定时器操作修改                                                                                                                                                                                                                                                 | 3         |
| `src/multi_socket_engine.cpp`  | **stop/close 分离**：`stop()` 轻量（close_ch + epoll_th_.join()），`close()` 完整（+timer_.close() + send_queue_.close()）                                                                                                                           | 115       |
| `src/multi_socket_engine.h`    | `stop()`/`close()` 声明                                                                                                                                                                                                                                    | 12        |
| `src/single_socket_engine.cpp` | **stop/close 分离**：同 multi_socket_engine 模式                                                                                                                                                                                                         | 63        |
| `src/single_socket_engine.h`   | `stop()`/`close()` 声明                                                                                                                                                                                                                                    | 4         |
| `src/tcpdirect_engine.cpp`     | **stop/close 分离**：同 multi_socket_engine 模式                                                                                                                                                                                                         | 35        |
| `src/tcpdirect_engine.h`       | `stop()`/`close()` 声明                                                                                                                                                                                                                                    | 4         |

### 2.3 API 头文件（4 文件）

| 文件                           | 修改点                                                                                                                               | diff 行数 |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ | --------- |
| `include/api_callback.h`     | 新增虚方法 `on_logout(const LogoutAns&)` + `on_trade_counter(counter_type, branch_id, fund_account_id, status)`                  | 14        |
| `include/api_config.h`       | 新增配置项常量 `single_cust_per_link`(bool) + `counter98_mode`(int32) + `counter98_report_sync`(map)                           | 4         |
| `include/gw_head.h`          | 字节序注释修正（大端→主机字节序，HostToNetwork no-op）+ encode/decode 注释同步                                                      | 13        |
| `include/order_trade_type.h` | 新增 `LogoutAns` 结构体（client_req_no/cust_id/fund_account_id/account_id/branch_id/err_code/err_msg）；新增 `#include <string>` | 15        |

### 2.4 设计文档（9 文件）

| 文件                           | 修改点                                                                                                                                    | diff 行数 |
| ------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------- | --------- |
| `doc/ARCHITECTURE.md`        | ① gw_counter_direct 缓存模型更新（GwSessionCache→GwLoginCache 实例缓存 + 撤单优化）② 引擎 stop/close 分离 ③ idle_engine close() no-op | 10        |
| `doc/api_callback.md`        | 新增 `on_logout`/`on_trade_counter` 说明                                                                                              | 10        |
| `doc/api_instance.md`        | stop()/close() 分离代码与说明                                                                                                             | 27        |
| `doc/callback_manager.md`    | stop/close 分离 + logout/trade_counter 事件                                                                                               | 19        |
| `doc/counter98.md`           | build_*_msg 已实现（非留空）+ C98SessionCache 章节 + 登录流程更新                                                                         | 46        |
| `doc/gw_counter_direct.md`   | GwLoginCache 章节 + 撤单优化 + 关键成员 + 待办                                                                                            | 37        |
| `doc/multi_socket_engine.md` | stop()/close() 接口表                                                                                                                     | 5         |
| `doc/order_trade_type.md`    | LogoutAns 结构说明                                                                                                                        | 16        |
| `doc/tcpdirect_engine.md`    | stop()/close() 接口表                                                                                                                     | 5         |

### 2.5 Mock 测试（15 文件）

| 文件                                             | 修改点                               | diff 行数 |
| ------------------------------------------------ | ------------------------------------ | --------- |
| `CMakeLists.txt`                               | 编译配置修改                         | 9         |
| `mock/98_counter/config/server_config.json`    | 98 mock 配置调整                     | 4         |
| `mock/client/CMakeLists.txt`                   | 编译配置（新增源文件）               | 26        |
| `mock/client/config/connection_config.json`    | 连接配置更新                         | 11        |
| `mock/client/config/test_cases/fte_combo.json` | FTE 组合测试用例                     | 25        |
| `mock/client/src/callback_handler.cpp`         | 回调处理逻辑更新                     | 34        |
| `mock/client/src/callback_handler.h`           | 回调处理头更新                       | 5         |
| `mock/client/src/main.cpp`                     | 主程序更新（支持多配置切换）         | 195       |
| `mock/client/src/metric_stats.cpp`             | 指标统计（新增 P95）                 | 4         |
| `mock/client/src/metric_stats.h`               | 指标统计头                           | 5         |
| `mock/client/src/mock_client.cpp`              | mock 客户端（多柜台支持 + 性能测试） | 236       |
| `mock/client/src/mock_client.h`                | mock 客户端头                        | 23        |
| `mock/client/src/perf_runner.cpp`              | 性能测试框架                         | 54        |
| `mock/client/src/perf_runner.h`                | 性能测试头                           | 17        |
| `mock/client/src/test_case_runner.cpp`         | 测试用例（新增查询类型 + None）      | 204       |
| `mock/client/src/test_case_runner.h`           | 测试用例头                           | 15        |
| `mock/client/src/test_report.cpp`              | 测试报告                             | 29        |
| `mock/client/src/test_report.h`                | 测试报告头                           | 5         |

---

## 三、A 状态文件（已 add，需提交）

### 3.1 API 头文件（3 文件）

| 文件                        | 说明                   |
| --------------------------- | ---------------------- |
| `include/common_struct.h` | 通用结构体定义（新增） |
| `include/struct_ans.h`    | 应答结构体定义（新增） |
| `include/struct_req.h`    | 请求结构体定义（新增） |

### 3.2 Mock 配置（8 文件）

| 文件                                                    | 说明                                                    |
| ------------------------------------------------------- | ------------------------------------------------------- |
| `mock/98_counter/config/server_config_9003.json`      | 98 mock 端口 9003 配置                                  |
| `mock/client/config/connection_config_fte_low.json`   | FTE 低负载连接配置                                      |
| `mock/client/config/connection_config_gone.json`      | GOne 连接配置（fast_counter_type=2, speed_link_type=1） |
| `mock/client/config/connection_config_gw_multi.json`  | GW 多客户连接配置                                       |
| `mock/client/config/connection_config_gw_single.json` | GW 单客户连接配置                                       |
| `mock/client/config/test_cases/gone_combo.json`       | GOne 组合测试用例                                       |
| `mock/client/config/test_cases/gone_login.json`       | GOne 登录测试用例                                       |
| `mock/client/config/test_plan_gone.json`              | GOne 测试计划                                           |
| `mock/client/config/test_plan_gw.json`                | GW 测试计划                                             |

### 3.3 Mock 源码（5 文件）

| 文件                                     | 说明                     |
| ---------------------------------------- | ------------------------ |
| `mock/client/api_net_time_capture.cpp` | 网络时间捕获实现（新增） |
| `mock/client/api_net_time_common.h`    | 网络时间公共头（新增）   |
| `mock/client/src/logger.cpp`           | 日志模块实现（新增）     |
| `mock/client/src/logger.h`             | 日志模块头（新增）       |
| `mock/client/src/result_analysis.cpp`  | 结果分析模块实现（新增） |
| `mock/client/src/result_analysis.h`    | 结果分析模块头（新增）   |

### 3.4 Mock 脚本（1 文件）

| 文件                                | 说明                     |
| ----------------------------------- | ------------------------ |
| `mock/client/run_perf_compare.sh` | FTE vs GOne 顺序对比脚本 |

---

## 四、? 状态文件（未版本化，需判断是否 add）

### 4.1 建议提交（源码/文档类）

| 文件/目录                                    | 说明                                                                                                                  | 操作        |
| -------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- | ----------- |
| `98_counter.md`                            | 98 柜台深入分析文档（含 UML 时序图/类图/流程图）                                                                      | `svn add` |
| `gone_counter.md`                          | FPGA 柜台深入分析文档（含 base/direct/gateway 完整分析）                                                              | `svn add` |
| `gw_counter.md`                            | GW 柜台深入分析文档（含撤单优化/GwLoginCache）                                                                        | `svn add` |
| `src/c98_session_cache.h`                  | C98 会话缓存实现（实例缓存，非全局单例）                                                                              | `svn add` |
| `src/gw_login_cache.h`                     | GW 登录缓存实现（GwLoginCache 实例缓存）                                                                              | `svn add` |
| `mock/client/config/cases/`                | 测试用例（10 个 json：login/order/cancel/heartbeat/perf 等）                                                          | `svn add` |
| `mock/client/config/cases_gone/`           | GOne 测试用例（3 个 json：cancel/order/trade expected）                                                               | `svn add` |
| `mock/doc/`                                | mock 设计文档（5 个 md：api_net_time_design/api_test_manual/mock_client_design/mock_client_test/mock_client_upgrade） | `svn add` |
| `mock/gone_counter/`（不含 build 和 .log） | GOne 模拟柜台源码（src/ + config/ + CMakeLists.txt）                                                                  | `svn add` |

### 4.2 不建议提交（构建产物/日志/分析结果）

| 文件/目录                                   | 说明                              | 理由               |
| ------------------------------------------- | --------------------------------- | ------------------ |
| `mock/98_counter/build/`                  | 构建产物（bin/CMakeCache/obj 等） | 可自动生成         |
| `mock/client/build/`                      | 构建产物（bin/CMakeCache/obj 等） | 可自动生成         |
| `mock/client/api_log/`                    | 运行时日志（fte_*.log）           | 运行时生成         |
| `mock/client/fte_*.log`                   | FTE 运行日志                      | 运行时生成         |
| `mock/client/gone_*.log`                  | GOne 运行日志                     | 运行时生成         |
| `mock/client/result/`                     | 测试结果分析（57 个 txt 文件）    | 非源码，可重跑生成 |
| `mock/gone_counter/gone_counter_mock.log` | GOne mock 运行日志                | 运行时生成         |
| `mock/gone_counter/build/`                | 构建产物                          | 可自动生成         |

### 4.3 需 add 的新增文件清单（建议执行）

```bash
# 三个 counter 分析文档
svn add 98_counter.md gone_counter.md gw_counter.md

# 缓存类源码
svn add src/c98_session_cache.h src/gw_login_cache.h

# 测试用例
svn add mock/client/config/cases/ mock/client/config/cases_gone/

# mock 设计文档
svn add mock/doc/

# GOne 模拟柜台源码（排除 build 和 .log）
svn add --depth empty mock/gone_counter/
svn add mock/gone_counter/CMakeLists.txt mock/gone_counter/config/ mock/gone_counter/src/
```

---

## 五、修改点分类汇总

### 5.1 按功能分类

| 功能分类                  | 文件数 | 核心修改                                                                                               |
| ------------------------- | ------ | ------------------------------------------------------------------------------------------------------ |
| **stop/close 分离** | 12     | api_instance、callback_manager、4 个引擎（multi/single/tcpdirect/idle）的 stop()/close() 双接口        |
| **回调事件扩展**    | 5      | cb_event_type 新增 logout(16)/trade_counter(17)、LogoutAns 结构、on_logout/on_trade_counter 虚方法     |
| **账户缓存实例化**  | 7      | GwLoginCache/C98SessionCache 替代 GwSessionCache 全局单例；单客户 local_session_ / 多客户 login_cache_ |
| **撤单优化**        | 3      | order_sys_no 直接返回 FTE clordno；消除 order_locators 缓存映射；build_cancel_msg 字段顺序修正         |
| **GCC 4.8.5 兼容**  | 3      | is_integral_v→is_integral<T></t>::value、stdatomic.h→atomic、新增 operator<<(string)                 |
| **FPGA 双链路**     | 4      | aio_socket_link reset_remote()、fpga 字段截断修复、双链路架构                                          |
| **冲突解决**        | 2      | counter98.cpp/h 采用线上 .r17 完整实现（2602 行）                                                      |
| **设计文档**        | 9      | 8 个模块文档 + ARCHITECTURE.md 同步当前代码                                                            |
| **Mock 测试**       | 18     | 多柜台支持、性能测试框架、GOne/GW 配置与测试用例                                                       |
| **新增源码**        | 11     | 3 个 counter 分析文档 + 2 个缓存类 + 6 个 mock 模块 + 模拟柜台                                         |

### 5.2 按风险等级

| 风险                         | 文件                                                                               | 说明                           |
| ---------------------------- | ---------------------------------------------------------------------------------- | ------------------------------ |
| **高（核心逻辑变更）** | counter98.cpp/h、gw_counter_direct.cpp/h、gw_session_cache.h                       | 冲突解决 + 撤单优化 + 缓存重构 |
| **中（接口扩展）**     | api_instance.cpp/h、callback_manager.cpp/h、api_callback.h、order_trade_type.h     | stop/close 分离 + 回调扩展     |
| **低（兼容性/文档）**  | common 层 3 文件、doc 9 文件、mock 18 文件                                         | GCC 兼容 + 文档同步 + 测试用例 |
| **新增（未上线验证）** | 98_counter.md/gone_counter.md/gw_counter.md、c98_session_cache.h、gw_login_cache.h | 新文件，需编译验证             |

---

## 六、SVN 提交建议

### 6.1 提交策略

建议**分批提交**，按风险等级从低到高：

**第一批（低风险，文档+兼容+新增）**：

```bash
# 文档
svn add 98_counter.md gone_counter.md gw_counter.md
svn add mock/doc/
# 缓存类
svn add src/c98_session_cache.h src/gw_login_cache.h
# 测试用例
svn add mock/client/config/cases/ mock/client/config/cases_gone/
# GOne 模拟柜台
svn add --depth empty mock/gone_counter/
svn add mock/gone_counter/CMakeLists.txt mock/gone_counter/config/ mock/gone_counter/src/
# Common 层兼容
# (M 状态自动包含)
svn commit -m "feat: GCC 4.8.5 兼容 + 新增缓存类 + counter 分析文档 + mock 测试用例"
```

**第二批（中风险，接口扩展）**：

```bash
svn commit -m "feat: stop/close 分离 + 回调事件扩展(logout/trade_counter)"
```

**第三批（高风险，核心逻辑）**：

```bash
svn commit -m "feat: counter98 冲突解决(采用线上r17) + 撤单优化 + GwLoginCache 实例缓存"
```

### 6.2 提交前检查清单

- [ ] `LANG=zh_CN.utf8 svn status` 确认无残留冲突标记（`C` 状态）
- [ ] `svn resolved` 清理已解决的冲突文件
- [ ] 删除 `.mine`、`.r17`、`.r4`、`.r5`、`.r8` 等冲突临时文件
- [ ] 确认 `build/`、`*.log`、`result/` 等非源码目录未被 add
- [ ] `svn diff` 预览提交内容
- [ ] 在 docker otc 内 CMake 编译验证通过后再提交

### 6.3 注意事项

1. **编码问题**：`svn status` 需 `LANG=zh_CN.utf8`，否则中文文件名（`ai-分析-规划-建立规则.txt`）触发 `E000022 UTF-8 转换错误`
2. **build 目录**：`mock/98_counter/build/`、`mock/client/build/`、`mock/gone_counter/build/` 不应提交，建议在 `.gitignore` 中忽略（但 SVN 无 .gitignore 机制，需手动排除）
3. **日志文件**：`api_log/`、`*.log` 文件不应提交，提交前确认 `svn status` 中无意外包含
4. **result 目录**：57 个测试结果分析文件非源码，不建议提交；若需保留历史，可归档到独立目录
