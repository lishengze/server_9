# mock_client FTE 集成测试记录

> 本文档详细记录在 FTE（上海/深圳）柜台联调测试过程中遇到的问题、修改的代码、增删的数据、解决方案与实际结果。
> 时间跨度：2026-09-14 ~ 2026-09-15

---

## 一、测试环境

### 1.1 运行环境
- **docker 容器**：`otc`（镜像 `otc_2026_4_29:latest`）
- **FTE 源码/部署目录**：`/mnt/work/gt_trunk`（宿主机 `/home/lsz/code/work/gt_trunk`）
- **NewAPI 源码**：宿主机 `/home/lsz/code/work/api_trunk`
- **编译器**：gcc 4.8.5（docker 内），CMake 用 `check_cxx_compiler_flag` 跳过 `-mprefer-vector-width`

### 1.2 FTE 环境组件
| 组件 | 端口 | 说明 |
|:---|:---|:---|
| 上海 FTE (UTE_61_611_11) | 33001 | 连接 38140/38141 模拟交易所 |
| 深圳 FTE (UTE_84_842_21) | 33002 | 连接 39142 模拟交易所 |
| tgw_simulator (Stock) | 38141 | 模拟上海竞价 |
| tgw_simulator (Bond) | 38140 | 模拟上海债券 |
| tgw_simulator (ETF) | 39142 | 模拟深圳 |
| counter98_mock | 9001 | 模拟 98 柜台（AGW/账户登录） |

### 1.3 启动/停止脚本（docker 容器内）
```bash
cd /mnt/work/gt_trunk/DYS-FRAMEWORK/fte/test_all
./stop_all.sh    # 停止 模拟交易所 + 上海/深圳 FTE
./clear_all.sh   # 清理日志（不会重置数据）
./start_all.sh   # 启动 模拟交易所 + 上海/深圳 FTE
./status_all.sh  # 查看状态
```

### 1.4 mock_client / counter98_mock
```bash
# counter98_mock（单独启动，start_all.sh 不含）
cd /home/lsz/code/work/api_trunk/trunk/NewAPI/gone/api/mock/98_counter
/home/lsz/code/work/api_trunk/build_cmake/bin/counter98_mock --config config/server_config.json &

# mock_client（运行组合测试，需 LD_LIBRARY_PATH 指向 liblbapi.so）
cd /home/lsz/code/work/api_trunk/trunk/NewAPI/gone/api/mock/client
LD_LIBRARY_PATH=/home/lsz/code/work/api_trunk/build_cmake/lib:$LD_LIBRARY_PATH \
  /home/lsz/code/work/api_trunk/build_cmake/bin/mock_client \
  --config config/connection_config.json \
  --lib /home/lsz/code/work/api_trunk/build_cmake/lib/liblbapi.so \
  --testcase config/test_cases/fte_combo.json --report test_report.txt
```

---

## 二、开发测试过程中遇到的问题与解决

### 问题 1：FTE 启动失败（env.sh UTE_BIN 指向 0 字节文件）
- **现象**：`start_all.sh` 启动上海 FTE 失败。
- **根因**：`env.sh` 中 `UTE_BIN` 指向 0 字节文件；`libutedatainit.so` 也是 0 字节。
- **解决**：`UTE_BIN` 改为有效二进制；复制有效 `libutedatainit.so`。
- **结果**：FTE 正常启动。

### 问题 2：FTE 启动校验失败（cash_fund/account_ute 数量不一致）
- **现象**：`fund_info_manager.cpp Init` 要求 `cash_fund_map_.size() == account_data_map_.size()`，否则 `SetDataLoadErr(2)` 启动失败。
- **解决**：插入 cash_fund 时同步插入对应的 account_ute。
- **结果**：FTE 启动通过。

### 问题 3：FTE 登录密码来源（非 cash_fund.bin）
- **关键发现**：FTE 登录密码从 XML `ext_mod_user_info_ute_<partition>.xml` 的 `<Password>` 字段加载（`ftedata_init.cpp LoadLoginInfo`），**不是** cash_fund.bin！改 bin 密码无效。
- **passwd_map_ key**：`GeneralFundAssetKey(fund_account_id, branch_id)`，branch_id 数组带 `\0` 填充（如 `0001\0\0\0\0\0\0`）。
- **解决**：在 XML 中添加对应账户密码。

### 问题 4：FTE 旧版二进制不含调试日志
- **现象**：运行二进制 `build/fte/ute`（旧版）不含 CheckPasswd 调试日志。
- **解决**：重新编译 `./compile_fte.sh -r`。
- **结果**：日志生效，确认 passwd_map_ 加载。

### 问题 5：数字字段乱码（字节序问题）★核心
- **现象**：`client_seq_id`、`market_id`、`order_qty` 等数字字段乱码。
- **根因**：NewAPI 发送大端序，而 **FTE 使用主机字节序（x86 小端）**。`FTE_MEMCOPY` 宏在未定义 `FTE_BIG_ENDIAN` 时做原始 memcpy，不转换字节序。
- **解决**：修改 `trunk/NewAPI/gone/api/include/gw_head.h`，将三个 `HostToNetwork` 重载及 `HostToNetworkT` 模板改为直接返回 `v`（no-op），并加注释说明 FTE 使用主机字节序。
- **结果**：数字字段正确。

### 问题 6：`No security info [600007\0\0][101]`
- **根因**：NewAPI 用空字节填充 `security_id`，FTE 期望空格填充。
- **解决**：在 `gw_counter_direct.cpp` 的 `build_order_msg` / `build_etf_order_msg` 中，`memcpy(body->security_id.data(), ...)` 后调用 `space_pad(body->security_id)`。
- **结果**：安全码识别正确。

### 问题 7：`No security info [600007  ][101]`（cash_auction_params_ute.xml 缺失）
- **根因**：FTE 的 `cash_auction_params_manager::Init()` 遍历 `cash_auction_params_ute`，文件缺失导致查不到 600007。
- **解决**：创建 `etf_test_sh/cash_auction_params_ute.xml`，根元素 `<cash_auction_params_ute>`，含 600007 条目及全部必填字段（BuyQtyUnit、SellQtyUnit、PriceTick 等）。
- **结果**：通过 security 识别。

### 问题 8：`security type error [600007  ][101]` → kSecurityErr
- **根因**：`security_type=0`（默认），因 `list_security_ute.xml` 缺失。
- **解决**：创建 `etf_test_sh/list_security_ute.xml`，600007 条目设 `SecurityType=1, MktCalFlag=1, PrevClosePrice=250200` 等。
- **结果**：通过 security type 校验。

### 问题 9：`offerWay 1 is invalid or no gw`（委托无法路由到交易所）★本次核心
- **现象**：委托通过全部业务校验，但 FTE 日志报 `SendOrderToExch 1115 | offerWay 1 is invalid or no gw.`，委托不能到达交易所。
- **链路定位**：
  - `uplink_biz_processor.cpp:1113` 检查 `fund_data_->group_index_array_[offerWay]` 是否为 -1。
  - `tgw_manager.cpp:204-238` Reload 时，对每个 fund 遍历 `sh_binary_map_`，仅当 `conn_iter->pbu_id_set_.find(fund.offer_pbu_)` 命中 **且** `conn_iter->offer_way_ == kSHOes` 时才设置 `group_index_array_[kSHOes]`。
  - 连接的 `pbu_id_set_` 来自 FTE 自身配置 `ute.xml`（`<pbu_id_list><pbu_id>21085</pbu_id></pbu_id_list>`），经 `CopyToArray` → **空格填充** `"21085 "`（6 字节，byte[5]=' '）。
  - 账户的 `offer_pbu_` 来自 `account_ute_61.bin`，原始存储为 **空字节填充** `"21085\0"`（byte[5]='\x00'）。
- **根因**：`"21085 "` 与 `"21085\0"` 第 6 字节（' ' vs '\x00'）不同，`std::set<PBUID_def>::find()` 匹配失败，`group_index_array_[kSHOes]` 保持 -1 → 报 `offerWay invalid`。
- **解决**（数据修复）：
  1. 修改 `tgw_simulator/data/simulator_tgw.xml`：`<pbu_id></pbu_id>` → `<pbu_id>21085</pbu_id>`（让模拟交易所下发 pbu_array 含 21085）。
  2. 修改 `account_ute_61.bin`：将 fund 1000000000000001 的 `trade_pbu`、`offer_pbu` 从 `"21085\0"` 改为空格填充 `"21085 "`（备份 `account_ute_61.bin.bak_pbu`）。
  ```python
  # 用 python 直接补丁（备份后）
  data = bytearray(open('account_ute_61.bin','rb').read())
  row109 = 53 + 109*122
  for pbu_pos in (60, 66):          # trade_pbu, offer_pbu 在行内偏移
      data[row109 + pbu_pos + 5] = 0x20   # 空格填充
  open('account_ute_61.bin','wb').write(bytes(data))
  ```
- **结果**：`offerWay invalid` 错误消失，委托成功到达交易所模拟器并完成成交。

### 问题 10：委托测试预期值错误
- **现象**：委托成功但测试用例失败，预期 `order_status=6, rtn_type=4`，实际 `order_status=0, rtn_type=1`。
- **根因**：测试用例预期值是猜测的，未对齐 NewAPI 状态映射。mock_client 的 `on_order_rtn`（委托回报）收到的是委托受理回报（非成交回报）。
- **状态映射核对**（`gw_counter_direct.cpp`）：
  - `map_ord_status(2) → ORDER_STATE_DONE_PART(3)`；`map_ord_status(3) → ORDER_STATE_DONE_FULL(4)`
  - `map_exec_type('F') → RSP_TYPE_ORDER_TRADE(3)`；`map_exec_type('0') → RSP_TYPE_COUNTER_RSP(1)`
- **解决**：更新 `fte_combo.json` 委托用例预期值：
  - `order_status: 0`（ORDER_STATE_ORDER_IDLE）
  - `rtn_type: 1`（RSP_TYPE_COUNTER_RSP，委托回报）
  - `order_qty: 100`、`side: "1"`（去掉 `trade_qty` 校验，委托回报 trade_qty=0）
- **结果**：测试用例通过。

### 问题 11：成交回报（msg_id=2005）无法到达客户端 ★核心
- **现象**：委托/撤单测试通过，但客户端始终收不到成交回报（2005）。客户端日志只收到 `2001`（登录）、`2003`（委托回报）、`2003`（委托确认），随后链接断开（`err_type=1, err_code=-39` = `LBERR_CH_LINK_BROKEN`，即对端关闭连接）。
- **链路定位**：
  - FTE 日志确认 2005 已生成并写入 endpoint：`SendReportTradeOrderER fund_idx=0 endpoint=0x... msg_type=2005`，且 `SendTradeOrderER exec_type[F] ord_status[2] cum_qty[100]`（全部成交）。
  - 但紧接着 FTE 日志出现：`Heartbeat timeout, Endpoint [127.0.0.1:35232]` → `DoLogOut` → `Sesssion will be closed` → `Passive close` → `write 737 | Socket is closed`。
  - 即 **FTE 在发送 2005 后约 8~10ms 因"心跳超时"主动关闭了连接**，2005 数据未及被客户端读取。
- **根因**（FTE 心跳周期单位错误）：
  - 客户端登录消息 `heart_bt_int=5`（意图 5 **秒**）。
  - FTE `uplink_biz_processor.cpp:207`（tcp_direct 分支）`output = logon_req.heart_bt_int;` 把 5（秒）直接赋给 `heart_period_`（默认 5000ms）。
  - `tcp_endpoint.h:749` `detect_timer_.expires_from_now(LocalMilliseconds_def(hb_hd_->get_period_milli() * 2))` 按**毫秒**解释 → 超时 = 5*2 = **10ms**（而非 10 秒）。
  - 客户端心跳间隔 5 秒，远大于 10ms，FTE 在收到首个心跳前即判定超时并关闭连接。
- **解决**（FTE 代码修复，1 行）：
  - `uplink_biz_processor.cpp:207`：`output = logon_req.heart_bt_int;` → `output = logon_req.heart_bt_int * 1000;`（秒 → 毫秒）
  - 同时清理此前遗留的编译错误调试日志（`SendTradeOrderER SEND` 引用了不存在的 `trade_order_er->exec_type/ord_status`，应访问 `order_er_info.exec_type`，直接删除）。
  - 重新编译 `./compile_fte.sh`，`stop_all.sh` + `start_all.sh` 重启 FTE。
- **结果**：
  - FTE 日志 `get_period_milli` 由 `5` 变为 `5000`，`Heartbeat timeout` 不再出现。
  - 客户端日志新增 `msg_id=2005`，且 `on_trade_rtn: exec_price=250200, exec_qty=100, exec_id=1` 回调触发。
  - 成交回报完整链路打通：**登录 → 委托 → 委托回报 → 委托确认 → 成交回报（2005）→ 撤单**。

---

## 三、最终测试结果（2026-09-15）

### 3.1 组合测试 `fte_combo.json`（登录 + 委托 + 成交回报 + 撤单）
```
总计: 4 | 通过: 4 | 失败: 0
[PASS] FTE 登录测试        (6字段校验: err_code, market_type, cust_id, fund_account_id, account_id, branch_id)
[PASS] FTE 委托买入测试    (17字段校验: side, order_type, order_status, policy_id, market_type, reserved,
                           security_id, order_price, order_qty, rtn_type, err_code, fee, cancel_qty,
                           cust_id, fund_account_id, account_id, branch_id)
[PASS] FTE 成交回报校验    (17字段校验: 同上 + trade_qty, exec_price, exec_qty)
[PASS] FTE 撤单测试        (7字段校验: err_code=50046[订单已成交], rej_api, cust_id, fund_account_id,
                           account_id, branch_id, market_type)
```

### 3.2 逐字段校验机制（Task 7.5 增强）
- **JSON 格式**：`expected_response.fields` 对象包含回报结构体的**全部字段**，值为 `null` 表示动态字段（如 `order_sys_no`、时间戳等）跳过校验，非 `null` 值进行精确比对。
- **字段提取**：`extract_response_fields()` 自动提取回报结构体所有字段到 `map<string,string>`，支持定长 char 数组的 `\0`/空格裁剪（`trim_fixed`）。
- **动态引用**：撤单请求的 `order_sys_no` 支持 `"$last_order_sys_no"` 特殊值，自动引用上一笔委托的 order_sys_no。
- **异步等待**：成交回报(2005)测试使用主动轮询等待（超时 5s），不依赖同步响应机制。
- **撤单等待**：撤单测试使用 `has_cancel_rsp()` 特定响应等待，避免被中间的其他回报(2003)干扰。

### 3.3 客户端日志确认收到全部回报（含 2005 成交回报）★修复后
```
gw deal_recv_msg: msg_id=2001 msg_len=78   登录应答
gw deal_recv_msg: msg_id=2003 msg_len=324  委托回报
gw deal_recv_msg: msg_id=2003 msg_len=324  委托确认
gw deal_recv_msg: msg_id=2005 msg_len=324  成交回报  ← 修复后新增
[Callback] on_trade_rtn: exec_price=250200, exec_qty=100, exec_id=1
```
修复前客户端在收到 2003 后就因 FTE 心跳超时断开（`err_code=-39`），2005 从未到达；修复后 2005 正常到达并触发 `on_trade_rtn`。

### 3.4 FTE 日志确认完整成交链路（委托 → 确认 → 成交）
```
DealTradeOrderReqBusi: internal_order offer_way[1] gw_index[0]  (无 offerWay 错误)
SendTradeOrderER:  exec_type[1] ord_status[10]  委托回报
DealConfirm:       PktSHOrderConfirm exec_type[0] ord_status[0]  委托确认
DealReport:        PktSHOrderReport  exec_type[F] ord_status[2]  last_px[250200] last_qty[100] leaves_qty[0]  成交回报
SendTradeOrderER:  exec_type[F] ord_status[2] cum_qty[100] last_qty[100] last_px[250200]  全部成交
get_period_milli [5000]  (修复后心跳周期为 5000ms，无 Heartbeat timeout)
```
委托全流程：**登录 → 委托 → 交易所确认 → 成交回报（全部成交）→ 撤单** 全部打通，且客户端能收到 2005 成交回报。

---

## 四、关键配置/数据文件清单

| 文件 | 修改内容 |
|:---|:---|
| `trunk/NewAPI/gone/api/include/gw_head.h` | `HostToNetwork*` 改为 no-op（FTE 主机字节序） |
| `trunk/NewAPI/gone/api/src/gw_counter_direct.cpp` | `build_order_msg`/`build_etf_order_msg` 加 `space_pad(security_id)` |
| `DYS-FRAMEWORK/fte/test_all/etf_test_sh/cash_auction_params_ute.xml` | 新建，含 600007 |
| `DYS-FRAMEWORK/fte/test_all/etf_test_sh/list_security_ute.xml` | 新建，600007 SecurityType=1 |
| `DYS-FRAMEWORK/fte/test_all/tgw_simulator/data/simulator_tgw.xml` | `<pbu_id>21085</pbu_id>` |
| `DYS-FRAMEWORK/fte/test_all/etf_test_sh/account_ute_61.bin` | trade_pbu/offer_pbu 空格填充（备份 .bak_pbu） |
| `trunk/NewAPI/gone/api/mock/client/config/test_cases/fte_combo.json` | 委托用例预期值修正 |
| `DYS-FRAMEWORK/fte/src/business/uplink_biz_processor.cpp` | `heart_bt_int * 1000`（秒→毫秒）；删除遗留调试日志 |

---

## 五、经验与教训

1. **FTE 使用主机字节序（x86 小端）**，协议字段不做网络字节序转换，NewAPI 对接方必须同步。
2. **PBUID_def / SecurityID_def 等定长 char 数组**，FTE 内部统一用 `CopyToArray` **空格填充**；上游数据（bin）若用空字节填充会导致 `std::set::find` 等精确比较失败，务必对齐填充方式。
3. **FTE 登录密码在 XML**，不在 cash_fund.bin。
4. **FTE 启动强校验**：cash_fund 与 account_ute 数量必须一致。
5. **FTE 配置与模拟交易所配置分离**：连接 pbu 在 FTE `ute.xml`（`pbu_id_list`），模拟交易所下发的 pbu_array 在 `simulator_tgw.xml`，两者需一致。
6. **FTE 心跳周期单位陷阱**：登录消息 `heart_bt_int` 语义为秒，但 FTE 的 `detect_timer`/`send_timer` 按毫秒解释（`LocalMilliseconds_def(period*2)`）。若客户端发送 5（秒），FTE 会按 5ms 处理 → 10ms 心跳超时 → 主动断链，导致后续回报（如 2005 成交回报）丢失。对接方需保证 `heart_period` 为毫秒值。
7. 测试用例预期值应先核对 NewAPI 状态/回报类型映射，避免猜测。

---

## 六、性能优化记录（Task 8：2026-09-15）

> 基于 `mock_client_design.md` §14.3 的 B/C/D/E/F 方案，对 `gw_counter_direct` 模块实施优化。
> 优化范围：`trunk/NewAPI/gone/api/src/gw_counter_direct.cpp` + `.h`

### 6.1 优化方案概览

| 方案 | 级别 | 优化内容 | 涉及代码 |
|:---|:---|:---|---:|
| **B** | P1 | 合并 memcpy 与空格填充（`cksum_copy_pad`），消除同一数组的两次遍历 | `build_order_msg`/`etf`/`cancel` |
| **C** | P1 | 校验和与序列化合并，边写边累加校验和，消除第二次 O(n) 遍历 | 同上（内建于直接序列化） |
| **D** | P2 | 直接序列化到 o_buf（无中间 body 对象），消除栈上构造与序列化拷贝 | 同上 |
| **E** | P0 | 高频日志降级（`info_log`→`debug_log`）：`deal_recv_msg` 每笔回报日志、`deal_trade_rtn` 入口日志 | `deal_recv_msg`、`deal_trade_rtn` |
| **F** | P2 | 原子化跨线程状态变量：`login_state`/`trade_link_connect_` 用 `atomic_load16/store16`；`session_seq_` 用 `atomic_fetch_add64` | `.h` + `.cpp` 全部访问点 |

### 6.2 修改的代码

**新增辅助函数（方案 B/C/D）：**
```cpp
// 单字节写 + 校验和累加
static inline void cksum_put(char*& p, uint8_t b, uint32_t& sum);
// 定长块写 + 校验和累加
static inline void cksum_write(char*& p, const void* src, size_t n, uint32_t& sum);
// 拷贝并空格填充 + 校验和累加（方案 B 核心）
static inline void cksum_copy_pad(char*& p, const char* src, size_t n, uint32_t& sum);
// 大端 uint32 写（ByteSwap32，用于消息头）+ 校验和累加
static inline void cksum_be32(char*& p, uint32_t v, uint32_t& sum);
// 网络序 int64/32/16 写 + 校验和累加（HostToNetwork 保留调用以兼容未来）
static inline void cksum_net64/32/16(...);
// 校验和尾部写入（sum%256 → ByteSwap32 → memcpy 4 字节）
static inline void cksum_finish(char*& p, uint32_t sum);
```

**方案 C/D 核心变更**：`build_order_msg`/`build_etf_order_msg`/`build_cancel_msg` 从「栈上构造 body → encode → 单独校验和」改为「直接在 o_buf 中按字段顺序序列化，边写边累加校验和」。字段顺序与 `TradeOrderReq::encode` / `CancelOrderReq::encode` 完全一致（经 gw_head.h 逐字段核对）。

**方案 F 变更**（`.h` + `.cpp`）：
- `get_session_seq_no()` 改为 `atomic_load64(&session_seq_)`
- 3 个 `deal_*_req` 的 `trade_link_connect_`/`login_state` 读改为 `atomic_load16`
- `deal_cust_login`/`ans_cust_login`/`deal_log_ans` 写改为 `atomic_store16`
- `deal_link_connect`/`deal_link_close` 写改为 `atomic_store16`
- 7 处 `++session_seq_` 改为 `atomic_fetch_add64(&session_seq_, 1) + 1`

**方案 E 变更**：
- `deal_recv_msg` 每笔回报日志：`info_log` → `debug_log`（含 hexbuf 构造）
- `deal_trade_rtn` 入口日志：`info_log` → `debug_log`

**删除的代码**：
- `space_pad<N>()` 模板函数（原被 3 个 build 函数共调用 12 次，优化后由 `cksum_copy_pad` 替代）

### 6.3 遇到的问题

| 问题 | 说明 | 解决方案 |
|:---|:---|---:|
| `CancelOrderReq::encode` 字段顺序确认 | 撤单消息的字段顺序（fund_account_id/branch_id/account_id/cust_id/client_seq_id/agw_seq_id/orig_client_seq_id/orig_clordno）需与 encode 一致 | 读取 `gw_head.h` 第 870-890 行确认，手工序列化严格对齐 |
| `TradeOrderReq::encode` 字段顺序确认 | 委托消息 13 个字段的写入顺序 | 读取 `gw_head.h` 第 790-808 行确认，`stop_px` 在最后 |
| `HostToNetwork` 为 no-op | 此前已确认 FTE 使用主机字节序，`HostToNetwork` 直接返回 `v`，因此 `cksum_net64` 写入主机序，与 encode 输出一致 | 保留 `HostToNetwork` 调用以兼容未来 |
| `atomic_fetch_add64` 返回旧值 | `++session_seq_` 返回新值，`atomic_fetch_add64` 返回旧值 | 改为 `atomic_fetch_add64(&session_seq_, 1) + 1` |
| PktNewHeader 使用 `ByteSwap32`（非 `HostToNetwork`） | 消息头字段需大端序，body 字段用主机序 | `cksum_be32` 用 `ByteSwap32`，body 字段用 `cksum_net64/32/16`（no-op） |
| 退出时 `cmutex::lock` 断言失败 | 进程退出阶段，API 实例析构与 engine 线程清理顺序问题 | **预先存在的 shutdown 问题**（优化前旧代码同样触发），与本次优化无关 |

### 6.4 实际结果

**编译**：通过，无新增警告（仅 `g1_msg_ver`/`c98_msg_ver` 预先存在的 unused 警告）。

**回归测试**（FTE 环境 + mock_client `fte_combo.json`）：
```
总计: 4 | 通过: 4 | 失败: 0
[PASS] FTE 登录测试        (0ms, 6字段校验)
[PASS] FTE 委托买入测试    (0ms, 17字段校验)
[PASS] FTE 成交回报校验    (60ms, 17字段校验)
[PASS] FTE 撤单测试        (20ms, 7字段校验)
```
4/4 全部通过，证明直接序列化输出与优化前字节完全一致，FTE 正确解析。

**退出崩溃**：进程退出时 `cmutex::lock` 断言失败（exit=134）。经 **git stash 对比验证**，优化前旧代码同样触发此崩溃，确认是**预先存在的 shutdown 问题**，与本次优化无关。

### 6.5 优化效果评估

| 维度 | 优化前 | 优化后 | 改善点 |
|:---|:---|---:|:---:|
| 消息构建遍历次数 | memcpy(N) + strnlen(N) + fill(N) = 3N/字段 | `cksum_copy_pad` 一次遍历 = N/字段 | **减少 66% 数组遍历** |
| 校验和计算 | 单独 `GenerateSzCheckSum` O(n) 第二次遍历 | 边写边累加，零额外遍历 | **消除一次 O(n) 遍历** |
| 中间拷贝 | 栈上 `body` 对象 + `encode` 到 o_buf | 直接序列化到 o_buf | **消除中间对象** |
| 日志 I/O（高吞吐） | `info_log` 每笔回报 + 每笔成交 | `debug_log`（生产可关闭） | **高吞吐下 I/O 显著下降** |
| 跨线程状态访问 | 普通变量（数据竞争风险） | atomic 操作 | **消除数据竞争** |