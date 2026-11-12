# NewAPI 字典映射分析与对齐方案（2026-11-10）

> **状态**：✅ **已实施**（2026-11-10）：两个 P0 已修复、API 字典已按 98 CTE 补齐、clean 编译通过。实施明细见 §5。
> **目标**：① 核对 API 本地字典与 98 CTE 字典的一致性/缺口；② 比对 98 CTE 与 FTE 柜台字典，给出 `gw_counter_direct.cpp` 各请求/回报字段的映射转换方案。
> **字典源（三方）**：
> - API 本地：`trunk/NewAPI/gone/api/include/order_trade_type.h`（`lb_api` 命名空间 `static constexpr`）
> - 98 CTE：`gt_trunk/DYS-FRAMEWORK/gw_external_msg/V2/external/include/gw/external/codec/constants_agw_external.h`（`atp::gw::Xxx` 结构体 + 同名 `.cpp` 赋值）
> - FTE：`gt_trunk/DYS-FRAMEWORK/fte/include/common/ute_types.h`（`common::Xxx` 结构体，取值见 `src/common/ute_types.cpp`）
> **解析口径**：`.h` 内 `static const T kX = v;` 与 `.cpp` 内 `const T X::kX = v;` 合并（CTE 部分取值只在 `.cpp`）；API 按名字前缀归域。复现命令见附录 B。

---

## 0. 结论摘要

### 0.1 六个关键结论

| # | 结论 | 严重度 |
|---|------|:------:|
| **C1** | ✅**已修复**｜**`gw_counter_direct.cpp::map_fte_ord_status()` 用错了字典**：它按「0 空闲 / 1 新报 / 2 部成 / 3 全成 / 4 撤中 / 5 全撤 / 8 废」转换，但 FTE **回报消息里的 `ord_status` 字段类型是 `OrdStatus_def`（`common::OrdStatus`）**，其取值与 CTE/API 同义（0 新报 / 1 部分成交 / 2 全部成交 / 3 部成部撤 / 4 已撤销 / 5 部分撤销 / 8 拒绝 / 9 未申报 / 10 正报 / 11 待撤销 / 12 部成待撤）。**当前转换除 0、8 外全部错位**（例如 FTE 回「部分成交 1」会被当成「已申报 0」） | **P0** |
| **C2** | ✅**已修复**｜**`order_type` 不能直通 FTE**：FTE 的委托类型是**按市场两套字典**（上海 `SHOrderType` / 深圳 `SZOrderType`），同字符不同义。尤其 **SZ 限价在 FTE 是 `'1'`**，而 API 限价是 `'2'` → 直通后 SZ 限价单会被 FTE 解释为「本方最优」 | **P0** |
| **C3** | ✅**已处置**｜**API 委托类型字典与 CTE 存在同值不同义**：API `'1'` = 最优五档即时成交剩余撤销，CTE `'1'` = 市价。另有 5 个 API 类型（`'3'~'7'`）在 CTE 无对应，CTE 的 `'U'`（本方最优）/`'K'`（市价剩余转限价）在 API 无对应 | 高 |
| **C4** | **API 把「业务类型」折叠进了 `side`**（`S/T/U/V/W/X` 等），而 98 CTE 与 FTE 都是 **`side` + `business_type` 两维**表达（如融资买入 = side `'1'` + business_type `201`）。同一业务在两套模型里的承载位置不同，是 98/个微映射的主要坑 | 高 |
| **C5** | **委托状态（OrdStatus）在三方完全一致**（API 11 值 = CTE 11 值；FTE 多 `13 kProcessed`、`99 kNull`），**可直接直通**——前提是先修掉 C1 的错误转换 | 中 |
| **C6** | ✅**已补齐**｜API **缺两类字典**：`time_in_force`（字段存在但无字典常量，CTE 有 4 值）与 `business_type`（字段存在但无字典常量，CTE 有 54 值 / FTE 20 值） | 中 |

### 0.2 要求一统计总表（API vs 98 CTE）

| 域 | API 条数 | CTE 条数 | 同值同义 | **同值不同义** | **API 缺**（CTE 有） | **API 多**（CTE 无） | 判定 |
|----|:--------:|:--------:|:--------:|:--------------:|:--------------------:|:--------------------:|------|
| 市场 Market | 4 | 6 | 4 | 0 | **2**（105/106 港股通沪/深） | 0 | 基本对齐，缺港股通 |
| 委托类型 OrdType | 7 | 4 | 1 | **1**（值 `'1'`） | **2**（`U`/`K`） | **5**（`3`~`7`） | **需双向映射** |
| 买卖方向 Side | 16 | 41 | 10 | 0 | **31** | **6**（`S/T/U/V/W/X`） | 结构不同（折叠业务） |
| 委托状态 OrdStatus | 11 | 11 | 11 | 0 | 0 | 0 | **完全一致** |
| 执行类型 ExecType | 5 | 6 | 5 | 0 | **1**（`J` 黄金） | 0 | 缺 `J` |
| 业务类型 BusinessType | **0（无字典）** | 54 | — | — | **54** | — | **缺整套字典** |
| 订单有效期 TimeInForce | **0（无字典）** | 4（`0/3/4/9`） | — | — | **4** | — | **缺整套字典** |
| 证券类型 SecurityType | 71 | 0（CTE 无此域） | — | — | — | — | 98 侧不校验，与 FTE 对齐见 §3.1 |

> 合计（**分析时**）：**同值不同义 1 处（P0 级）**、**API 缺项 89 项**（2+2+31+0+1+54+4，含两套整字典）、**API 多余 11 项**（5+6）。
>
> **补齐后（2026-11-10）**：API 缺项 **89 → 0**——已补 `MARKET_TYPE_HKT_SH/SZ`、`ORDER_SIDE_*`（+31，含 5 项与 `ORDER_TYPE_*` 值相同的独立侧别）、`ORDER_TYPE_98_*`（4）、`RSP_TYPE_GOLD`、`BUSINESS_TYPE_*`（54）、`TIME_IN_FORCE_*`（4）。**`ORDER_TYPE_*` 主字典保持 ATP 7 值不变**（原因见 §5.2：与 CTE `'1'` 值冲突，无法合并为同一命名空间）。

---

## 1. 要求一：API 本地字典 vs 98 CTE 字典

### 1.1 市场 Market

| 值 | API | CTE `AgwMarketID` | 一致 |
|:--:|-----|-------------------|:----:|
| 0 | `MARKET_TYPE_NULL` 空 | `kNULL` 空 | ✅ |
| 101 | `MARKET_TYPE_SH` 上交所 | `kShangHai` 上海 | ✅ |
| 102 | `MARKET_TYPE_SZ` 深交所 | `kShenZhen` 深圳 | ✅ |
| 109 | `MARKET_TYPE_BSE` 北交所 | `kThird` 北京 | ✅ |
| 105 | — | `kHKTShangHai` 港股通-沪 | ❌ 缺 |
| 106 | — | `kHKTShenZhen` 港股通-深 | ❌ 缺 |

**结论**：已定义 4 项与 CTE 完全同值同义；**缺 105/106（港股通）**。若后续支持港股通，需补 `MARKET_TYPE_HKT_SH/HKT_SZ`。

> 注：`market_type` 在 API 配置项里是 **int8**（`api_config.cpp known_attrs`），值域上界 127 → 101/102/109 可容纳，105/106 亦可；但 FTE 侧还有 103/107/108/110/111/112/199（见 §3.1），API 目前无法表达。

### 1.2 委托类型 OrdType ★（唯一"同值不同义"）

| 值 | API | CTE `AgwOrdType` | 判定 |
|:--:|-----|------------------|------|
| `'1'` | `ORDER_TYPE_JDSC` 最优五档即时成交剩余撤销 | `kMarket` **市价** | ⚠️ **同值不同义** |
| `'2'` | `ORDER_TYPE_FIX` 限价 | `kFixed` 限价 | ✅ |
| `'3'` | `ORDER_TYPE_JDSZ` 最优五档即时成交剩余转限价 | — | ❌ CTE 无 |
| `'4'` | `ORDER_TYPE_BFZY` 本方最优 | —（CTE 用 `'U'`） | ❌ 值不同 |
| `'5'` | `ORDER_TYPE_DFZY` 对手方最优 | — | ❌ CTE 无 |
| `'6'` | `ORDER_TYPE_FAK` 即时成交剩余撤销（深） | — | ❌ CTE 无 |
| `'7'` | `ORDER_TYPE_FOK` 全额成交或撤销（深） | — | ❌ CTE 无 |
| `'U'` | — | `kLocalOptimal` 本方最优 | ❌ API 无 |
| `'K'` | — | `kTransferFixed` 市价剩余转限价（沪） | ❌ API 无 |

**结论**：API 是「交易所差异化 7 值统一字典」，CTE 是「98 业务化 4 值字典」，二者**不是同一套**。98 侧转换口径（与 `开发者升级说明.txt` 一致，且与 CTE 值域相符）：

| API → 98 CTE | 说明 |
|--------------|------|
| `'2'` 限价 → `'2'` kFixed | 唯一一一对应 |
| `'4'` 本方最优 → `'U'` kLocalOptimal | 值不同需转换 |
| `'3'` 五档剩转限价 → `'K'` kTransferFixed | 仅沪市语义 |
| `'1'`/`'5'`/`'6'`/`'7'` → `'1'` kMarket（市价） | **语义收窄**：98 无对应精细类型，只能当市价；**建议记录原始类型并在回报中原样回传**（当前 API 只能在 `OrderReq` 中携带来回） |

> ⚠️ 风险：`'5'` 对手方最优 → 市价 `'1'` 会让柜台以市价成交，**与客户预期（对手方最优价）不符**，需业务确认是否允许降级，或直接返回 `LBAPI_ERR_UNSUPPORTED_TYPE`。

### 1.3 买卖方向 Side

- **API 有 16 项**：`0/1/2/3/4/9/D/E/H/I`（与 CTE 同值同义，10 项）+ **`S/T/U/V/W/X`（CTE 完全没有）**。
- **CTE 有 41 项**，API 缺 31 项（`5/6/7/8/@/%/A/C/F/G/J/K/M/O/P/Q/R/Y/Z/a~l`）。

| API 独有 | 含义 | CTE 承载方式（建议） |
|----------|------|----------------------|
| `S` | 配股认购（注释标"独立，非98、个微字典"） | 98：side `'9'`(kCall) 或 business_type `14`(kSubscriptionRatio) —— **待确认** |
| `T` | 担保品买入 | 98：business_type `203`(kCollateralInOrOut) + side `'1'` —— **待确认** |
| `U` | 担保品卖出 | 同上 + side `'2'` —— **待确认** |
| `V` | 融资买入 | 98：business_type `201`(kMarginTrading) + side `'1'` |
| `W` | 融券卖出 | 98：business_type `201` + side `'2'` |
| `X` | 卖券还款 | 98：side `'g'`(kActivedSellRepay) 或 business_type `204`(kBuySecuRepay) —— **待确认** |

**结论（C4）**：API 把上述业务折叠进 `side`，而 98 用 **side + business_type** 两维表达。因此 98 映射必须"拆解"：`side → (side', business_type)`；反向回报则需"合并"：`(side', business_type) → side`。

### 1.4 委托状态 OrdStatus ✅ 完全一致

| 值 | API | CTE | |
|:--:|-----|-----|---|
| 0 | `ORDER_STATE_NEW` | `kNew` | ✅ |
| 1 | `ORDER_STATE_PART_FILLED` | `kPartiallyFilled` | ✅ |
| 2 | `ORDER_STATE_FILLED` | `kFilled` | ✅ |
| 3 | `ORDER_STATE_PART_FILLED_PART_CANCEL` | `kPartiallyFilledPartiallyCancelled` | ✅ |
| 4 | `ORDER_STATE_CANCELED` | `kCancelled` | ✅ |
| 5 | `ORDER_STATE_PART_CANCELED` | `kPartiallyCancelled` | ✅ |
| 8 | `ORDER_STATE_REJECTED` | `kReject` | ✅ |
| 9 | `ORDER_STATE_UNSENT` | `kUnSend` | ✅ |
| 10 | `ORDER_STATE_SENDING` | `kSended` | ✅ |
| 11 | `ORDER_STATE_WAIT_CANCEL` | `kWaitCancelled` | ✅ |
| 12 | `ORDER_STATE_PART_FILLED_WAIT_CANCEL` | `kPartiallyFilledWaitCancelled` | ✅ |

**结论**：11/11 完全一致，98 侧可直通（`counter98.cpp` 现无转换即正确）。FTE 多 `13`/`99` 的处理见 §3.1。

### 1.5 执行类型 ExecType

| 值 | API | CTE | |
|:--:|-----|-----|---|
| `'0'` | `RSP_TYPE_NEW` | `kNew` | ✅ |
| `'1'` | `RSP_TYPE_INTERNAL` | `kInternal` | ✅ |
| `'4'` | `RSP_TYPE_CANCELED` | `kCancelled` | ✅ |
| `'8'` | `RSP_TYPE_REJECTED` | `kReject` | ✅ |
| `'F'` | `RSP_TYPE_TRADE` | `kTrade` | ✅ |
| `'J'` | — | `kGold` 黄金 | ❌ 缺 |

**结论**：5/5 同值同义；仅缺 `'J'`（黄金相关）。建议补 `RSP_TYPE_GOLD='J'` 或明确归入"未识别"分支。

### 1.6 业务类型 BusinessType（API 无字典）

- `common_struct.h` 多个回报结构有 `uint8_t business_type` 字段，但 `order_trade_type.h` **没有对应常量字典**。
- CTE `AgwBusinessType` 有 **54** 项（`0 全部 / 1 现金拍卖 / 2 质押 / 3 债券分销 / 5 大宗 / 6 盘后定价 / 7 融资 / 9 现金质押式回购 / 10 约定式回购 / 12 ETF 实时申赎 / 13 发行 / 14 配股 / 15 债券回售 / 16 期权行权 / 17 开放式基金申赎 / 18 要约收购 / 20 转托管 / 21 投票 / 22 协议交易 / 23 权证行权 / 24 密码服务 / 27 转处置 / 28 券种转换 / 29 兑付 / 31 分级基金实时拆分合并 / 32 备兑锁定解锁 / 34 期权组合 / 63 港股通 / 97 盘后固定价 / 99 信用交易 / 200~218 两融与三方业务等）。
- FTE `BusinessType` 20 项（`1 现金拍卖 / 5 大宗 / 6 盘后定价 / 12 ETF / 13 发行 / 14 配股 / 15 债券回售 / 97 / 200~213` 等）。

**建议**：新增 `lb_api::BUSINESS_TYPE_*` 字典（先覆盖 98/FTE 都在用的子集），并把 `business_type` 纳入 `OrderRtn/TradeRtn` 的对外契约说明。

### 1.7 订单有效期 TimeInForce（API 无字典）

| 值 | CTE `AgwTimeInForce` | API |
|:--:|----------------------|-----|
| `'0'` | `kGFD` 当日有效 | 无常量（`common_struct.h` 仅有 `char time_in_force` 字段） |
| `'3'` | `kIOC` 即时成交剩余撤销 | 无 |
| `'4'` | `kFOK` 全额成交或撤销 | 无 |
| `'9'` | `kAtCrossing` 集合竞价 | 无 |

**建议**：补 `TIME_IN_FORCE_*` 字典；`OrderReq` 若需下发 TIF，需与"委托类型已含 IOC/FOK 语义"去重（避免两处同时表达冲突）。

---

## 2. 要求二：98 CTE vs FTE 字典 + `gw_counter_direct` 映射

### 2.1 CTE 与 FTE 的域差

| 域 | CTE | FTE | 差异 | 对映射的影响 |
|----|-----|-----|------|-------------|
| Market | 6（0/101/102/105/106/109） | **13**（另加 103 香港、107/108 B 股、110 基金、111/112 银行间、199 港股通） | FTE 多 7 值 | FTE 回报可能给出 API 无定义的市场值 → 需映射表 + 兜底 |
| OrdStatus | 11 | **13**（多 `13 kProcessed`、`99 kNull`） | FTE 多 2 | `13/99` 需定义兜底（建议视为未知并保留原值） |
| ExecType | 6（`0/1/4/8/F/J`） | **7**（多 `'K' kMock`） | FTE 多 1 | `'K'`（模拟撮合）需映射 |
| Side | 41 | **47**（多 `kNULL`、`kPricingBuy/Sell`、`kIntentionBuy/Sell`、`kEnhanceBuy/Sell`、`kOddShareSell`；少 `kDisposal/kDisposalReturn`） | 值集不同 | 两融/定价/意向/增持/零股等侧别仅 FTE 有；API 的 `S/T/U/V/W/X` 两方都无 → 必须拆解为 side+business_type |
| BusinessType | 54 | 20 | CTE 多 36（质押/回购/期权/基金/转托管/投票/兑付/港股通/三方等） | FTE 只支持子集，ETF 用 `12` |
| **OrdType（委托类型）** | **1 套 4 值**（`'1' 市价 / '2' 限价 / 'U' 本方最优 / 'K' 市价剩转限价`） | **2 套**：`SHOrderType`（`1 五档IOC/2 限价/3 五档剩转限价/4 本方最优/5 对手方最优`）+ `SZOrderType`（`1 限价/2 本方最优/3 对手方最优或限价/4 IOC/5 FOK/6 五档FOK`） | **字典组织方式不同，且 FTE 按市场分表** | **本次最关键的转换点**（见 2.2） |
| TimeInForce | 4 | 无独立字典（SZ 由 order_type 推导 TIF/价格档位，见 `fte_to_sz_order_type`） | — | 个微侧 TIF 由委托类型隐含，不单独传 |

### 2.2 ★ FTE 委托类型：同字符、按市场不同义

`gt_trunk/.../fte/src/business/uplink_biz_processor.cpp`：

- **上海**（`FillOrderFieldsSH`）：`exch_sh_order.ord_type = trade_order_info->order_type;` —— **直接透传**，要求入站 `order_type` 已是 SH 交易所码。
- **深圳**（`FillOrderFieldsSZ`）：`auto& tmp = fte_to_sz_order_type[order_type - SZOrderType::kBegin];` —— 用 **`SZOrderType` 值域**查表转换（`kLimited→{'0','2',0}`、`kLocalOptimal→{'0','U',0}`、`kCounterpartyOptimalOrLimited→{'0','1',1}`、`kImmediateOrCancel→{'3','1',0}`、`kFillOrKill→{'3','1',0}`、`kFiveLevelFillOrKill→{'3','1',5}`）。

| 字符 | 上海 `SHOrderType` 含义 | 深圳 `SZOrderType` 含义 | 同值是否同义 |
|:----:|------------------------|------------------------|:------------:|
| `'1'` | 最优五档即时成交剩余撤销 | **限价** | ❌ |
| `'2'` | **限价** | 本方最优 | ❌ |
| `'3'` | 最优五档即时成交剩余转限价 | 对手方最优或限价 | ❌ |
| `'4'` | 本方最优 | 即时成交剩余撤销(IOC) | ❌ |
| `'5'` | 对手方最优 | 全额成交或撤销(FOK) | ❌ |
| `'6'` | —（`kEnd`） | 五档全额成交或撤销 | — |

### 2.3 ★ API 委托类型 → FTE 映射表（按市场）

| API 值/含义 | 上海(101) 目标 | 深圳(102) 目标 | 备注 |
|-------------|:--------------:|:--------------:|------|
| `'1'` 最优五档即时成交剩余撤销 | `'1'` ✅ 直通 | **无对应**（SZ 无"五档 IOC"；`'4'` 是非五档 IOC、`'6'` 是五档 FOK） | 建议：拒绝（`LBAPI_ERR_UNSUPPORTED_TYPE`）或降级 `'4'`（**须业务确认**） |
| `'2'` 限价 | `'2'` ✅ | **`'1'`（必须转换）** | 直通会把限价单变成"本方最优"——**P0** |
| `'3'` 最优五档即时成交剩余转限价 | `'3'` ✅ | 无对应 | 建议拒绝（该类型本身是沪市专用语义） |
| `'4'` 本方最优 | `'4'` ✅ | **`'2'`（必须转换）** | |
| `'5'` 对手方最优 | `'5'` ✅ | **`'3'`（对手方最优或限价，有语义偏差）** | 需业务确认是否接受 |
| `'6'` 即时成交剩余撤销(深) | 无对应 | **`'4'`** | 沪市建议拒绝 |
| `'7'` 全额成交或撤销(深) | 无对应 | **`'5'`** | 沪市建议拒绝 |

> **实现建议**：新增
> ```cpp
> // order_type: API(ATP 7值统一字典) → FTE(按市场 SH/SZ 两套)
> char map_api_order_type_to_fte(char api_type, uint16_t market_id, bool& supported);
> ```
> `supported=false` 时由调用方返回 `LBAPI_ERR_UNSUPPORTED_TYPE`（与 `开发者升级说明.txt` 中"FPGA/个微不支持的类型返回不支持"口径一致）。

### 2.4 ★ API `side` → FTE `side` + `business_type`

| API side | FTE `Side` 值 | FTE `BusinessType` 建议 | 说明 |
|----------|---------------|--------------------------|------|
| `'1'` 买 / `'2'` 卖 | `'1'` / `'2'` | `1`(kCashAuction) | 普通现金竞价，直通 |
| `'4'`/`'3'` 逆回购/正回购 | `'4'`(kAntiRepo) / `'3'`(kRepo) | `9`/`10`/`11`（按回购品种） | 98 侧另有专职业务，需与柜台确认 |
| `'9'` 认购 | `'9'`(kCall) | `13`(kIssue) / `211`(kThirdIssue) | 按发行类型 |
| `'D'` 申购 / `'E'` 赎回 | `'D'` / `'E'` | **`12`(kETF / CTE kETFRealTimePurchaseRedemption)** | ETF 申赎走 1010 消息；LOF/开放式基金另议 |
| `'H'`/`'I'` 质押入库/出库 | `'H'` / `'I'` | `203`(kCollateralInOrOut) | |
| `'V'` 融资买入 / `'W'` 融券卖出 | `'1'` / `'2'` | **`201`(kMarginTrading)** | API 折叠 → FTE 需拆开 |
| `'X'` 卖券还款 | `'g'`(kActivedSellRepay) | `204`(kBuySecuRepay) | 二者择一需确认（FTE 两套值都存在） |
| `'S'` 配股认购 | `'9'`(kCall)？ | `14`(kSubscriptionRatio) | **待确认** |
| `'T'`/`'U'` 担保品买入/卖出 | `'1'` / `'2'` | `203` | **待确认** |

> **取值来源**：上表 business_type 数值在 98 CTE 与 FTE 两侧**同名同值**（`kMarginTrading=201`、`kCollateralInOrOut=203`、`kBuySecuRepay=204`、`kSubscriptionRatio=14`、ETF=`12`），可跨端共用一份常量。
>
> 现状：`body.side = req.side;`（透传）。对 `'1'/'2'/'D'/'E'/'H'/'I'` 成立；对 **`S/T/U/V/W/X` 会直接发错**（FTE 不认识这些字符，`Side::IsValid` 会拒单）。

### 2.5 各报文的字典字段映射清单

| 报文（`gw_counter_direct`） | 字典字段 | 方向 | 现状 | 需要 |
|---------------------------|----------|------|------|------|
| `build_login_msg`（1001） | `heart_bt_int`、无字典 | 出 | ✅ | — |
| `build_order_msg`（1003） | `market_id` / `side` / `order_type` | 出 | 直通 | **`order_type` 按 §2.3 转换；`side` 按 §2.4 拆解；`market_id` 校验值域** |
| `build_etf_order_msg`（1010） | 同上 + 隐含 business_type=ETF | 出 | 直通 + `order_type` 固定 `'2'` 的 todo | 确认 ETF 是否固定限价；`side` 必须 `'D'/'E'` |
| `build_cancel_msg`（1004） | 无字典（三个号码） | 出 | ✅ | 保持 |
| `deal_order_rtn`（2003） | `ord_status` / `exec_type` / `market_id` / `ord_type` / `side` | 入 | `map_fte_ord_status()` **错误**；其余直通 | **`ord_status` 改直通（删除错误转换）**；补充 `exec_type='J'/'K'`；`market_id` 越界值处理 |
| `deal_trade_rtn`（2005） | `exec_type` / `market_id` / `side` | 入 | 直通 | 同上 |
| `deal_cancel_rsp`（2004） | `ord_status` / `exec_type` | 入 | 同上 | 同上 |
| `deal_reject_msg`（9） | `reject_reason_code` | 入 | 直通（仅打印） | 建议补 `AgwBizRejCode` 描述映射，便于排障 |
| `deal_etf_trade_rtn`（2010） | `exec_type` / `ord_status` | 入 | 同上 | 同上 |

### 2.6 现有 `map_*` 函数逐条评估

| 函数（`gw_counter_direct.cpp`） | 现状 | 判定 | 建议 |
|-------------------------------|------|------|------|
| `map_fte_market_to_api()`（≈L153） | `return fte_market;` 直通 | ⚠️ 对 0/101/102/109 正确；FTE 的 103/107/108/110/111/112/199 会溢出 API 字典 | 加白名单 + 非法值降级（保留原值并打 WARN，或映射 NULL） |
| `map_fte_ord_status()`（≈L160） | 按"空闲/新报/部成/全成/撤中/全撤/废"转换 | ❌ **错误（C1）** | **删除转换，直通**（FTE `OrdStatus` 与 API 同义）；另对 FTE `13/99` 定义兜底 |
| `map_fte_exec_type()`（≈L184） | 直通 | ✅ 正确（`'0'/'1'/'4'/'8'/'F'` 同义）；缺 `'J'`/`'K'` | 补 `'J'`/`'K'` 处理 |
| `body.order_type = req.order_type`（1003/1010） | 直通 | ❌ **错误（C2）** | 按 §2.3 加市场相关映射 |
| `body.side = req.side` | 直通 | ⚠️ 部分正确（C4） | 按 §2.4 拆解 + `business_type` |

### 2.7 工作区现状说明（2026-11-10，作者就地手改 3 处）

分析期间工作区已被作者修改 3 个文件（**非本文产出**），本文结论基于修改后的状态：

| 文件 | 改动 | 与本文的关系 |
|------|------|-------------|
| `include/api_interface.h` | 注释：卖券还款 `X` 由"待确认，暂不支持"改为"还是走非加速模式" | 与 §1.3 `X` 的 side 映射口径相关（98/FTE 仍需映射，见 §2.4） |
| `include/common_struct.h` | 注释：`transact_time` 精度由 "todo" 改为 "统一取纳秒" | 消除 §1.7 之外的一处字段语义待办 |
| `src/gw_counter_direct.cpp` | 行为：`out.order_sys_no = er.orig_clordno`（原为 `fte_order_sys_no(er)`） | 影响 §2.5 撤单三号码/原单定位；`orig_clordno` 字段存在于 `TradeOrderER`（`gw_head.h:849`），增量编译通过 |

> 上述 3 处改动已在本地 `./build.sh Release -DBUILD_MOCK=ON` 增量编译通过（四目标 `Built target`）。

---

## 3. 优先级与建议改动

| 优先级 | 事项 | 位置 | 影响 | **状态** |
|:------:|------|------|------|:--------:|
| **P0-1** | 删除 `map_fte_ord_status()` 的错误转换，改为直通（保留 13/99 兜底） | `gw_counter_direct.cpp` ≈L160 | 否则**所有回报的委托状态全错**，客户状态机必然错乱 | ✅ 已实施（直通 + 13/99 兜底） |
| **P0-2** | 新增 `map_api_order_type_to_fte(api_type, market_id, &supported)`，按 §2.3 转换 | 同上 `build_order_msg`/`build_etf_order_msg` | 否则**深市限价单被当本方最优**发出去 | ✅ 已实施（SH 5 值 / SZ 5 值 + 不支持即拒） |
| **P1-1** | `side` 拆解：新增 `map_api_side_to_fte(api_side, &fte_side, &business_type, &supported)` | 同上 | 否则两融/担保品/配股/卖券还款等 **`S/T/U/V/W/X` 全部被拒单** | ⏸ 未实施（`deal_order_req` 现返回 UNSUPPORTED_OP） |
| **P1-2** | `exec_type` 补 `'J'`(黄金)/`'K'`(模拟) 分支 | `map_fte_exec_type` | 否则未知值透传进 API 字典 | ◑ 已补字典 `RSP_TYPE_GOLD='J'`；映射分支未加 |
| **P1-3** | `market_id` 值域校验（FTE 13 值 vs API 4 值） | `map_fte_market_to_api` | 否则 API 层出现未定义市场值 | ⏸ 未实施 |
| **P2-1** | 补 API 字典：`BUSINESS_TYPE_*`、`TIME_IN_FORCE_*`、`RSP_TYPE_GOLD`、`MARKET_TYPE_HKT_SH/SZ` | `order_trade_type.h` | 补齐对外契约，减少 magic number | ✅ 已实施（+96 常量） |
| **P2-2** | 98 侧：`ord_type` 7 值→4 值映射（`3→K`、`4→U`、`1/5/6/7→1`）与 `side` 折叠业务拆解 | `counter98.cpp`（现有 `todo 待确认` 处） | 与本文 §1.2/§1.3 口径一致 | ⏸ 未实施（字典已备，待联调确认） |
| **P2-3** | 拒绝原因码字典（`AgwBizRejCode` / FTE `RejectReasonCode`）补描述映射，便于排障 | 98/gw 双端 | 提升可运维性 | ⏸ 未实施 |

> 说明：P0 两项为**正确性缺陷**（会导致业务错误而非仅仅"不精确"），建议优先修复并补单测。

---

## 4. 验证方法

1. **表驱动单测**（不依赖柜台）：把 §2.3/§2.4 的映射表写成数据驱动用例，对 `map_api_order_type_to_fte` / `map_api_side_to_fte` / `map_fte_ord_status` 逐项断言（含 `supported=false` 的拒绝项）。
2. **回报状态回放**：用构造的 `TradeOrderER`（写 `ord_status=1/2/3/5/10/12`）验证 `deal_order_rtn` 输出 `ORDER_STATE_PART_FILLED/FILLED/...`，可直接证明 P0-1 修复。
3. **联调用例**（需 FTE）：沪深各发 限价/本方最优/对手方最优/IOC/FOK 一笔，抓包核对 FTE 侧 `ord_type` 是否符合 `SHOrderType`/`SZOrderType`。
4. **98 侧**：按 §1.2/§1.3 的映射表，与柜台逐项确认 `待确认` 标号项。

---

---

## 5. 实施记录（2026-11-10）

### 5.1 代码改动

| 文件 | 改动 | 行数 |
|------|------|------|
| `gone/api/src/gw_counter_direct.cpp` | ① `map_fte_ord_status()` 改为**按协议字典直通**（0~12 原值返回），仅对 FTE 内部值 `13 已处理`/`99 无` 兜底；② 新增 `map_api_order_type_to_fte(api_type, market_id, &o_fte_type)`（SH 5 值 / SZ 5 值，无对应返回 false）；③ `deal_order_req` 在入队前校验委托类型，不支持返回 `LBAPI_ERR_UNSUPPORTED_TYPE`；④ `build_order_msg` 写入映射后的 FTE 码；⑤ `build_etf_order_msg` 由"硬编码 `'2'`"改为**按市场取限价码**（SH `'2'` / SZ `'1'`） | +79 / −25 |
| `gone/api/include/order_trade_type.h` | 按 98 CTE 补齐字典：`MARKET_TYPE_HKT_SH/SZ`(105/106)、`ORDER_SIDE_*`(+31)、`ORDER_TYPE_98_*`(4)、`RSP_TYPE_GOLD`('J')、`BUSINESS_TYPE_*`(54)、`TIME_IN_FORCE_*`(4) | +96 常量（136 → 232） |

### 5.2 两点设计取舍（需知悉）

1. **`ORDER_TYPE_*` 主字典保持 ATP 7 值不动**：CTE `AgwOrdType` 的 `'1'` 是"市价"，而 ATP `'1'` 是"最优五档即时成交剩余撤销"，**同值不同义**，无法并入同一命名空间。故新增 `ORDER_TYPE_98_*`（4 值）专供 98 报文转换，两套字典各自标注来源。
   > 若要求"主字典改为 CTE 语义"，属**对外契约变更**（客户侧 `OrderReq.order_type` 传值含义改变），需单独决策与通知，不宜在本次静默切换。
2. **ETF 委托类型不再硬编码 `'2'`**：原注释"ETF 不区分市价类型，固定填 '2'"在 FTE 按市场分表后是错的——SZ 的 `'2'` 是"本方最优"。现统一走 `ORDER_TYPE_FIX` → 按市场取限价码。
3. **不支持的类型显式拒绝**：`'6'`/`'7'`（FAK/FOK）在沪市、`'1'`/`'3'`（五档 IOC / 五档剩转限价）在深市无对应，`deal_order_req` 直接返回 `LBAPI_ERR_UNSUPPORTED_TYPE`（该错误不触发 98 回退，与 `开发者升级说明.txt` 口径一致）。

### 5.3 验证

| 项 | 结果 |
|----|------|
| 编译 | `./build.sh Release -DBUILD_MOCK=ON`：`lbcommon` / `g1api` / `perf_client` / `mock_client` 四目标全部 `Built target`，0 error、0 告警（我方文件） |
| 字典覆盖 | 脚本核对：`ORDER_SIDE_` 41/41、`MARKET_TYPE_` 6/6、`RSP_TYPE_` 6/6、`BUSINESS_TYPE_` 54/54、`TIME_IN_FORCE_` 4/4、`ORDER_TYPE_98_` 4/4 → **缺口 0** |
| 常量卫生 | 232 个常量**无重名**；`ORDER_SIDE_*` 扩到 47 项（41 CTE ∪ 6 ATP 扩展） |
| 未验证（需柜台） | 与 FTE 的实报文往返：SZ 限价/本方最优/对手方最优、SH 五档类、ETF 申赎；98 侧 `side` 拆解与 `ord_type` 7→4 映射 |

### 5.4 后续（本次未做）

- P1-1 `side` 拆解（`S/T/U/V/W/X` → side + business_type）：现仍由 `deal_order_req` 返回 `UNSUPPORTED_OP`。
- P1-2 `exec_type` 映射补 `'J'`/`'K'` 分支（字典已备）。
- P1-3 `market_id` 值域校验（FTE 13 值 vs API 4 值）。
- P2-2 98 侧 `ord_type` 7→4 映射与 `side` 折叠业务拆解（`counter98.cpp` 的 `todo 待确认`）。
- P2-3 拒绝原因码描述映射。

---

## 附录 A：关键字典全量（对照用）

### A.1 CTE `AgwOrdType`（4）
`'1'` 市价 / `'2'` 限价 / `'U'` 本方最优 / `'K'` 市价剩余转限价（沪）

### A.2 FTE `SHOrderType`（5）/ `SZOrderType`（6）
- SH：`'1'` 五档即时成交剩余撤销 / `'2'` 限价 / `'3'` 五档即时成交剩余转限价 / `'4'` 本方最优 / `'5'` 对手方最优
- SZ：`'1'` 限价 / `'2'` 本方最优 / `'3'` 对手方最优或限价 / `'4'` 即时成交剩余撤销 / `'5'` 全额成交或撤销 / `'6'` 五档全额成交或撤销

### A.3 CTE `AgwSide`（41）摘要
`'0'` 所有、`'1'` 买、`'2'` 卖、`'3'` 正回购、`'4'` 逆回购、`'5'` 转股、`'6'` 回售、`'7'` 要约收购、`'8'` 解除要约、`'9'` 认购、`'A'` 激活、`'C'` 注销、`'D'` 申购、`'E'` 赎回、`'F'` 融资、`'G'` 融券、`'H'` 入库、`'I'` 出库、`'J'` 转托管、`'K'` 转托管撤销、`'M'` 合并、`'O'` 开仓、`'P'` 实物申购、`'Q'` 查询、`'R'` 实物赎回、`'Y'` 解锁、`'Z'` 锁定、`'@'` 处置、`'%'` 处置回转、`'a'` 转入、`'b'` 转出、`'c'/'d'/'e'/'f'` 增补平仓/清仓/强制卖出系列、`'g'` 卖券还款、`'h'` 回售撤销、`'i'/'j'` 现金/股票追缴、`'k'/'l'` 约定购回买入/卖出

### A.4 FTE `Side`（47）在 CTE 基础上新增
`' '` kNULL、`'m'/'n'` 定价买/卖、`'o'/'p'` 意向买/卖、`'q'/'r'` 增持买/卖、`'s'` 零股卖出

### A.5 委托状态（CTE/FTE/API 三方对照）
`0` 新报 / `1` 部分成交 / `2` 全部成交 / `3` 部成部撤 / `4` 已撤销 / `5` 部分撤销 / `8` 拒绝 / `9` 未申报 / `10` 正报 / `11` 待撤销 / `12` 部成待撤（FTE 另 `13` processed、`99` null）

### A.6 执行类型
`'0'` 新 / `'1'` 内部 / `'4'` 撤销 / `'8'` 拒绝 / `'F'` 成交（CTE 另 `'J'` 黄金；FTE 另 `'K'` 模拟）

---

## 附录 B：复现命令（字典提取与差分）

```bash
# 1) CTE / FTE / API 三方字典提取（Python，解析 struct kXxx 与 constexpr）
python3 - <<'PY'
import re, json, pathlib
CTE_H="/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/gw_external_msg/V2/external/include/gw/external/codec/constants_agw_external.h"
CTE_C="/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/gw_external_msg/V2/external/src/gw/external/codec/constants_agw_external.cpp"
FTE_H="/home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/include/common/ute_types.h"
API_H="/home/lsz/code/work/api_trunk/trunk/NewAPI/gone/api/include/order_trade_type.h"
def structs(path, hdr):
    t=pathlib.Path(path).read_text(errors="replace"); out={}
    for m in re.finditer(hdr, t, re.M):
        s=m.end(); n=t.find("\nstruct ", s); blk=t[s: n if n>0 else len(t)]
        items={i.group(2):(i.group(4) or "").strip() for i in re.finditer(r'static const (\w+) (\w+)(\s*=\s*([^;]+))?;', blk)}
        if items: out[m.group(1)]=items
    return out
cte=structs(CTE_H, r'^struct EXPORT_MACRO_DEFINE (\w+)\s*$')
for m in re.finditer(r'^const (\w+) (\w+)::(\w+)(\s*=\s*([^;]+))?;', pathlib.Path(CTE_C).read_text(errors="replace"), re.M):
    cte.setdefault(m.group(2),{})[m.group(3)]=(m.group(5) or "").strip() or None
fte=structs(FTE_H, r'^struct (\w+)\s*$')
api={m.group(2):m.group(3).strip() for m in re.finditer(r'static constexpr (\w+) (\w+)\s*=\s*([^;]+);', pathlib.Path(API_H).read_text())}
print("CTE:", len(cte), "FTE:", len(fte), "API:", len(api))
print("AgwOrdType", cte["AgwOrdType"]); print("SHOrderType", fte["SHOrderType"]); print("SZOrderType", fte["SZOrderType"])
PY

# 2) FTE 委托类型使用点（确认 SH 直通 / SZ 查表）
grep -n "exch_sh_order.ord_type = trade_order_info->order_type" \
     /home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/src/business/uplink_biz_processor.cpp
grep -n "fte_to_sz_order_type\[" \
     /home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/src/business/uplink_biz_processor.cpp

# 3) FTE 回报 ord_status 字典（证明与 CTE 同义）
grep -n "struct OrdStatus" -A 16 /home/lsz/code/work/gt_trunk/DYS-FRAMEWORK/fte/include/common/ute_types.h
```

---
