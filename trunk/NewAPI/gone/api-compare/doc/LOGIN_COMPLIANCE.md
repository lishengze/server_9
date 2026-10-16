# 登录需求实现状态矩阵

> **对照需求**：[REQUIREMENTS.md](REQUIREMENTS.md) **§3.2**（log_state_ctl 状态机：三场景 + 重登失败终局 + 登录失败不关链 + 换号拒绝）
> **代码基线**：[api/src/](../src/) 当前实现（个微柜台已知问题按约定暂不处理——附注 8③）
> **图例**：✅ 已实现 ｜ ◐ 部分实现（受协议占位/已知偏差限制，见附注） ｜ ❌ 缺口
> **速查**：状态字典/在途标记定义见 [api_event_msg.h](../src/api_event_msg.h)；三场景与重连矩阵详见 [ARCHITECTURE.md](ARCHITECTURE.md) §6.2/§6.4

## 1. §3.2.0 登录状态模型（核心不变量）

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| 状态字典 `LOGIN_STATE_NONE/LOGOUT/OK`，判定一律 `==OK / !=OK` | ✅ | [api_event_msg.h](../src/api_event_msg.h) 常量 + 全柜台判定常量化 |
| 登录成功状态仅能被用户主动登出改变（换账户一律拒绝） | ✅ | 写者仅：登录成功应答（save_client_info / deal_log_ans）、登出（deal_logout_req to_logout） |
| 链接断开/重连/建立不清除任何**客户**登录状态 | ✅ | 各柜台 deal_link_close 仅清在途位（clear_pending），客户状态位不动；**agwuser 例外**：agw_login_state 断链复位（会话与 98 链路绑定，§3.2.1），曾成功标志 have_agw_ok_ 保持 |
| 断链清除该链接全部登录在途标记（作废请求无登录回调） | ✅ | 98 步骤无在途缓存、无可清；direct：单客户；gateway：遍历 clients_vec_；个微：自身 |
| FPGA 下线为独立故障维度，不触碰登录状态 | ✅ | `deal_fpag_state` 仅置 fpga_state（+on_event+direct 关 core） |
| 98 map val 即登录状态，成员永不因登出移除 | ✅ | `cust_login_map_` val 0/1/2；`mark_cust_login_ok`=→OK、`deal_logout_req`（无成员→NOT_LOG_CUST / 有成员→LOGOUT）、`have_cust_login`=val==OK |
| 用户登出不删除客户信息 | ✅ | gateway 客户对象/索引保留；98 map 成员保留 |

## 2. §3.2.1/3.2.2 分级登录与三场景

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| 步骤① agwuser：已成功跳过（同步幂等） | ✅ | `counter98::deal_agw_login`（api_impl::login 第一步） |
| 步骤② 98 客户：val==OK 跳过直达柜台步骤 | ✅ | `counter98::deal_login_req` 分级跳过 |
| **98 步骤不设在途缓存** | ✅ | 98 账户一旦成功登录永不再次登录；重复请求的消化依赖柜台步骤状态机（S1 在途合并 / S2 幂等 / 迟到失败忽略 val==OK）。**注意**：每个 98 成功应答均投递柜台登录事件（98 层未做去重），重复成功应答的第二个事件命中 S2 幂等会**再次回调成功**（见附注 7） |
| 步骤③ 柜台三场景（状态机） | ✅ | 三柜台 `deal_cust_login` 统一调用 `log_state_ctl.to_login(o_havelog, user_origin)`：幂等回调/合并不发/占位发送 |
| S2 幂等=登录成功即回调成功（**不看链路**） | ✅ | to_login 用户分支第一优先判定 state==OK → o_havelog=1 |
| FPGA 下线时 login 本地降级成功（98 降级） | ✅ | direct/gateway `deal_cust_login` 判定 `fpga_state==2` → 本地降级成功回调（不发包、状态位不变、清在途）；已成功客户亦命中该分支回成功；恢复后由用户重新 login 走正常 FPGA 登录 |
| 内部重登（S3）：重连直投事件（资格预查 is_logined&&非下线），在途控制统一在柜台 to_login 内部分支（置 RELOGIN、状态保持） | ✅ | direct/gateway deal_link_connect 直投 `delive_login_event`；个微 `delive_relogin_event` |
| **fpga 直连与个微均不支持换账户**：绑定过账户（含在途/已登出）后异账户登录拒绝（NOT_LOG_CUST + 失败回调，不发后台）；首登不受限 | ✅ | direct：client_info_ 账户非空且不同；个微：last_login_event_ 不同且 !is_free()（gateway 多客户天然共存） |
| gateway 首登并发安全 | ◐ | 客户对象在**首个成功应答**时入库（deal_log_ans，入库前无互斥载体，并发首登的重复 98 请求各自发送）；重复成功应答各自投递柜台登录事件，第二个命中 S2 幂等再次回调成功（单发单回调不严格成立，见附注 7）。残留边界：新客户登录在引擎同步阶段失败（connect/发送失败）时对象未建，ans_cust_login 仅告警无用户回调（待正式协议联测评估） |

## 3. §3.2.3 登录回调语义（按发起方）

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| 用户发起必回调：成功经状态机回调一次 | ✅ | `log_state.ans_login_ok()==1` → on_login 成功（三柜台统一） |
| 失败回调（USER/RELOGIN 统一）：清状态(→NONE)+回调+不再重登+不关链 | ✅ | `log_state.ans_login_err()==1` → on_login 失败（deal_log_ans） |
| 用户发起失败→清状态+回调 | ✅ | 同上 USER 分支；引擎同步失败（ans_cust_login）同语义 |
| 无在途迟到应答忽略 | ✅ | take==NONE 分支 warning 日志 |
| 回调例外（①合并一次回调 / ②断链作废无登录回调） | ✅◐ | ①由 S1 合并承载、②由断链清除承载（用户文档需明示）；①在"98 重复成功应答 / gateway 新客户并发首登"场景不成立（附注 7） |
| 回调归因按（资金账号+分支机构），不新增来源字段 | ✅ | LoginAns 回显账户字段 |
| login_cb_flag 退役 | ✅ | 字段已删除，全部引用清除 |

## 4. §3.2.4 用户主动登出

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| logout()/on_logout() 接口与回调全链路 | ✅ | api_interface / order_trade_type / api_callback + api_impl::logout + callback_manager::on_logout |
| 清除 98 登录状态（val→LOGOUT，成员不移除） | ✅ | `counter98::deal_logout_req`（map 有成员 → val→LOGOUT） |
| 清除柜台登录状态 + 按模式关链（direct 关 core / gateway 不关 GW / 个微关链） | ✅ | 三柜台 `deal_logout_req` |
| 不影响 agwuser 登录状态 | ✅ | logout 路径不触碰 agw_login_state |
| 不向柜台发协议消息（纯本地，同步完成） | ✅ | 无组包/队列/网络动作 |
| 从未成功登录过（98 map 无成员）→ **登出失败**（同步返回 NOT_LOG_CUST，无回调） | ✅ | `counter98::deal_logout_req`（两层编排；有成员含已登出 → val→LOGOUT 成功） |
| 已成功登录过 + **用户在途**登录 → 拒绝（STATE_LIMITED，无回调） | ✅ | 各柜台 `deal_logout_req` → `log_state_ctl.to_logout`（用户在途返回 <0） |
| 已成功登录过 + **重连在途**登录 → 允许登出 | ✅ | to_logout 仅拒用户在途；置 LOGOUT 并清在途位，迟到重登应答按 LOGOUT 忽略 |
| 登出编排顺序（**柜台层 → 98 层 → 回调**） | ✅ | api_impl::logout 依次调 fast_.deal_logout_req → c98_.deal_logout_req → on_logout；柜台层拒绝（STATE_LIMITED/NO_CUST）时 98 状态尚未变更；柜台层成功 ⇒ 必经 98 登录成功（map 必有成员），98 层失败仅防御 |
| fpga direct 登出账户匹配 | ✅ | 仅"已绑定账户"做匹配拒绝；client_info_ 未绑定（offline 降级/FPGA 登录失败）→ to_logout 幂等成功，与 gateway"未入库→成功"对称 |
| 个微登出账户匹配 | ❌ | deal_logout_req 未校验请求账户（代码内 todo）：异账户 logout 会登出**绑定账户**并关个微链（**个微柜台问题，按约定暂不修复**，附注 8③） |
| 98 agwuser 不支持登出（登出不影响 agwuser 登录） | ✅ | logout 路径不触碰 agw_login_state |

## 5. §3.2.5 断线重连与重新登录矩阵

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| 98 重连：曾成功登录过（have_agw_ok_）→ 重发 agw 登录（断链时 agw_login_state 已复位；CAS 占位与用户线程互斥）；已成功 98 客户不重登 | ✅ | counter98 deal_link_connect（重发期间 98 业务按 NOT_LOG_AGW 本地拒绝——与窗口期"不拦截"的表述差异见附注 3） |
| fpga GW 重连：无条件重取证券信息 | ✅ | direct/gateway deal_link_connect（SEC_INFO_REQ） |
| fpga GW 重连：`is_logined() && fpga_state!=2` 客户重登（S3，状态保持） | ✅ | 资格预查 + `delive_login_event` 直投（柜台 to_login 内部分支控制） |
| gateway"至少一个客户重登"以投递成功为准；全下线/全登出豁免；应登>0 且全投递失败才关链；**have_switch=1（地址切换）时任一投递失败即关链** | ✅ | gateway deal_link_connect 重登循环（need_count/re_count + have_switch 判定） |
| fpga core 重连门：`login_state==OK && fpga_state!=2` 方可重连，无登录动作 | ✅ | fpga_counter_direct can_link_connect(TRADE)（登录状态保持使 GW 断链期间 core 可重连） |
| 个微重连：`login_state==OK` 自动重登 | ✅ | gw_counter_direct deal_link_connect + delive_relogin_event |
| 内部重登被柜拒绝（终局）：清状态+回调+不再自动重登+不关链 | ✅ | `ans_login_err()` RELOGIN 分支（状态机内） |
| 窗口期原则：链接建立至重登成功之间业务放行、后台明确拒绝 | ◐ | 无窗口期拦截逻辑（依赖后台契约，见附注 3） |
| FPGA 下线→恢复闭环（core 自动重拨 + GW 重连自动重登） | ✅ | fpga_state 恢复推送 + can_link_connect 门 + 重登门 |

## 6. §3.2.6 业务门控

| 条款 | 状态 | 实现位置 / 说明 |
|---|---|---|
| 委托/撤单校验：柜台客户登录状态==OK + 链接状态 | ✅ | 三柜台 deal_order_req/deal_cancel_req（判定常量化） |
| 降级矩阵：仅 COUNTER_OFFLINE（下线）/UNSUPPORTED_OP 降级 98；断链硬失败 | ✅ | api_impl 降级判定 + 柜台错误码 |
| 98 业务登录前置：链接 + agw 登录 | ◐ | check_req_precond 现生效 LINK_DISCONNECTED / NOT_LOG_AGW；have_cust_login（账户登录成功过）检查为注释 todo——待正式 98 协议到位后确认是否启用 |

## 7. 附注（已知限制、缺口与依赖）

1. **协议占位**：98 协议（c98msg_tmp.h）与个微业务消息为占位——登录/登出的**状态机与事件链**完整实现，98 账户登录消息、个微业务消息待正式协议补齐后仅需替换组包/解析层（个微委托/撤单当前仅发送 g1 消息头、消息体留空）。
2. **fgw 侧联动**：①"同连接不可换号"不变式由 API 侧单客户模式拒绝换号保证；②后台契约确认"曾成功客户重连重登不拒 + board/user 当日不变"——API 内部重登被拒按终局处理（清状态+回调+不再重登，不关链）。
3. **窗口期原则依赖后台明确拒绝**：链接建立至重登成功之间放行的业务请求，需后台以明确应答拒绝（不静默丢弃）——待联测确认。注意：98 侧本地前置检查（check_req_precond 的 LINK_DISCONNECTED / NOT_LOG_AGW）当前**生效**——agw 重登窗口内 98 业务被本地拒绝，与"不做本地拦截"的表述存在差异（have_cust_login 检查为注释 todo），正式协议到位时统一。
4. **98 map 幽灵成员**（换号拒绝的副产物）：异账户 login 在柜台步骤被拒，但其 **98 步骤已先行执行并可能成功**——98 map 留下该异账户的 OK 成员；无行为影响（该账户再 login 仍被柜台拒绝），显式 logout 可清理。
5. **建议联测场景**：S1 并发合并 / 断链作废在途（四链路）/ 重登失败终局（清状态不重登）/ 登出三模式（含 offline 降级账户与在途拒绝）/ 下线→恢复闭环 / 异账户登录拒绝（direct+个微）/ board/user 变化异常路径（fgw 重启重分配 → 回调失败 + 在途消费恢复）/ agw 重登窗口期 98 业务本地拒绝行为。
6. **模板共享库符号约束**：`aio_socket_link` / `tcpdir_link` / `link_timer_op` 及各引擎模板的显式实例化必须置于成员函数定义所在编译单元（各 .cpp 末尾），否则 .so 运行期符号缺失。
7. **重复成功应答未在 98 层抑制**：`counter98::deal_cust_login_ans` 对每个成功应答都投递柜台登录事件（不做"仅首个投递"去重）。常规时序下（同账户并发登录被柜台步骤 S1 合并）仅回调一次；但 98 重复成功应答 / gateway 新客户入库前并发首登场景下，第二个柜台登录事件命中 S2 幂等会**再次回调成功**——与"同账户并发登录仅回调一次"的需求存在偏差（重复回调均为成功，无失败误报）。待正式 98 协议联测评估是否在 98 层补抑制。
8. **登出路径**：①编排顺序为柜台层先行（拒绝时 98 状态不变）；②fpga direct 未绑定账户登出幂等成功（不做匹配拒绝）；③个微未校验请求账户（异账户 logout 登出绑定账户+关链）——**个微柜台问题，按约定暂不修复**，正式个微协议落地时一并处理。
