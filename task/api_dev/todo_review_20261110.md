# API 仓库"待确认 / todo"复盘（2026-11-10）

> **范围**：`trunk/NewAPI/gone/api` 的 `src/`、`include/`、`counter98/`（不含 `api-compare/`——那是框架开发者转存副本，不参与编译）。
> **方法**：全仓 `grep -nE "待确认|\btodo\b|TODO"` 定位标记 → 逐条读上下文判定"待确认的是什么 / 影响哪个链路" → 另做一遍"无标记但语义可疑"的人工复核。
> **基线对比**：2026-11-10 上午的《字典映射分析》统计为 **47 处**（src 28 + include 19）；本轮修复（P0 两处 + 字典补齐 + 作者手改 3 处）后降至 **38 处**。

---

## 0. 摘要

| 指标 | 数值 |
|------|------|
| 现存标记数 | **38**（`counter98.cpp` 16 / `struct_req.h` 14 / `counter98.h` 2 / `gw_counter_direct.cpp` 2 / `order_trade_type.h` 2 / `common_struct.h` 1 / `fpga_counter_base.cpp` 1）→ 归并为 **20 个逻辑待确认项**（`struct_req.h` 14 条同属「回报未知」，合并为 E2） |
| 本轮已闭环 | **9**（gw 7 + `api_interface.h` 1 + `common_struct.h` 1） |
| **P0（静默出错：假登录成功 / 错误成交 / 资金风险）** | **3 项**：`B1` AGW 登录假成功、`A4` ETF business_type、`B6` 98 报文主机序 |
| P1（字段失真 / 回报缺失 / 能力缺口） | **17 项**（A1/A2/A3/A5/A6、B2/B3/B4/B5/B7、C1/C2、D1/D2、U1/U3/U5） |
| P2 / P3 | P2 **4 项**（E1/E2/U2/U4）、P3 **1 项**（E3）；另**无标记但语义可疑 5 项**（U1~U5，见 §3） |
| 结论 | 剩余待确认**几乎全部依赖真实柜台联调**（98 AGW / FTE），离线无法闭环；其中 **B1、A4、B6 属"错了会静默出错"的类型**，建议列为联调首日必验项 |

**一句话影响面**：待确认项集中在 **①98 登录语义 ②98 委托/回报字典 ③ETF 业务类型 ④个微（FTE）回报字典边界**；一旦判错，表现为"登录假成功→后续全拒单"、"委托类型被当市价成交"、"ETF 报文被拒或当回购"、"回报状态/类型失真导致客户端状态机错乱"。

---

## 1. 本轮已闭环的 9 处（对照基线）

| # | 原位置 | 原待确认内容 | 闭环方式 |
|---|--------|--------------|----------|
| 1 | `gw_counter_direct.cpp` `fill_er_common` | `order_status` 字典映射 `[待确认]` | 依据 FTE `OrdStatus_def` 与 CTE/API 同义 → 改**直通**，仅 `13/99` 兜底（P0-1） |
| 2 | `gw_counter_direct.cpp` `build_order_msg` | `side` 字典"未证实，按 ATP 透传" | `side` 与 FTE `Side` 同值同义（买/卖/申/赎），注释改为"直通"并说明适用范围 |
| 3 | 同上 | `order_type` 字典"未证实，透传" | 新增 `map_api_order_type_to_fte()` 按市场映射（P0-2） |
| 4 | `gw_counter_direct.cpp` `build_etf_order_msg` | `todo: ETF 固定填 '2'` | 改为按市场取限价码（SH `'2'` / SZ `'1'`） |
| 5 | `gw_counter_direct.cpp` `fill_er_common` | `order_sys_no` 取 `fte_order_sys_no()` 待确认 | 作者已改为 **`er.clordno`**（委托/成交回报给 FTE 内部单号，客户撤单时原样回传）；撤单回报 `deal_cancel_rsp` 用 `er.orig_clordno`（被撤原单号）——两处均正确 |
| 6 | `include/api_interface.h` | 卖券还款 `X`"待确认，暂不支持" | 作者已改为"走非加速模式" |
| 7 | `include/common_struct.h:162` | `transact_time` 精度 todo | 已明确"统一取纳秒"（**注意 95 行同名字段未同步，见 E1**） |
| 8 | `include/order_trade_type.h` | 缺 `BUSINESS_TYPE_*`、`TIME_IN_FORCE_*` 等整字典 | 已补 96 个常量（CTE 6 域 100% 覆盖） |
| 9 | `src/fpga_counter_base.cpp:136` | 原 `todo`（order_status） | 已解，仅剩"原 todo 已解"说明性标记（可清理，见 E3） |

---

## 2. 现存 38 处（按链路分组）

> 严重度：**P0** 静默出错（假成功/错误成交/资金风险）；**P1** 字段失真或回报缺失；**P2** 可运维；**P3** 清理。
> 下表 **20 个逻辑项**承接 38 个代码标记（`struct_req.h` 14 条合并为 E2）。

### A. 98 委托出站（组包）

| 编号 | 位置 | 现状 | 待确认内容 | 影响 | 级别 |
|:----:|------|------|------------|------|:----:|
| A1 | `counter98.cpp:290` | 已有 6 个映射：`'2'`→kFixed、`'4'`→kLocalOptimal、`'3'`→kTransferFixed、`'1'/'5'/'6'/'7'`→**kMarket（市价）**，`default` 原值透传 | ① `'5'` 对手方最优映射为"市价"是否可接受（成交价可能显著偏离客户预期）；② `default` 未知类型应否改为拒绝而非透传 | 委托类型被柜台按市价撮合；未知类型透传会被柜台以"类型错误"拒单且回报难定位 | P1 |
| A2 | `counter98.cpp:320` `api_order_side()` | 已实现 3 组反向映射（配股/信用/两融），其余**原值透传** | CTE `AgwSide` 41 个值的剩余部分（转股 `'5'`/回售 `'6'`/预受要约 `'7'`/`'8'`/`A`/`C`/`F`/`G`/`J`/`K`/`M`/`O`/`P`/`Q`/`R`/`Y`/`Z`/`a`~`l`）如何映射回 API 侧别 | 回报 `side` 落在 API 字典**已补齐但业务未定义**的值上 → 客户端可能误判（如把"回售"当普通卖出） | P1 |
| A3 | `counter98.cpp:339`、`counter98.h:323`、`order_trade_type.h:87`、`counter98.cpp:1943`/`1970` | `api_order_type()` **恒返回 0**；成交回报 `order_type` 暂填 0 | 98 回报委托类型 → API 字典的映射表（CTE `AgwOrdType` 4 值 → API 7 值方向） | 客户端收到的 `order_type` 为 0（API 字典中 0 未定义）→ 若客户按类型做风控/统计会失真；需明确"0=未知"语义并从原委托记录取类型 | P1 |
| A4 | `counter98.cpp:392` | ETF `business_type` 取 **12**（kETFRealTimePurchaseRedemption），代码注释另提候选 **2=质押式回购** | 与柜台裁决 ETF 申赎的正确 business_type | **若判错**：ETF 报文被拒（业务中断），或最坏被当"质押式回购"处理（**资金/持仓风险**） | **P0** |
| A5 | `counter98.cpp:399` | ETF 仅填实例市场股东账号，另一市场留空 | 跨市场 ETF 是否必须填两个 market 的股东账号 | 跨市场 ETF 申赎失败（沪市代码在深市实例等） | P1 |
| A6 | `order_trade_type.h:50` | 卖券还款 `X` 标注「待确认 98 业务映射，当前委托返回不支持」；实查：`gw_counter_direct` 对 `X` 返回 `UNSUPPORTED_OP`，`counter98` 的 side 分派也**无 `X` 分支**（落 default → `UNSUPPORTED_OP`） | 98 侧承载卖券还款的（side, business_type）组合（候选：`AgwSide::kActivedSellRepay` 即 `'g'`，或 `kBuySecuRepay` 即 204） | **该业务当前全链路不可用**（极速拒 → 降级 98 也拒），客户端只得到「不支持」 | P1 |

### B. 98 登录与会话（含字节序）

| 编号 | 位置 | 现状 | 待确认内容 | 影响 | 级别 |
|:----:|------|------|------------|------|:----:|
| B1 | `counter98.cpp:819`、`1536` | `AgwMsgLogin` **不含错误字段**，实现语义=收到同号(3)入站消息即视为登录成功 | 柜台失败时是否也回同号消息（若回，则失败被当成功）；是否需要额外校验（会话号/后续心跳/查询可达性） | **登录假成功** → 后续所有委托被柜台拒（客户只看到一堆拒单，难定位）；本项已由 `login()` 返回值承载接入结果，但 AGW 层判定仍需确认 | **P0** |
| B2 | `counter98.cpp:811` | `sender_comp_id` = agw 用户名 | 柜台要求的发送方代码是否等于 agw 用户名（还是独立 ID） | 登录被拒（字段校验失败） | P1 |
| B3 | `counter98.cpp:1213` | `msg.time = 0`（发起验证时间） | 时间格式（YYYYMMDDHHMMSS? 毫秒? 是否允许 0） | 登录认证失败 | P1 |
| B4 | `counter98.cpp:1225` | 密码明文下发，`pwd_type` 未定 | 是否需要加密/`pwd_type` 取值 | 登录失败；或明文密码进入日志的安全隐患 | P1 |
| B5 | `counter98.cpp:1367` | `account_id` 从账户组"按市场匹配，无匹配取第一个" | 多市场账户组的选取规则；`market_id` 字典 | 股东账号填错 → 委托被拒（错误信息不直观） | P1 |
| B6 | `counter98.cpp:8` | 报文 `host_to_network=false`（**主机序直通**），注释自标"联调首验点" | 98 AGW 是否接受主机序（x86 小端） | **若判错**：所有数值字段字节序错乱 → 表现为"字段值离谱/全字段校验失败"，排查成本极高 | **P0** |
| B7 | `counter98.h:491` | C 串 → ATP 定长字段（尾部空格填充）"约定待确认" | 98 侧要求空格填充还是 `\0` 填充 | 与 FTE `pbu` 空格填充同类坑（历史上导致报单被拒且难定位） | P1 |

### C. 98 回报语义

| 编号 | 位置 | 现状 | 待确认内容 | 影响 | 级别 |
|:----:|------|------|------------|------|:----:|
| C1 | `counter98.cpp:1821` | 用 `orig_cli_ord_no != 0` 区分"撤单响应"与"委托确认" | 该判定是否对所有柜台版本成立（是否会出现 `orig_cli_ord_no` 非 0 的普通委托确认） | 回报被分发到错误回调（撤单应答 vs 委托回报）→ 客户端状态错乱 | P1 |
| C2 | `counter98.cpp:1873` | `deal_cancel_ans` 注："是否包括撤单确认、撤单成交、撤单废单？" | 撤单类回报的完整集合与消息号覆盖 | 漏处理的撤单回报**静默丢弃** → 客户端撤单请求永不返回（挂起） | P1 |

### D. 个微（FTE）通道

| 编号 | 位置 | 现状 | 待确认内容 | 影响 | 级别 |
|:----:|------|------|------------|------|:----:|
| D1 | `gw_counter_direct.cpp:255` | `map_fte_market_to_api()` **原值直通**，仍标 `[待确认]` | FTE `MarketID` 有 13 值（含 103 香港、107/108 B 股、110 基金、111/112 银行间、199 港股通），API 字典仅 6 值 | 回报 `market_type` 落在 API 未定义值 → 客户端按市场分支处理时走 default（统计/风控失真） | P1 |
| D2 | `gw_counter_direct.cpp:955` | 拒绝消息处理注："是否包括撤单等请求的拒绝？" | FTE `RejectMsg` 是否也覆盖撤单/ETF 请求 | 撤单被拒却收不到拒绝回报 → 客户端挂起 | P1 |

### E. 结构与协议缺口

| 编号 | 位置 | 现状 | 待确认内容 | 影响 | 级别 |
|:----:|------|------|------------|------|:----:|
| E1 | `common_struct.h:95` | `transact_time` 注"待确认具体逻辑"；而同文件 **162 行已改为"统一取纳秒"** | 两处同名字段语义应统一 | 文档自相矛盾 → 后续实现者可能按两种口径写，导致时间字段精度不一致 | P2 |
| E2 | `struct_req.h`（14 处） | 14 类业务委托"回报未知"：网上发行 101301、配股 101401、债券转股回售 101501、要约收购 101801、网络投票 102101、港股通公司行为 106201、港股通业务 106301、港股通投票 106401、科创板盘后定价 109701、担保品划转 7103701、现券还券 7105701、股转要约 7121701、新三板发行 7122701、ETF 网下认购 7130701 | 各业务的回报消息号与结构 | 若这些业务被下发，**回报无解析**（回调不触发）；目前属"协议资料缺口"而非缺陷 | P2 |
| E3 | `fpga_counter_base.cpp:136` | 注释"已拒绝 (字典已定, 原 todo 已解)" | 无（仅残留标记） | 无功能影响，属清理项 | P3 |

---

## 3. 无标记但语义可疑（人工复核发现 5 处）

| 编号 | 位置 | 现象 | 影响 | 级别 |
|:----:|------|------|------|:----:|
| U1 | `counter98.cpp:1867` | `out.err_code = msg.key_error_code.empty() ? 0 : LBAPI_ERR_INVALID_PARAM;` —— 任意柜台错误码**统一折叠为 -1（INVALID_PARAM）** | 客户端拿不到真实拒绝原因，只能看到"参数错误"，排障成本高 | P1 |
| U2 | `counter98.cpp:1865` | `cancel_qty = order_qty - leaves_qty - cum_qty`，注释"不钳制" | 部分柜台回报组合可能算出负数 → 客户端显示负撤单量 | P2 |
| U3 | `gw_counter_direct.cpp` `map_fte_exec_type()` | 直通，未处理 FTE 独有 `'J'`(黄金)/`'K'`(模拟) | 回报 `rtn_type` 出现 API 字典外字符（已补 `RSP_TYPE_GOLD`，但映射分支未加） | P1 |
| U4 | `gw_counter_direct.cpp` `deal_order_req` | `side` 为 `S/T/U/V/W/X` 时返回 `UNSUPPORTED_OP` | **属设计正确行为**：该错误会**触发 98 回退**（两融/担保品/配股/卖券还款由 98 承接，`counter98.cpp:400~430` 已实现）。此前分析文档 §3 P1-1 把它列为"会被拒单"，**此处更正**：不是缺口，但需联调验证 98 侧确实能承接 | P2（验证项） |
| U5 | `gw_counter_direct.cpp` `map_fte_market_to_api()` | 同 D1 | 同 D1 | P1 |

---

## 4. 影响面汇总（按链路）

| 链路 | 受影响的待确认项 | 一旦判错的可见后果 |
|------|------------------|--------------------|
| **98 登录** | B1、B2、B3、B4、B6、B7 | 登录假成功（后续全拒单）；或登录直接失败（字段/字节序/时间格式/密码/填充不符） |
| **98 委托** | A1、A2、A4、A5、B5 | 类型被当市价成交；ETF 被拒或误当回购（资金风险）；股东账号错 → 拒单 |
| **98 回报** | A2、A3、C1、C2、U1、U2 | 回报 `side`/`order_type`/`err_code` 失真；撤单回报漏分发 → 客户端挂起 |
| **个微（FTE）** | D1、D2、U3、U5 | `market_type` 越界；撤单拒绝回报缺失；`rtn_type` 出现字典外字符 |
| **协议覆盖** | E2 | 14 类业务可下发但回报无解析（当前未使用，风险潜伏） |
| **文档一致性** | E1、E3 | 后续实现者按错误口径编码 |

---

## 5. 建议的确认顺序与验证方法

| 顺序 | 确认项 | 找谁确认 | 验证方法 |
|:----:|--------|----------|----------|
| 1 | **B6 字节序** | 98 AGW 柜台（或 vendor 协议文档） | 联调第 1 步：发一条最小报文，抓包比对字段字节序 |
| 2 | **B1 登录语义** | 98 AGW 柜台 | 故意用错密码登录，观察柜台是否回同号消息；若回则必须改为"登录失败"判定 |
| 3 | **A4 ETF business_type** | 柜台业务 + `AgwBusinessType` 文档 | 用 12 与 2 各发一笔小额申赎，核对柜台受理结果（**勿在生产验证**） |
| 4 | A1/A2/A3 字典映射 | 柜台 | 按 `dict_mapping_20261110.md` 的映射表逐项发单 + 回放回报 |
| 5 | B2/B3/B4/B5/B7 | 柜台 | 组合用例：正确/错误凭据、多市场账户、定长填充 |
| 6 | C1/C2、D2、U1 | 柜台 + 自测 | 构造"撤单成功/撤单废单/撤单成交"三类回报，验证分发与 `err_code` |
| 7 | D1、U3、U5 | FTE | 构造 FTE 回报：13 个 market 值、`exec_type='J'/'K'` |
| 8 | E1/E2/E3 | 内部 | 文档口径统一 + 清理残留标记；E2 待协议资料补齐后补结构 |

> 建议把 **B1、A4、B6** 三项写进联调首日"必验清单"，因为它们都是**静默出错**类型：不会崩、不会报错，只会让业务结果错误。

---

---

## 6. 达成目标：gw counter 登录 / 委托 / 撤单 功能测试所需修改

> **目标界定**：让 `mock_client --plan test_plan_gw.json` 在本地 **FTE(33001) + 98 mock(9003) + 模拟交易所** 环境跑通「登录 → 委托 → 成交回报 → 撤单 → 心跳」。
> **重要范围结论**：该目标只走 **个微(FTE)** 通道，98 仅承担 **AGW 用户登录（前置）** 与失败降级。因此 §2 中 **A 组（98 委托/回报字典）全部与本目标无关**，不需要为跑通测试先解决。

### 6.1 测试链路与前置

```
mock_client --plan test_plan_gw.json
   └─ libg1api.so (api_impl::login)
        ├─[前置] counter98::deal_agw_login  →  98 mock (127.0.0.1:9003)   ← 必须先通
        └─[主链路] gw_counter_direct  →  FTE (127.0.0.1:33001)  →  tgw_simulator(38140/38141)
```

**关键前置**：`api_impl::login()` 第 1 步是 98 AGW 同步登录（`api_instance.cpp:394` `c98_.deal_agw_login()`），**AGW 登录不通过则极速柜台登录根本不会发起** → 98 mock 必须在线且凭据匹配。

### 6.2 阻断级（不改则一定跑不通）——**4 项**（B-2 经复核无需修改）

| # | 位置 | 现状 | 必须的修改 | 证据 |
|:--:|------|------|-----------|------|
| **B-1** | `mock/client/config/connection_config_gw_single.json`（及 `connection_config.json`、`cases/*.json`、`cases_gone/*.json`） | `market_type: 1`（旧字典）；`cases` 期望也是 1 | 改为 **101（沪）或 102（深）**，用例与预期同步；`connection_config.json` 已是 101 可作样板 | `deal_order_req` 校验 `req.market_type != market_type` 且必须 ∈{101,102}，否则返回 `LBAPI_ERR_INVALID_PARAM`；`market_type` 配置项是 int8，101/102 可容纳 |
| ~~B-2~~ ✅ | `gw_counter_direct.cpp` `fill_er_common()` | **已核实正确（无需修改）**：现为 `out.order_sys_no = er.clordno`（2026-11-10 17:38 已改），`deal_cancel_rsp` 为 `er.orig_clordno` | 无 | 依据 FTE `gw_external_message.cpp:641 FillWithInternalOrderInfo()`：`clordno=internal_order->clordno`（每条回报都有）、`orig_clordno` 仅撤单路径赋值（`uplink_biz_processor.cpp:1468/1591`）。→ **委托/成交回报必须用 `clordno`，撤单回报用 `orig_clordno`**，当前实现正是如此 |
| **B-3** | `mock/98_counter`（AGW 模拟柜台） | 已实现 Login/CustLogin/心跳/委托/撤单/各类查询（vendor `lb_counter98` 编码，msg_ver 会话=0/业务=2）；**未实现回报同步 `AgwMsgSync`/`AgwMsgSyncFinish`** | ① 测试配置**不要设置** `counter98_report_sync`（未配置时 API 跳过同步，见 `counter98.cpp:902`）；② 若要测同步，需在 mock 补 Sync/SyncFinish 应答 | `counter98.cpp` 分发含 `kAgwMsgSyncFinish`；`mock/98_counter/src/client_session.cpp` 无 sync 分支（grep 无命中） |
| **B-4** | FTE 部署产物 | `/mnt/work/gt_test/work_atp/cmake/fte/bin/ute` 时间 **10-19 16:47**，而 FTE 源码 `uplink_biz_processor.cpp` 为 **11-02** | **重新 `./compile_fte.sh -r` 并部署**，避免产物落后于源码 | 文件时间戳比对 |
| **B-5** | FTE 上场数据与用例 | `cases/login_request.json`：`fund_account_id=1000000000000001`、`branch_id=0001`、`password=TEST000000001`、`account_id=A12345678901` | 与 FTE 数据三方一致：`cash_fund.bin`/`account_ute_*.bin`（含 `trade_pbu`/`offer_pbu` **空格填充**）、`ute.xml` 的 `pbu_id`、`simulator_tgw.xml` 的 `pbu_array`；密码来自 `ext_mod_user_info_ute_<partition>.xml` 的 `<Password>`（**不是 bin**） | 历史坑：pbu 空字节填充导致 `offerWay invalid`；密码来源错导致 `kPasswdErr` |

### 6.3 会影响"测试结论是否可信"的高优先项

| # | 项 | 现状 | 建议 |
|:--:|----|------|------|
| H-1 | `cases/order_expected.json` 期望值 | `market_type=1`、`order_qty=100`、`rtn_type=1`、`order_sys_no=null` | 与 B-1 同步改字典值；确认数量口径（API/FTE 均"股×100"，100 股 ⇒ 10000，用例值需与实际下发一致） |
| H-2 | 撤单用例 `order_sys_no: "$last_order_sys_no"` | 依赖上一笔委托回报的 `order_sys_no`（现= clordno，已正确） | 建议断言「撤单成功后 `CancelRsp.order_sys_no` = 原委托的 clordno」 |
| H-3 | `trade_link_ready()` / `wait_link_ready()` | 已实现（`callback_handler` 按 `LINK_TYPE_SPEED_TRADE` 记录）；gw 单链接模式 `link_type=1` | 保留；确认 `--plan` 流程在登录完成后等待业务链接就绪再发单 |
| H-4 | 心跳周期 | API 侧已 `heart_bt_int = heart_interval * 1000`（`gw_counter_direct.cpp:502`），FTE 侧直赋（`uplink_biz_processor.cpp`） | **已正确，不需改**；若测试出现 5~15s 断链，先查此项与 `on_com_msg()` 业务消息保活 |
| H-5 | `--plan` 的 perf 段 | `test_plan_gw.json` 中 `perf_test.enable=true` | 跑功能测试时建议先置 `false`，避免性能压测干扰功能判定（或单独跑 perf） |

### 6.4 与本目标无关、可暂缓的项

| 待确认项 | 为何不影响本目标 |
|----------|------------------|
| A1/A2/A3/A4/A5/A6（98 委托与回报字典、ETF、卖券还款） | gw 目标只走 FTE 通道；98 仅做 AGW 登录，不下发委托 |
| C1/C2（98 回报语义）、U1（98 错误码折叠） | 同上，98 回报不参与 gw 功能测试（除非测试降级路径） |
| D1（`map_fte_market_to_api` 值域）、D2（拒绝消息范围） | 沪深 101/102 直通正确；买卖/撤单不触发越界值；仅在异常/拒绝场景才暴露 |
| E2（`struct_req.h` 14 类回报未知） | 测试只覆盖现金竞价买卖 + 撤单 |
| U3（`exec_type 'J'/'K'`） | 模拟交易所不会产生黄金/模拟撮合类型 |

### 6.5 执行顺序（建议）

| 步 | 动作 | 验证 |
|:--:|------|------|
| 1 | 改 B-1（配置与用例 `market_type` 1→101/102） | `mock_client` 启动日志无 `INVALID_PARAM` |
| 2 | （B-2 已验证无需改）确认委托回报 `order_sys_no` 非 0 | 撤单用例能取到非 0 的原单号 |
| 3 | 重编部署 FTE（B-4） | `ute` 时间戳新于源码；`status_all.sh` 端口监听 |
| 4 | 拉起 98 mock(9003) + FTE(33001) + 模拟交易所 | `counter98_mock` 起来后 AGW 登录成功（日志 `agw user login answer`） |
| 5 | 校验数据对齐（B-5） | 登录成功（`on_login` 无 err）；委托不被 `offerWay invalid` 拒 |
| 6 | 跑 `--plan test_plan_gw.json`（关闭 perf） | 5/5 通过：登录/委托/成交/撤单/心跳 |
| 7 | （可选）开 perf 段做性能基线 | 报告非 0 且口径已标注 v2 |

### 6.6 工作量估计

| 类别 | 项 | 预估 |
|------|----|------|
| 配置/用例 | B-1 + H-1（约 8 个 JSON 文件的 `market_type`） | 半小时内 |
| 环境 | B-3 起 mock、B-4 重编 FTE、B-5 数据对齐 | 1~2 小时（视 FTE 编译与数据核对） |
| 测试 | 跑通 5 用例 + 定位异常 | 半天内 |

> **建议的"部分修复"最小集**：**B-1（配置/用例字典值）**，其余为环境准备（起 98 mock、重编 FTE、对齐数据）。代码侧目前**没有**必须提前修的正确性阻断（B-2 经复核已正确）。

---

---

## 7. 本轮实施记录：gw 功能测试配置修复（2026-11-10 18:40）

> 目标：让 `mock_client --plan test_plan_gw.json` 具备跑通「登录/委托/成交/撤单/心跳」的配置前提。
> 规范：**凡属"待确认"的取值，一律在用例里保留 `_待确认` 标记**，供后续人工确认后删除或定稿。

### 7.1 改动清单

| # | 文件 | 改动 | 依据 |
|:--:|------|------|------|
| 1 | `connection_config_gw_single.json` | `market_type` 1→**101**；`single_cust_per_link` → **`need_multi_cust: 0`**；`perf_test.order` 的 `market_type`→101、`order_qty`→10000 | 用例证券 `600007` 属沪市；r21 起配置键改名（语义相等：单客户=0） |
| 2 | `connection_config_gw_multi.json` | 同上；`need_multi_cust: 1` | 多客户模式 |
| 3 | `cases/login_expected.json` | `market_type`→101 | 实例市场（LogOnAns 回填） |
| 4 | `cases/order_request.json` | `market_type`→101；`order_qty` 100→**10000**；移除 `policy_id`/`tgw_id`（r21 已下线字段） | 数量口径 = 股×100（API 自带离线用例 `test/order_insert_test.cpp` 同样用 10000 = 100 股） |
| 5 | `cases/order_expected.json` | `market_type`→101；`order_qty`→10000 | 与请求一致 |
| 6 | `cases/trade_expected.json` | `market_type`→101；`order_qty`/`trade_qty`/`exec_qty`→10000 | 同上 |
| 7 | `cases/cancel_expected.json` | `market_type`→101 | 与实例市场一致 |
| 8 | `cases/perf_order_request.json` | `market_type`→101；`order_qty`→10000 | 性能测试同口径 |
| 9 | `test_plan_gw.json` | `perf_test.enable` true→**false** | 功能测试先跑通；FTE 8000TPS 高负载易断链，会污染功能判定（性能测试单独开启） |
| 10 | `mock/client/src/test_case_runner.cpp` | 校验器新增：**字段名以 `_` 开头视为注释/待确认标记，跳过校验** | 让"待确认"标记可以安全地留在用例里而不导致用例必然失败 |

### 7.2 `待确认` 标记的存放规范（本次确立）

- **位置**：统一放在用例 JSON 的**顶层 `_待确认` 对象**里（不放进 `fields`/`order`，避免参与字段比对）。
- **内容**：`"<字段名>": "<当前取值的依据 + 需确认什么>"`。
- **兼容**：即便误放进 `fields`，校验器也会因 `_` 前缀跳过（`test_case_runner.cpp` 已加固），不会判失败。
- **清理条件**：联调确认后，把对应条目从 `_待确认` 删除（或转成注释性文档），**不要留下过期标记**。

当前标记分布：`login_expected` 1 条、`order_request` 5 条、`order_expected` 3 条、`trade_expected` 2 条、`cancel_expected` 1 条、`perf_order_request` 2 条、`test_plan_gw` 1 条 = **15 条待确认**。

### 7.3 关键待确认项（需联调拍板）

| 待确认点 | 当前取值 | 需确认什么 |
|----------|----------|-----------|
| 市场 | 101（沪） | 与 FTE 实例市场、上场数据（`security_info`）是否一致；若改用深市证券则需 102 |
| 数量口径 | 10000（=100 股×100） | FTE 的 `buy_qty_unit` 是否要求该值为整数倍（历史坑：不能整除会被拒） |
| 委托类型 | `'2'` 限价 | gw 侧按市场映射为 FTE SH `'2'`（已实现），联调验证实际报文 |
| 委托状态（委托回报） | 0 已申报 | 若模拟交易所立即回报成交，委托回报可能直接为 2/3，需按实际时序调整 |
| 委托状态（成交回报） | 3 部成部撤 | 全成应为 2；按模拟交易所实际撮合结果确认 |
| 撤单结果码 | 50046 | `kOrigOrderWrongStatus`（原委托已成交不可撤）；若要测"撤单成功"需改为未成交委托并置 0 |
| 成交价 | 250200 | 模拟交易所按委托价成交；真实柜台可能不同 |

### 7.4 验证

| 项 | 结果 |
|----|------|
| 编译（含 mock_client 改动） | ✅ `lbcommon/g1api/perf_client/mock_client` 四目标 `Built target`，0 error |
| 98 mock 重建 | ✅ `mock/98_counter` 独立编译通过（源码 10-16 新于原产物 09-16） |
| 配置语法 | ✅ 9 个 JSON 均可被 `json.load` 解析；`fields`/`order` 内无 `_` 前缀键 |
| 运行联调 | ⏳ 依赖 FTE 部署（`run_trunk` 缺 `ute`/`etf_test_sh`，需 `compile_fte.sh -r` + 部署），本轮已在执行 |

### 7.5 实测结果（2026-11-10 19:20，FTE 上海链路 + 98 mock + 模拟交易所）

环境：`ute`(33001, 新编译) + `mock_exch_server`(38140/38141) + `counter98_mock`(9003)。
命令：`mock_client --plan config/test_plan_gw.json`（perf 已关）。

| # | 用例 | 结果 | 关键日志 |
|:--:|------|------|----------|
| 1 | FTE 登录 | ❌ 失败 | `on_login: err_code=4, market_type=101` → **4 = `CheckError::kPasswdErr`** |
| 2 | FTE 委托买入 | ❌ 失败 | `order_insert() 返回: -35`（`LBAPI_ERR_NOT_LOG_CUST`，登录失败的连锁） |
| 3 | FTE 成交回报 | ❌ 失败 | `未收到成交回报(2005)`（委托未发出） |
| 4 | FTE 撤单 | ❌ 失败 | `order_cancel() 返回: -35`（同上） |
| 5 | FTE 心跳维持 | ✅ **通过** | 30s 无断链 |

**已验证生效**：
- `market_type=101` 被 FTE 接受（登录应答回填 101），**B-1 配置修复有效**。
- 98 AGW 登录链路通（`login()` 返回 0，AGW 会话建立），**B-3 前置成立**。
- 心跳（`heart_bt_int` ×1000）30 秒无断链，**H-4 口径正确**。
- 新 `ute` 与部署目录就绪（B-4 已做：编译+安装+部署 `run_trunk`）。
- 用例加载、`_待确认` 标记不影响校验（校验器 `_` 前缀跳过生效）。

**当前唯一阻塞：登录被 FTE 判 `kPasswdErr(4)`**。已排除的因素：
| 已排查 | 结论 |
|--------|------|
| FTE 是否加载该账户 | ✅ 已加载：`LoadLoginInfo insert: fund_account_id=[1000000000000001] branch_id=[0001      ] password=[TEST000000001`（`etf_test_sh/run/log/fteinit.txt`，本次 11:13 写入） |
| 定长填充是否不一致 | 登录表用 **空格填充**（`branch_id=[0001      ]`），与 API `to_fte_str` 的空格填充一致；`BranchID_def` 两侧均为 `array<char,10>` |
| 密码是否透传 | 链路完整：`mock(LoginReq.password)` → `counter98` 组 `acc_login_event_info`（`str_copy_format(password)`）→ `gw build_login_msg`（截 99B，尾部空格）→ FTE `AsStringTrim(password)` 后比较 |
| 密码取值 | 用例 `TEST000000001` 与 XML `<Password>` **完全一致** |

**剩余疑点（需下一步定位）**：`LogOnReq` 报文在两侧的字段偏移是否一致（若 `password` 落点错位，则 FTE 读到的密码必然不匹配），或 `CheckPasswd` 的 key 与报文实际值存在差异。
建议手段：① 把 ute 以 `-l Debug` 启动（`start_all.sh` 写死 `-l Info`，需临时改启动参数），抓 `DealLogon message` 的 dump；② 或 `tcpdump` 抓 33001 的登录报文，按 `gw_head.h` 的 encode 顺序解码 `password`/`branch_id` 偏移。

### 7.6 登录打通与 counter98 字典改动（2026-11-11）

#### 7.6.1 登录失败的完整根因链（3 处定长字段截断 + 1 处 mock 比较方式）

| # | 位置 | 问题 | 修法 |
|:--:|------|------|------|
| 1 | `mock/98_counter/src/client_session.cpp` | AGW 用户/密码用 `strnlen` 取串：API 按定长**空格填充**发送 → 取得 `"agw_user_01␣␣␣…"` → 精确比较失败（日志 `AGW login failed`） | 新增 `fixed_to_string/trim_string`（NUL/空格/TAB 裁剪），AGW 与账户登录字段统一裁剪 |
| 2 | `counter98::build_cust_login_event` | `str_copy_format` 最多拷 `dst_size-1` → 16 字节资金账号被截成 **15 位** | 新增 `copy_fixed_full`（清 0 + 满宽拷贝） |
| 3 | `counter98::from_atp_cstr`（`counter98.h`） | 同样按 `dst_len-1` 截断 → 极速登录事件里资金账号 15 位 | 允许满宽（`n > dst_len` 才截），`n < dst_len` 时补 NUL |
| 4 | `gw_counter_direct::from_fte_str`、`counter98::from_atp_str` | 回报侧按 `N-1` 截断 → 登录回报 `fund_account_id/cust_id/account_id` 各少 1 位 | 同上改为允许满宽；满宽字段按定长消费 |

**结果**：`on_login: err_code=0` ✅，登录用例 6/6 字段全对。

#### 7.6.2 counter98 字典改动（按已确认字典，保留原 `待确认` 标记）

| 位置 | 改动 | 说明 |
|------|------|------|
| `build_order_msg`（委托类型出站） | 补映射依据注释：`'2'→kFixed`、`'4'→kLocalOptimal`、`'3'→kTransferFixed`、`'1'/'5'/'6'/'7'→kMarket`；`default` 原值透传 | **原 `// todo : 待确认，委托市价类型如何映射98字典` 标记保留未动**；新增说明"语义收窄 + 若业务不接受需前置拒绝（当前未实现，待确认）" |
| `api_order_side`（方向回映） | 补说明：API `ORDER_SIDE_*` 已按 CTE 41 值补齐，非折叠业务直通；仅配股/担保品/两融三类需由 `(business_type, side)` 反推 | **原 `// todo : 待确认，98 的委托方向和业务类型…` 标记保留未动** |
| `api_order_type`（委托类型回映） | 由"恒返回 0"改为映射：`kFixed→'2'`、`kLocalOptimal→'4'`、`kTransferFixed→'3'`；`kMarket` 及其它 → 0（API 无"通用市价"，避免与 `'1'` 五档IOC 混淆） | **原 `// todo: 待确认 … 暂填 0。` 标记保留未动**；因 vendor 常量为运行期 `static const`，用 if 比较（不可作 case 标签） |

> `counter98.cpp` 中 `待确认` 标记数量核对：修改前后均为 **7 处**（未被删除或改写）。

#### 7.6.3 当前测试结果（gw 功能测试，5 用例）

| # | 用例 | 结果 |
|:--:|------|:----:|
| 1 | FTE 登录 | ✅ 通过（6/6 字段） |
| 2 | FTE 委托买入 | ❌ 等待回报超时 |
| 3 | FTE 成交回报 | ❌ 未收到 2005 |
| 4 | FTE 撤单 | ❌ 未收到撤单应答 |
| 5 | FTE 心跳维持 | ✅ 通过（30s） |

**已知事实**：`order_insert()` 返回 **0**（已入引擎队列），但 API 侧 DEBUG 日志无任何发送记录，FTE 当日日志无 `600007` 相关记录 → **委托未真正到达 FTE 或被静默丢弃**。
**环境侧**：上海链路走 `SSETDGW → STEP 库 39144`，`mock_sqlserver` 先前未启动（`run_trunk` 缺该二进制，已从 `run/` 补齐并在 `run/` 目录启动，39144 已监听）。
**下一步定位**：① `mock_sqlserver --fill` 与 38140/38141 的撮合/回报链路是否生效；② 若仍无 2003，需抓包确认 API 是否真发出 1003（容器无 tcpdump，可用 `api_net_time_capture` 或加临时 debug 打印）；③ 校验 FTE 上场数据 `account_ute_61.bin` 的 `trade_pbu/offer_pbu` 与本实例 pbu(21085) 一致（历史坑）。

### 7.7 委托链路排查（2026-11-11，FTE Debug + 方向1）

#### 7.7.1 FTE 以 `-l Debug` 启动的结论

| 尝试 | 结果 |
|------|------|
| 手工 `ute … -l Debug`（含 ATP_DATA/ATP_CFG、cd 到 `etf_test_sh/run`） | **启动即退出**（exit 0），日志仅 2 行（`Application start to initialize` + `signal_thread start`） |
| `start_all.sh` 临时把 `-l Info` → `-l Debug` | 同样启动即退出（33001 不监听） |
| 恢复 `-l Info` | 正常（33001 监听、登录/心跳可用） |

> **结论**：本版本 FTE 在 `-l Debug` 下无法启动（既有问题，与本次改动无关）；脚本已还原（`start_all.sh.bak_dbg` 已移除）。→ 拿 FTE 内部委托日志需改用"源码提级日志 + 重编"，本轮未做。

#### 7.7.2 方向1（pbu 三方一致性）核对结果

| 位置 | 值 | 结论 |
|------|----|------|
| 模拟交易所 `mock_exch.xml` `<pbu>` | 原 **00000000** → 改 **21085** | **不一致 → 已对齐**（原文件备份 `mock_exch.xml.bak_pbu_20261111`） |
| FTE `etf_test_sh/cfg/ute.xml` `pbu_id` | 21085（SSESTEP 段 offer_way=2） | 基准 |
| `mock_sqlserver` 启动参数 | `--pbu 21085 --setid 1 --fill` | 一致 |
| 账户上场数据 `account_ute_61.bin`（bin_tool 查 1000000000000001） | `trade_pbu=21085 offer_pbu=21085 assignment=2 partition=61` | **一致** |

**对齐后重跑**：登录 ✅ / 心跳 ✅，委托仍 ❌（等待回报超时）。

#### 7.7.3 加临时探针后的决定性证据（API 侧）

在 `gw_counter_direct::deal_order_req`（入队后）与 `single_socket_engine`（发送点）加临时探针：

```
DIAG_ORDER enqueue side=1,order_type=2,market=101,qty=10000,price=250200,seq=0
DIAG_SEND  evt_type=0,data_len=118,ret=118      ← 118 = 8(头)+106(TradeOrderReq)+4(校验和)
```

> **委托已正确组包并 100% 写出到 FTE（ret=118）**，因此"未发出"可排除。

#### 7.7.4 当前断点：FTE 收到 1003 后无任何处理痕迹

- FTE 今日日志：无 `600007`、无 `execreport` 写库记录、无 `GetStrategyRspQueue Fail` 错误、无拒单日志；
- FTE 为 **NO_DSE 构建**（日志 `NO_DSE mode: started DSE message drop thread`），STEP 链路（SSESTEP/39144）已连通（`ClearDatabase`/`SyncReport next=1` 持续运行）。
- 推测：NO_DSE 下委托进入业务分发后无回传通道（或 strategy 应答队列缺失）而被静默丢弃。

**下一步（需 FTE 侧证据）**：在 `tcpserver_handler` / `ccu_trade_impl` / `strategy_trade_busi` 的 1003 分支加 `LOG_INFO`，重编 FTE（约 5 分钟）后复跑，即可判定是"未分发"还是"分发后丢弃"；或改用 DSE 栈（`start_dse_stack.sh`，需部署 `domain_server`+`dse` 并设大页）。

### 7.8 FTE/DSE/模拟交易所 体系学习与 DSE 栈打通（2026-11-11）

#### 7.8.1 学到的体系（源：`gt_trunk/DYS-FRAMEWORK/fte/test_all/dse_stack/README.md` 等）

**组件关系**：`domain_server`(AMI 配置中心+仲裁器, 5221/5222) → `dse`(数据同步引擎) ↔ 共享内存 SPSC 双队列 `ute_dse`/`dse_ute` ↔ `fte`(交易引擎，**必须 `-DFTE_WITH_DSE=ON`，即不定义 NO_DSE**)；上位机为 `mock_exch_server`(38140/38141) + `mock_sqlserver`(39144, `--fill` 出成交回报)。

**启动顺序（严格）**：mock 交易所 → domain_server（含 `-i` 导入配置，Context 数应 **730**）→ 等 10s → dse → 等 10s → fte（对象池初始化慢，约 **90s** 后建共享内存通道）。

**FTE 三个必须参数**：`-n UTE_61_611_11`（沪）/`UTE_84_842_21`（深）、`--domain-server {127.0.0.1:5222}`、mock 交易所先起；否则出现"进程活着但客户端连不上（local_port: 0）"或 SIGBUS。

**环境前置**：大页 `nr_hugepages=140`（不足会告警并降级）、`igmp_max_memberships≥200`、清理 `/dev/shm/{dse_ute,ute_dse}*` 残留。

**联通判据**：DSE 日志 `ute_dse share memory queue attach successed`；FTE 日志 `dse_ute share memory queue attach successed`；`/dev/shm` 有 6 个文件（含 lock）。

#### 7.8.2 本轮实际打通情况

| 步骤 | 结果 |
|------|------|
| `make ute dse`（FTE_WITH_DSE=ON） | ✅ 编译通过（**需补容器兼容依赖**，见下） |
| domain_server 启动 + 配置导入 | ✅ 5221/5222 监听，**Context 730** |
| dse 启动 | ❌ `exit=157`，仅 1 行日志（`DseEngineMDProcess Init`），未生成日志文件 |
| 历史 dse（`work_atp/ninja/fte/bin/dse`，glibc≤2.14） | ❌ 缺 `libabf_te.so`（全盘不存在），不可用 |

**dse 编译依赖适配（关键经验，README §6.1 的落地）**：
1. 链接缺 `libabf_te/abf_te_rzrq/abf_te_third/abf_md/abf_crc/abf_detail/abf_grisk/abf_query` → 建 `DSE_PREBUILT_LIB_DIR=/mnt/work/gt_test/work_atp/dse_deps`，用 `ar rcs lib<name>.a` 建 **8 个空归档**满足 `-l`；配置时该目录会被 `link_directories(BEFORE)` 采纳。
2. 运行时缺 `libabf_util.so`/`libabf_tech.so`/`libmsg_proto.so`/`libbiz_msg.so` → 从 `work_atp/cmake/{common,tech,msg_proto}/lib` 拷入 dse_deps；
   ⚠️ **`libmsg_proto.so` 必须选低版本**：`work_atp/cmake/msg_proto/lib` 的为 `GLIBCXX_3.4.11`（可用），`gcc_11/`、`test_time_lsz/` 的需 3.4.29（容器只有 3.4.19，不可用）。
3. 以上完成后 `ldd dse` 无缺失（missing=0）。

#### 7.8.3 当前结论与下一步

- **正解确认**：gw 委托必须有 DSE 联通（NO_DSE 构建下委托静默丢弃，与 §7.7.4 观察一致）。
- **卡点**：dse 运行时 `exit=157`（配置/Context/依赖均已核对），需进一步：① `LD_DEBUG=libs`/`gdb` 抓退出点；② 对照 gt_svn 仓的 abf 运行库（`libabf_te.so` 等）补齐；③ 或联系 DSE 维护方确认 157 码含义。
- **环境已恢复可用态**：`run_trunk/ute` 回退为 NO_DSE 版（登录/心跳可测），DSE 版另存 `run_trunk/ute_dse_version`；`run_trunk/dse` 保留以备继续调试。

### 7.9 全组件重编/部署/联调（2026-11-11 第二轮，容器内）

#### 7.9.1 已完成

| 环节 | 动作 | 结果 |
|------|------|------|
| 重编 | `make ute dse -j8`（FTE_WITH_DSE=ON 配置） | ✅ `[100%] Built target dse`；ute 亦为 DSE 版 |
| 依赖适配 | `DSE_PREBUILT_LIB_DIR=/mnt/work/gt_test/work_atp/dse_deps`：8 个 stub 归档 + 真实 `libabf_util/tech/biz_msg` + **低版本 `libmsg_proto`(GLIBCXX_3.4.11)** | ✅ `ldd dse` missing=0 |
| 部署 | `run_trunk/{dse,ute_dse_version,dse.xml}`、配置目录 `etf_test_sh/cfg/{dse.xml,domainserver_98.config,ute.xml}` | ✅ |
| domain_server | 启动 + `-i` 导入 | ✅ 5221/5222、**Context=730**、etcd 可查到 `Instance/DSE_61_611_11` 与 `Context/DSE_61_611_11` |
| 模拟交易所 | mock_exch 38140/38141 + mock_sqlserver 39144(--pbu 21085 --fill) | ✅ |

#### 7.9.2 dse 启动仍失败（唯一卡点）

| 尝试 | 退出码 / 现象 |
|------|---------------|
| `./dse -n DSE_61_611_11 --domain-server {127.0.0.1:5222} -l Info --log-dir ./log_dse_sh` | **exit=157**，仅 1 行日志（`DseEngineMDProcess Init`），不生成日志文件 |
| 加 `-D /dev/shm/dse_sh` | exit=**176** |
| 加 `-E ${AMI}/bin` | exit=**176** |
| 两者都加 | exit=**121** |
| dse.xml 放 cwd | exit=157（无变化） |
| 移除 dse.xml 的 `UserName/UserPasswd` | exit=157（无变化） |
| 历史 dse（`work_atp/ninja/fte/bin/dse`，glibc≤2.14） | 缺 `libabf_te.so`（全盘不存在，gt_svn/gt_sit 也没有） |

**已排除**：缺依赖库、dse.xml 路径、Context 不存在、实例名不匹配、大页不足（已 140）、igmp 上限、shm 残留、DS 端口不通。
**gdb 证据**：`catch exit_group` 显示 `main` **正常 return**（非崩溃），即 AMI 主流程主动返回上述码；AMI 库中确有 `Get config for context <{1}> failed.` 等错误串，但 dse 未输出对应日志。

#### 7.9.3 结论与建议

- **gw 委托必须有 DSE 联通**（README §4.4 明确；NO_DSE 下委托静默丢弃，与本项目实测一致）。故 DSE 不通 ⇒ 委托/成交/撤单 3 个用例无法通过。
- 当前环境已回退到"可登录 + 可心跳"的 NO_DSE 组合（回归：登录 PASS、心跳 PASS、委托/成交/撤单 FAIL）。
- 下一步需要外部输入（三选一）：① AMI/DSE 的 **ExitCode 表**（157/176/121 含义）；② gt_svn 或运维侧的 **已验证 dse 部署包**（含 `libabf_te.so` 等运行库）；③ 由 DSE 维护方确认 dse 在本容器（CentOS7/glibc2.17/gcc4.8.5）下的启动参数。

---

## 附录：复现命令

```bash
cd trunk/NewAPI/gone/api
# 现存待确认/todo（38 处）
grep -rInE "待确认|\btodo\b|TODO" --include="*.cpp" --include="*.h" src include counter98 | wc -l
# 按文件分布
grep -rInE "待确认|\btodo\b|TODO" --include="*.cpp" --include="*.h" src include counter98 \
  | awk -F: '{print $1}' | sort | uniq -c | sort -rn
# 无标记风险抽查
sed -n '1860,1870p' src/counter98.cpp      # err_code 折叠 / cancel_qty 不钳制
grep -n "map_fte_exec_type" -A 4 src/gw_counter_direct.cpp
```

---

### 6.7 订正记录（2026-11-10 18:20）

- **订正内容**：§6.2 原 B-2「`fill_er_common` 取错字段（用 `er.orig_clordno`）→ 撤单必失败」**判断有误**。复核当前代码（文件时间 11-10 17:38）：
  - `fill_er_common()`：`out.order_sys_no = er.clordno;`（委托/成交回报，正确）
  - `deal_cancel_rsp()`：`out.order_sys_no = er.orig_clordno;`（撤单回报回被撤原单号，正确）
  - `build_api_cancel_rej()`：`msg.orig_clordno`（API 层拒绝时回显原单号，正确）
- **根因**：我沿用了 11:36 时读到的 `git diff` 快照（当时作者确实写作 `er.orig_clordno`），此后作者已于 17:38 修正，而我在 18:07 的分析中**未重新读取文件内容**，导致结论过期。
- **影响**：阻断级由 5 项更正为 **4 项**（B-1/B-3/B-4/B-5）；"最小修复集"由「B-2+B-1」更正为「B-1 + 环境准备」。
- **已同步**：§0 摘要分级、§2 已闭环表第 5 行、§6.2/§6.3/§6.5/§6.6 及知识库 §45.7、`Prompt.md` 第 38 条、工作日志。

---
