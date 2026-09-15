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

### 3.1 组合测试 `fte_combo.json`（登录 + 委托 + 撤单）
```
总计: 3 | 通过: 3 | 失败: 0
[PASS] FTE 登录测试        err_code=0, market_type=1
[PASS] FTE 委托买入测试    order_status=0, rtn_type=1, order_qty=100, side=1
[PASS] FTE 撤单测试        err_code=0
```

### 3.2 客户端日志确认收到全部回报（含 2005 成交回报）★修复后
```
gw deal_recv_msg: msg_id=2001 msg_len=78   登录应答
gw deal_recv_msg: msg_id=2003 msg_len=324  委托回报
gw deal_recv_msg: msg_id=2003 msg_len=324  委托确认
gw deal_recv_msg: msg_id=2005 msg_len=324  成交回报  ← 修复后新增
[Callback] on_trade_rtn: exec_price=250200, exec_qty=100, exec_id=1
```
修复前客户端在收到 2003 后就因 FTE 心跳超时断开（`err_code=-39`），2005 从未到达；修复后 2005 正常到达并触发 `on_trade_rtn`。

### 3.3 FTE 日志确认完整成交链路（委托 → 确认 → 成交）
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