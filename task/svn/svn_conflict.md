# SVN 冲突分析报告

> 生成时间：2026-10-16
> 环境：docker `otc` 容器内，SVN 工作副本 `/mnt/work/api_trunk/trunk`（与宿主机 `/home/lsz/code/work/api_trunk/trunk` 共享同一目录）
> SVN 仓库：`https://jyjs2.svn.gtja.net/svn/NewAPI`，当前工作副本 revision 17，Last Changed Author: `zouyi024572`
> 冲突来源：本地 API 开发分支 与 线上 SVN（r17）合并时产生的 6 处文本冲突

---

## 一、冲突总览

执行 `svn status`（需 `LANG=zh_CN.utf8`，否则中文文件名 `ai-分析-规划-建立规则.txt` 触发 UTF-8 转换错误）后，共发现 **6 处文本冲突**，全部位于 `trunk/NewAPI/gone/api/` 下：

| # | 冲突文件 | 冲突行 | 本地版本(.mine) | 线上版本(.r17) | 冲突类型 |
|---|---------|--------|----------------|----------------|---------|
| 1 | `include/api_config.h` | 82 | `single_cust_per_link` | `counter98_mode` + `counter98_report_sync` | 配置项新增 |
| 2 | `mock/client/src/test_case_runner.h` | 28 | `None` 枚举 | 10 个查询类型枚举 | 枚举新增 |
| 3 | `src/api_config.cpp` | 43, 581 | `single_cust_per_link`（bool） | `counter98_mode`(int32) + `counter98_report_sync`(map) | 配置注册表新增 |
| 4 | `src/api_config_impl.h` | 115, 158 | `single_cust_per_link_` | `counter98_mode_` + `counter98_report_sync_` | 配置实现新增 |
| 5 | `src/counter98.cpp` | 1（整文件） | 959 行骨架实现 | 2602 行完整实现 | 整文件重写 |
| 6 | `src/counter98.h` | 1（整文件） | 238 行骨架声明 | 359 行完整声明 | 整文件重写 |

**冲突方向**：`.mine` = 本地工作副本（当前 API 开发分支）；`.r17` = SVN 线上 r17 版本（`zouyi024572` 提交）。

---

## 二、逐文件详细分析

### 冲突 1：`include/api_config.h`（第 82 行）

**冲突内容**（`config_name` 命名空间内，两个版本的配置项常量声明）：

```cpp
<<<<<<< .mine
constexpr const char *single_cust_per_link =
    "single_cust_per_link"; // bool  true(单链接单客户)  非必须(false=单链接多客户)
=======
constexpr const char *counter98_mode = "counter98_mode";       // int32   0(默认登录模式) 非必须(1=快速登录模式,不处理回报同步)
constexpr const char *counter98_report_sync = "counter98_report_sync"; // map<uint8_t,int32_t>  空  非必须(默认登录模式下的回报同步分区/索引)
>>>>>>> .r17
```

**本质**：本地分支新增 `single_cust_per_link`（单链接单客户控制），线上 r17 新增 `counter98_mode`（98 柜台登录模式）与 `counter98_report_sync`（回报同步分区）。两者是**互不冲突的不同配置项**，仅因插入位置相同（都在 `log_output_dir` 之后）而触发冲突。

**证据**：
- `single_cust_per_link` 被 `gw_counter_direct.cpp`（43/48/55/64/76/85/531/572 行）和 `mock_client.cpp`（150-152 行）使用。
- `counter98_mode`/`counter98_report_sync` 被 `api_config.cpp`（153/260/328/429/536/597 行）和 `counter98.cpp`（1076/1077/2486-2532 行）使用。

**结论**：两者都必须保留，属于"同名位置的不同新增"。

---

### 冲突 2：`mock/client/src/test_case_runner.h`（第 28 行）

**冲突内容**（`TestCaseType` 枚举内）：

```cpp
<<<<<<< .mine
    None,       // 无请求（如仅等待异步回报）或无预期字段
=======
    // 查询类型
    OrderQuery,
    TradeQuery,
    FundQuery,
    PositionQuery,
    MaxOrderQtyQuery,
    TotalFundAssetQuery,
    FundMaxdrawQuery,
    CustMarginAvlQuery,
    AssetDebtsQuery,
    CreditMaxOrderQtyQuery,
>>>>>>> .r17
```

**本质**：本地新增 `None`（用于异步回报等待/无预期字段的测试场景），线上 r17 新增 10 个查询类型枚举（委托/成交/资金/持仓/最大可委托/总资产/可取资金/可用保证金/资产负债/两融最大可委托）。两者**互不冲突**，仅因都插在 `WaitHeartbeat` 之后、`Unknown` 之前。

**结论**：两者都必须保留。

---

### 冲突 3：`src/api_config.cpp`（第 43、581 行，两处）

**冲突 3a（第 43 行）**——`known_attrs` 静态注册表：

```cpp
<<<<<<< .mine
    {config_name::single_cust_per_link, attr_type::bool_val},
=======
    {config_name::counter98_mode, attr_type::int32_val},
    {config_name::counter98_report_sync, attr_type::map_val},
>>>>>>> .r17
```

**冲突 3b（第 581 行）**——`copy_attrs` 属性复制逻辑：

```cpp
<<<<<<< .mine
  // bool属性
  bool bool_val = false;
  const char *bool_attrs[] = {
      config_name::single_cust_per_link,
  };
  for (const char *name : bool_attrs) {
    if (src.get_attr(name, bool_val) == LBAPI_OK) {
      set_attr(name, bool_val);
    }
  }

=======
  // map属性
  std::map<uint8_t, int32_t> map_val;
  const char *map_attrs[] = {
      config_name::counter98_report_sync,
  };
  for (const char *name : map_attrs) {
    if (src.get_attr(name, map_val) == LBAPI_OK) {
```

**本质**：两处都是 `known_attrs` 注册表/`copy_attrs` 复制逻辑中，本地新增 bool 类配置项 `single_cust_per_link`，线上新增 int32 `counter98_mode` 与 map 类 `counter98_report_sync`。互不冲突，仅插入位置重叠。

**结论**：两者都必须保留（注意 map 复制逻辑需补全 `set_attr` 收尾，见整合方案）。

---

### 冲突 4：`src/api_config_impl.h`（第 115、158 行，两处）

**冲突 4a（第 115 行）**——getter 方法：

```cpp
<<<<<<< .mine
  /// 获取是否单链接单客户模式 (true=单客户, false=多客户)
  bool get_single_cust_per_link() const { return single_cust_per_link_; }
=======
  /// 获取 98 柜台登录模式 (0=默认登录模式, 1=快速登录模式)
  int32_t get_counter98_mode() const { return counter98_mode_; }
  /// 获取回报同步分区/索引配置
  const std::map<uint8_t, int32_t> &get_counter98_report_sync() const { return counter98_report_sync_; }
>>>>>>> .r17
```

**冲突 4b（第 158 行）**——成员变量：

```cpp
<<<<<<< .mine
  bool single_cust_per_link_;       ///< 单链接单客户模式(默认: true)
=======
  int32_t counter98_mode_;          ///< 98柜台登录模式(默认: 0=默认登录模式, 1=快速登录模式)
  std::map<uint8_t, int32_t> counter98_report_sync_; ///< 回报同步分区/索引配置(默认: 空)
>>>>>>> .r17
```

**本质**：与冲突 1、3 配套，本地新增 `single_cust_per_link_` 成员及 getter，线上新增 `counter98_mode_`/`counter98_report_sync_` 成员及 getter。互不冲突。

**结论**：两者都必须保留。

---

### 冲突 5：`src/counter98.cpp`（第 1 行，整文件重写）

**冲突内容**：整个文件被两个版本完全替换，冲突标记从第 1 行到第 3564 行。

- **.mine（本地，959 行）**：98 柜台"骨架"实现，使用 `c98msg_tmp.h`（临时协议头），`deal_send_error` 为 4 参数 `(char*, int32, int16 link_type, int32)`，`build_*_msg` 多为**留空实现**（注释明确"98 真实协议未知, 当前为留空实现"）。
- **.r17（线上，2602 行）**：98 柜台**完整正式实现**，使用 `counter98/lb_counter98_protocol.h` + `counter98/msg_map_agw_external.h`（正式协议库），`deal_send_error` 为 3 参数 `(char*, int32, int32)`，完整实现全部业务发送/解析/查询应答。

**关键证据（决定采用 .r17）**：
1. **调用签名匹配**：`multi_socket_engine.cpp:291` 调用 `counter98_->deal_send_error(data, data_len, ret)` 是 **3 参数**，与 .r17 的 3 参数签名**完全匹配**；.mine 的 4 参数签名会导致编译失败。
2. **接口完整性**：`api_instance.cpp` 调用了 `c98_.deal_max_order_qty_query / deal_total_fund_asset_query / deal_fund_maxdraw_query / deal_cust_margin_avl_query / deal_asset_debts_query / deal_credit_max_order_qty_query` 等 6 个查询方法，**只有 .r17 声明/实现了这些方法**（.mine 缺失）。
3. **依赖文件齐全**：`.r17` 依赖的 `counter98/lb_counter98_protocol.h`、`counter98/msg_map_agw_external.h`、`msg_map_agw_external.cpp` 均存在于 `NewAPI/gone/counter98/`，`gw_session_cache.h` 存在于 `gone/api/src/`。
4. **完整度**：.r17 是 2602 行完整实现（含全部查询应答 `*_ans` 方法），.mine 仅 959 行骨架。

**结论**：**采用 .r17 版本**（线上完整正式实现），丢弃本地骨架。

---

### 冲突 6：`src/counter98.h`（第 1 行，整文件重写）

**冲突内容**：与冲突 5 配套，整个头文件被两个版本完全替换（.mine 238 行 vs .r17 359 行）。

- **.mine（本地）**：声明使用 `c98msg_tmp.h`，`deal_send_error` 4 参数，`build_*_msg` 为 `void` 返回（留空）。
- **.r17（线上）**：声明使用 `counter98/lb_counter98_protocol.h` + `gw_session_cache.h`，`deal_send_error` 3 参数，`build_*_msg` 返回 `int32`（消息长度），完整声明全部查询方法及应答方法。

**关键证据**：
- `multi_socket_engine.h:110` 使用 `aio_socket_link<counter98>` 模板，需要 counter98 提供与 .r17 匹配的接口。
- .r17 声明的方法集与 `api_instance.cpp`/`multi_socket_engine.cpp` 的调用完全对应。

**结论**：**采用 .r17 版本**（与冲突 5 保持一致，保证声明与实现配套）。

---

## 三、合理整合方案

### 方案总原则

1. **互补新增型冲突（冲突 1-4）**：本地与线上的新增内容互不冲突，**全部保留**（merge 两方）。
2. **整文件重写型冲突（冲突 5-6）**：本地是骨架/临时实现，线上是完整正式实现，且线上接口与调用方签名完全匹配，**整体采用 .r17**。

### 具体操作步骤

#### 步骤 1：解决互补新增冲突（api_config 系列 + test_case_runner）

**1.1 `include/api_config.h`（冲突 1）**：冲突块内**同时保留**两方内容：

```cpp
constexpr const char *single_cust_per_link =
    "single_cust_per_link"; // bool  true(单链接单客户)  非必须(false=单链接多客户)
constexpr const char *counter98_mode = "counter98_mode";       // int32   0(默认登录模式) 非必须(1=快速登录模式,不处理回报同步)
constexpr const char *counter98_report_sync = "counter98_report_sync"; // map<uint8_t,int32_t>  空  非必须(默认登录模式下的回报同步分区/索引)
```

**1.2 `mock/client/src/test_case_runner.h`（冲突 2）**：冲突块内**同时保留**两方枚举：

```cpp
    None,       // 无请求（如仅等待异步回报）或无预期字段
    // 查询类型
    OrderQuery,
    TradeQuery,
    FundQuery,
    PositionQuery,
    MaxOrderQtyQuery,
    TotalFundAssetQuery,
    FundMaxdrawQuery,
    CustMarginAvlQuery,
    AssetDebtsQuery,
    CreditMaxOrderQtyQuery,
```

**1.3 `src/api_config.cpp`（冲突 3）**：
- 3a（第 43 行 known_attrs）：同时保留三行：
  ```cpp
  {config_name::single_cust_per_link, attr_type::bool_val},
  {config_name::counter98_mode, attr_type::int32_val},
  {config_name::counter98_report_sync, attr_type::map_val},
  ```
- 3b（第 581 行 copy_attrs）：同时保留 bool 复制块与 map 复制块，并确保 map 块收尾完整（`set_attr` + 循环闭合 `}`）。**注意**：map 复制块在冲突标记 `.r17` 侧被截断，需核对 `.r17` 完整代码补全 `set_attr(name, map_val)` 与收尾括号。

**1.4 `src/api_config_impl.h`（冲突 4）**：
- 4a（第 115 行 getter）：同时保留 `get_single_cust_per_link` + `get_counter98_mode` + `get_counter98_report_sync` 三个 getter。
- 4b（第 158 行成员）：同时保留 `single_cust_per_link_` + `counter98_mode_` + `counter98_report_sync_` 三个成员。
- **注意**：`api_config_impl.h` 需 `#include <map>`（.r17 侧已含）。

#### 步骤 2：解决整文件重写冲突（counter98）

**2.1 `src/counter98.cpp`（冲突 5）**：**整体采用 .r17**，即删除 `<<<<<<< .mine` 到 `=======` 的本地骨架部分（第 2-961 行），保留 `=======` 之后到 `>>>>>>>` 之前（第 963-3564 行）的 .r17 实现，并删除冲突标记行。

**2.2 `src/counter98.h`（冲突 6）**：**整体采用 .r17**，删除本地骨架部分（第 2-240 行），保留 .r17 声明（第 242-600 行），删除冲突标记行。

#### 步骤 3：冲突标记清理与验证

1. 用 `svn resolve --accept working <file>` 标记已解决（或手动编辑后 `svn resolved <file>`）。
2. 删除 `.mine`、`.r17`、`.r4`/`.r5`/`.r8` 临时文件。
3. **编译验证**（docker otc 容器内，CMake 重新编译）：
   - 确认 `api_config` 系列新增配置项能编译通过（bool/int32/map 三类注册与复制逻辑完整）。
   - 确认 counter98 采用 .r17 后，`api_instance.cpp`/`multi_socket_engine.cpp`/`single_socket_engine.cpp`/`tcpdirect_engine.cpp` 对 counter98 的调用签名匹配。
4. **功能验证**：复跑 mock_client 测试用例，确认 98 柜台相关功能正常。

#### 步骤 4：SVN 提交

```bash
# 在 docker otc 容器内 /mnt/work/api_trunk/trunk
svn resolve --accept working <6个冲突文件>   # 或 svn resolved
svn add <新文件：gw_head.h/gw_session_cache.h/gw_trade_msg.h 等>
svn commit -m "merge api dev branch with online r17: resolve conflicts in api_config/counter98"
```

---

## 四、风险与注意事项

1. **counter98 采用 .r17 会丢弃本地骨架的独有改动**：需确认本地骨架没有 .r17 缺失且被外部调用的独有接口。经核查，.r17 接口集**完全覆盖** `api_instance.cpp`/`multi_socket_engine.cpp` 的调用，且 .mine 的 4 参数 `deal_send_error` 与调用方（3 参数）不匹配，本地骨架本就无法编译，采用 .r17 是唯一正确选择。
2. **api_config.cpp copy_attrs 的 map 复制块被截断**：.r17 侧的 map 复制逻辑在冲突标记处不完整，需对照 `.r17` 文件（`api_config.cpp.r17`）补全 `set_attr` 调用与收尾括号，避免编译错误。
3. **编码问题**：`svn status` 需 `LANG=zh_CN.utf8`，否则 `NewAPI/common/ai-分析-规划-建立规则.txt` 中文文件名触发 `E000022 UTF-8 转换错误`，导致整个 status 失败。
4. **`.mine`/`.r17`/`.r4`/`.r5`/`.r8` 临时文件**：解决冲突后需删除，避免误提交到 SVN。
5. **提交前确认**：`svn status` 中仍有大量 `M`（已修改）与 `?`（未版本化）文件，需按需 `svn add` 新文件后再 `svn commit`，防止遗漏。

---

## 五、结论

- 6 处冲突中，**4 处（api_config.h / test_case_runner.h / api_config.cpp / api_config_impl.h）为互补新增**，应 merge 两方全部保留。
- **2 处（counter98.cpp / counter98.h）为整文件重写**，本地骨架缺失 6 个查询方法且 `deal_send_error` 签名与调用方不匹配，**必须整体采用线上 .r17 完整实现**。
- 解决后需在 docker otc 容器内 CMake 重编译验证，再 `svn resolve` + `svn commit` 提交。