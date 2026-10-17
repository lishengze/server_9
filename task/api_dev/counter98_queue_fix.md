# counter98 队列提交长度 Bug 修复 — 全面总结

> 日期：2026-10-17
> 触发场景：GCC 4.8.5 兼容性改造 + GOne 功能回归测试（测试 2/5 委托买入时 SIGSEGV 崩溃）
> 结论：counter98 发送队列存在"预留上限、提交上限、但 reader 按实际长度消费"的长度不一致 bug，导致引擎线程把预留空隙读成零长度事件，最终错位崩溃。

---

## 1. 问题背景

### 1.1 触发链

1. 项目从高版本 GCC 切换到 **GCC 4.8.5**（C++11 标准）进行编译（docker `otc` 容器）。
2. 完成 4 处 GCC 4.8.5 兼容性修复（详见 §4）后，编译通过。
3. 运行 GOne 功能测试计划 `test_plan_gone.json`：
   - 测试 1/5 GOne 登录测试 **通过**
   - 测试 2/5 GOne 委托买入测试 **SIGSEGV 崩溃**（exit 139）

### 1.2 崩溃现象

- gdb 定位崩溃点：`multi_socket_engine<fpga_counter_direct>::deal_event`（`multi_socket_engine.cpp:285`）
- 崩溃语句：`__atomic_store_n(evt->leave_time_ptr, perf_now_ns(), __ATOMIC_RELEASE)`
- 崩溃事件字段：
  - `link_type = 0`（LINK_TYPE_98）
  - `type = 0`（LINK_EVENT_TYPE_SEND_MSG）
  - `data_len = 0`（零长度事件）
  - `leave_time_ptr = 0x3e9`（垃圾指针，非法解引用导致崩溃）

---

## 2. 根因分析

### 2.1 直接根因

`counter98.cpp` 中所有 `deal_*_req` 函数采用"**预留上限、实际写入、提交上限**"模式：

```cpp
// 修复前（错误模式）
int32 take_len = lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN;  // 预留上限 = 16 + 2048 = 2064
int64 pos = take_req_que_mem(data, take_len);                      // 预留 take_len + sizeof
int32 msg_len = build_order_msg(req, data + sizeof(link_send_event)); // 实际写入 msg_len（如 157）
reinterpret_cast<link_send_event *>(data)->data_len = msg_len;      // 事件头 data_len = msg_len
cmt_req_que_mem(pos, take_len);                                     // 提交 take_len + sizeof  ← 错误
```

关键矛盾：

| 角色 | 长度口径 |
|------|---------|
| 写者（counter98）预留/提交 | `take_len + sizeof(link_send_event)` = 2064 + 16 |
| 读者（deal_event）消费 | `sizeof(link_send_event) + data_len` = 16 + msg_len（如 173） |

由于 `take_len(2064) >> msg_len(157)`，**写者提交的长度远大于读者消费的长度**，导致：
- 读者把每个事件**预留但未使用的空隙**（约 1900 字节）当成大量零长度事件读取（每 16 字节一个 `data_len=0` 的 SEND_MSG）。
- 空隙读完后，读者继续读到**下一个真实事件的中间**，把真实消息数据误读为 `link_send_event` 头，`leave_time_ptr` 字段被填充成垃圾值（0x3e9），最终解引用崩溃。

### 2.2 诊断证据

在 `deal_event` 加入诊断日志后，观察到：

```
EVT: pos=64   link_type=0 type=0 data_len=157 ltp=nil   ← 真实登录消息（正常）
EVT: pos=240  link_type=0 type=0 data_len=0   ltp=nil   ← 空隙（全零）
EVT: pos=256  link_type=0 type=0 data_len=0   ltp=nil   ← 空隙
... （大量 data_len=0 全零事件，每 16 字节一个）...
EVT: pos=2144 link_type=0 type=0 data_len=183 ltp=nil   ← 下一个真实事件（正常）
... （又大量 data_len=0 空隙）...
EVT: pos=4232 link_type=0 type=0 data_len=0   ltp=0x3e9 ← crash！
```

- 真实事件 pos=64（data_len=157，对齐后 176）→ 下一个真实事件 pos=2144。
- `2144 - 64 = 2080 = take_len + sizeof`，证明写者提交了 `take_len+sizeof`，而读者只消费了 `sizeof+157=173`（对齐 176），中间 1904 字节空隙被读成零长度事件。
- 最终 pos=4232 读到真实消息数据被误读，`leave_time_ptr` 变成 0x3e9 崩溃。

### 2.3 队列语义确认

`que_mth_buf` 是**零拷贝 SPSC 环形队列**，设计约束（`que_mth_buf.h` 注释）：
> "不存储数据长度，写入者保证每次写入的完整性"、"读者需要知道数据长度并提交匹配的长度"

即：**写者预留长度 == 写者提交长度 == 读者消费长度**，三者必须一致。counter98 的"预留上限、提交上限"违背了该约束。

而 `fpga_counter_direct` / `gw_counter_direct` 之所以没这个问题，是因为它们**在预留前就能确定精确长度**（消息是定长结构体，`take_len == msg_len`），预留/提交/消费完全一致。

---

## 3. 修复方案

### 3.1 原则

必须让"预留 == 提交 == 消费"三者一致。由于 counter98 的 98 协议消息是**变长**的（只有组包后才能确定 `msg_len`），无法在预留前获知精确长度，因此采用"**先组包到临时缓冲，再按精确长度预留、拷贝、提交**"的方案。

### 3.2 代码改动

**counter98.h**：新增 `send_req_msg()` 辅助函数 + 引入 `<cstring>`。

```cpp
FORCE_INLINE int32 send_req_msg(char *temp_buf, int32 msg_len) {
    char *data = nullptr;
    int64 pos = gw_send_queue_->write_get_mth(data, msg_len + sizeof(link_send_event)); // 精确预留
    if (unlikely(pos <= 0)) return static_cast<int32>(pos);
    link_send_event *evt = reinterpret_cast<link_send_event *>(data);
    evt->link_type = LINK_TYPE_98;
    evt->type = LINK_EVENT_TYPE_SEND_MSG;
    evt->data_len = msg_len;
    evt->leave_time_ptr = nullptr;  // 98 不参与 send 前时间戳记录
    if (msg_len > 0)
        memcpy(data + sizeof(link_send_event), temp_buf + sizeof(link_send_event), msg_len); // 精确拷贝
    gw_send_queue_->write_cmt_mth(pos, msg_len + sizeof(link_send_event)); // 精确提交
    gw_eng_op_->trigger_send();
    return 0;
}
```

**counter98.cpp**：全部 **20 个调用点**（委托/撤单/查询/登录/同步/重登）从"预留上限 + 提交上限"改为"临时缓冲组包 + `send_req_msg` 精确提交"。

```cpp
// 修复后（正确模式）
char temp_buf[lb_counter98::AGW_HEADER_SIZE + C98_MAX_MSG_LEN + sizeof(link_send_event)];
int32 msg_len = build_order_msg(req, temp_buf + sizeof(link_send_event)); // 组包到临时缓冲
if (msg_len <= 0) { ... return LBAPI_ERR_BUILD_MSG; }
int32 qret = send_req_msg(temp_buf, msg_len);                            // 精确预留+拷贝+提交
if (unlikely(qret < 0)) { ... return LBAPI_ERR_SEND_QUEUE_FULL; }
return LBAPI_OK;
```

改造的 20 个调用点：
- 委托类：`deal_order_req` / `deal_etf_order_req` / `deal_bse_order_req` / `deal_cancel_req`（4 个）
- 查询类：`deal_order_query` / `deal_order_batch_query` / `deal_trade_query` / `deal_trade_batch_query` / `deal_fund_query` / `deal_position_query` / `deal_max_order_qty_query` / `deal_total_fund_asset_query` / `deal_fund_maxdraw_query` / `deal_cust_margin_avl_query` / `deal_asset_debts_query` / `deal_credit_max_order_qty_query`（12 个）
- 登录/同步类：`deal_agw_login` / `send_report_sync` / `deal_login_req` / 重登（4 个）

### 3.3 为什么不采用"提交 msg_len + wrpos 回退"

曾尝试仅把 `cmt_req_que_mem(pos, take_len)` 改为 `cmt_req_que_mem(pos, msg_len)`（提交精确长度），但**无效**：
- `take_req_que_mem` 仍预留 `take_len+sizeof`，`wrpos` 已前移。
- 下一个事件从 `wrpos`（预留区末尾）开始，提交时 `wrcmt` 会**跨过预留空隙**跳到下一个事件末尾。
- 读者从空隙读到下一个事件，**空隙仍被读取**，问题依旧（且因 `wrcmt` 跳跃导致更复杂）。

因此必须**同时约束预留长度 == 提交长度**，即"临时缓冲组包 + 精确预留"，而非仅改提交长度。

---

## 4. 附带修复：GCC 4.8.5 兼容性

在排查过程中同时完成了 GCC 4.8.5（C++11）兼容性修复：

| 文件 | 问题 | 修复 |
|------|------|------|
| `trunk/NewAPI/common/include/mutils.h` | `std::is_integral_v<T>`（C++17） | `std::is_integral<T>::value` |
| `trunk/NewAPI/common/include/mthread.h` | `#include <stdatomic.h>`（C11） | `#include <atomic>` |
| `mock/client/src/callback_handler.cpp` | 缺 `#include <iostream>` | 补 `#include <iostream>` |
| `mock/client/src/test_case_runner.cpp` | 缺 `#include <iostream>` | 补 `#include <iostream>` |

---

## 5. 效果验证

### 5.1 GOne 功能测试（docker otc，GCC 4.8.5 编译）

| 测试用例 | 结果 |
|---------|------|
| 1/5 GOne 登录测试 | ✅ PASS |
| 2/5 GOne 委托买入测试 | ✅ PASS（64ms，修复前崩溃） |
| 3/5 GOne 成交回报校验 | ✅ PASS |
| 4/5 GOne 撤单测试 | ✅ PASS（20ms） |
| 5/5 GOne 心跳维持测试 | ✅ PASS（30s） |

**5/5 全部通过，无崩溃。**

### 5.2 编译验证

- `liblbapi.so`（lbapi）用 GCC 4.8.5 编译通过，无错误。
- `counter98.cpp` / `counter98.h` 无 LSP 语法错误（`api_config.h` include 报错为 IDE 路径问题，非真实编译错误）。

### 5.3 未验证项

- **FTE/GW 测试计划**（`test_plan_gw.json`）依赖 FTE 快速链路（33001），但 FTE 进程未运行、端口未监听，**环境不可用**，无法在 docker 中执行。
- counter98 修复已通过 GOne 测试中走 98 链路的登录用例充分验证（登录消息经 counter98 发送）。

---

## 6. 深入思考与经验沉淀

### 6.1 为什么这个 bug 之前没暴露

- 该 bug 是**潜伏性内存错位**，不影响每个槽位"第一个事件"的正确性（读者总能正确读到槽位起始的真实事件）。
- 空隙读出的零长度事件会触发 `send_msg(data, 0)` 发送空消息，但 mock 服务器对空消息通常**静默丢弃**，因此功能测试"看起来通过"。
- 只有当下一个真实事件的**数据恰好落在空隙读取窗口内**、且被误读为事件头时，才会触发非法指针崩溃。这在 GOne 委托买入（多消息并发）场景下被触发。

### 6.2 这类 bug 的通用排查方法

1. **崩溃点不是根因**：`__atomic_store_n(evt->leave_time_ptr, ...)` 崩溃只是表象，`leave_time_ptr` 是垃圾值说明**事件本身是错位的**。
2. **加诊断打印事件字段 + 原始字节**：打印每个事件的 `link_type/type/data_len/leave_time_ptr` 和 raw bytes，发现大量 `data_len=0` 全零事件。
3. **用位置关系反推**：真实事件 pos=64 → 下一个 pos=2144，间隔 2080 = `take_len+sizeof`，反推出写者按上限提交。
4. **对比无 bug 的同类实现**：`fpga_counter_direct` 是精确长度（定长消息），`take_len == msg_len`，无此问题，印证"预留==提交==消费"是铁律。

### 6.3 设计启示

- **零拷贝环形队列的写者必须保证"预留 == 提交 == 消费者口径"**，否则 reader 必然读到空隙。
- 变长消息不能"预留上限、提交上限"，必须**先组包确定实际长度，再精确预留**（牺牲一次 memcpy，换取队列一致性）。
- 若追求极致性能（避免 memcpy），可改为"预留上限 + 提交实际 + 回退 wrpos"，但需队列支持 wrpos 原子回退，且要处理对齐，复杂度高、风险大。

---

## 7. 修改文件清单

| 文件 | 类型 | 改动 |
|------|------|------|
| `trunk/NewAPI/gone/api/src/counter98.h` | 修复 | 新增 `send_req_msg()`、引入 `<cstring>` |
| `trunk/NewAPI/gone/api/src/counter98.cpp` | 修复 | 20 个调用点改为"临时缓冲组包 + 精确提交" |
| `trunk/NewAPI/common/include/mutils.h` | 兼容 | C++17 → C++11 |
| `trunk/NewAPI/common/include/mthread.h` | 兼容 | C11 → C++11 atomic |
| `mock/client/src/callback_handler.cpp` | 兼容 | 补 `#include <iostream>` |
| `mock/client/src/test_case_runner.cpp` | 兼容 | 补 `#include <iostream>` |

---

## 8. 相关文件

- 引擎消费端：`trunk/NewAPI/gone/api/src/multi_socket_engine.cpp`（`deal_event`，reader）
- 环形队列：`trunk/NewAPI/common/include/que_mth_buf.h` / `que_mth_buf.cpp`
- 事件结构：`trunk/NewAPI/gone/api/src/api_event_msg.h`（`link_send_event`）
- 知识库：`study/knowledge_base_system/NewAPI_知识库.md` §40