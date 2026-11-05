// c98_session_cache - 98 柜台专属的账户会话缓存 (counter98 持有)
//
// counter98 成员 (每 api 实例一份), 以 fund_account_id 为主键, 缓存 98 账户登录的
// 会话信息与组包字段。个微柜台另有独立缓存 (gw_login_cache.h), 两柜台互不共享。
// 设计契约 (与个微柜台缓存拆分, RUL-5):
//   1. 每账户一条 C98SessionInfo, 内嵌 log_98 登录状态机 (log_state_ctl) ——
//      状态/重复/在途控制 (幂等回调/在途合并, 三场景语义见 api_event_msg.h);
//   2. map 的插入只发生在 98 引擎线程消费 ACCOUNT_LOGIN 事件的 deal_cust_login
//      (不存在则插入, 存在则走状态机幂等/合并), 除 clear_all 外无删除 ——
//      节点指针稳定, get_session 返回的指针长期有效;
//   3. 字段读取线程模型: 组包 (用户线程) 经 get_session 指针直读 —— 组包前置门控
//      (log_98 is_logined, acquire) 与登录应答置 OK (CAS, release) 构成发布屏障;
//      身份字段 (cust/account/branch) 按此屏障直读, 成立前提 "发布后不再写" 由
//      fill_session_login_ok 值不变不写保证 (RUL-15, 成立条件 = 柜台侧账户身份
//      恒等); password/user_info/order_way_ext 可被
//      update_session_info 在盘中改密重登时刷新 (引擎线程, 锁内写, RUL-14), 用户线程
//      组包读取须经 copy_pack_password / copy_pack_credentials (锁内快照) 取值。

#pragma once

#include "api_event_msg.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

namespace lb_api
{

  /// 98 账户会话信息
  struct C98SessionInfo
  {
    log_state_ctl log_98;                       ///< 98 柜台登录控制状态机 (原子, 状态+在途)
    std::array<char, 16> fund_account_id;       ///< 资金账号（主键, 空格定长）
    std::array<char, 16> cust_id;               ///< 客户号（从登录应答回填）
    std::array<char, 12> account_id;            ///< 股东账户（从登录应答回填）
    std::array<char, 10> branch_id;             ///< 分支机构代码
    std::array<char, 2> order_way_ext;          ///< 客户委托方式
    std::array<char, 64> user_info;             ///< 用户私有信息
    std::array<char, 256> password;             ///< 客户登录密码（登录请求缓存, 供组包回填）
    std::array<char, 1024> client_feature_code; ///< 客户终端信息（组包回填用）
    int64_t last_cust_req_no = 0;               ///< 最近一次登录请求号（登录事件/回调关联用）

    void reset()
    {
      log_98.log_state = 0;
      fund_account_id.fill(' ');
      cust_id.fill(' ');
      account_id.fill(' ');
      branch_id.fill(' ');
      order_way_ext.fill(' ');
      user_info.fill(' ');
      password.fill('\0');
      client_feature_code.fill('\0');
      last_cust_req_no = 0;
    }

    C98SessionInfo() { reset(); }
  };

  /// 98 会话缓存管理器 (counter98 成员)
  /// 键规范 (NUL 截止 + 裁尾空格) 为内部实现细节, 全部接口直接接收原始账户标识
  /// (C 串/定长数组), 外部不做任何键转换。
  class C98SessionCache
  {
  public:
    C98SessionCache() = default;
    ~C98SessionCache() = default;
    C98SessionCache(const C98SessionCache &) = delete;
    C98SessionCache &operator=(const C98SessionCache &) = delete;

    /// 插入/覆盖会话 (upsert, 与个微 prepare_login 同构):
    ///   不存在 → 完整构建信息插入 (登录请求字段全量, log_98 为构造初值 NONE);
    ///   已存在 → 仅刷新凭据类字段, cust_id/account_id/branch_id 保持上次
    ///     fill_session_login_ok 的值 (身份字段以 800006 应答为权威源), log_98 不动
    void create_session(const acc_login_event_info &login_req);

    /// 获取会话 (找不到返回 nullptr)。节点从不删除 (clear_all 除外), 指针长期有效;
    /// 字段读取线程模型见文件头注释 §3; 状态机为原子字可跨线程直接调用。
    C98SessionInfo *get_session(const char *fund_account_id, size_t len);

    /// 登录应答回填: 98 ATP 账户登录应答 (cust_id/branch_id/account_id, C 串源, 空值不覆盖)
    void fill_session_login_ok(C98SessionInfo *sess, const char *cust_id, const char *branch_id,
                               const char *account_id);

    /// 更新会话的登录凭据字段 (盘中改密后用户重登时刷新; 状态机与身份字段不动),
    /// 并同步 last_cust_req_no —— 后续 build_fast_counter_login_event 按会话该字段
    /// 关联回调请求号, 不刷新则重试/二次登录回调用旧号
    void update_session_info(C98SessionInfo *sess, const acc_login_event_info &login_req);

    /// 委托/撤单组包快照 (用户线程 98 委托/撤单组包调用): password/order_way_ext/user_info 与
    /// update_session_info 的凭据刷新互斥 (RUL-14 后 order_way_ext 盘中可变), 锁内读取;
    /// user_info 取登录缓存原值 (RUL-29)
    void copy_pack_password(const C98SessionInfo *sess, std::array<char, INNER_PASSWORD_LEN> &o_password,
                            std::array<char, 2> &o_order_way_ext,
                            std::array<char, INNER_USER_INFO_LEN> &o_user_info);

    /// 组包凭据快照 (用户线程 98 查询组包调用): password/user_info 均与
    /// update_session_info 的改密刷新互斥, 读取必须在锁内
    void copy_pack_credentials(const C98SessionInfo *sess, std::array<char, INNER_PASSWORD_LEN> &o_password,
                               std::array<char, INNER_USER_INFO_LEN> &o_user_info);

    /// 清除全部账户在途 (98 断链: 只清在途不清状态)
    void clear_all_pending();

    /// 清除所有会话（慎用: 仅实例销毁/全量复位场景）
    void clear_all();

  private:
    /// 凭据类字段刷新 (事件为权威源: 委托方式/私有信息/密码/终端信息/请求号)
    static void copy_credentials(C98SessionInfo &info, const acc_login_event_info &ev)
    {
      memcpy(info.order_way_ext.data(), ev.order_way_ext, info.order_way_ext.size());
      memcpy(info.user_info.data(), ev.user_info, info.user_info.size());
      memcpy(info.password.data(), ev.password, info.password.size());
      memcpy(info.client_feature_code.data(), ev.client_feature_code, info.client_feature_code.size());
      info.last_cust_req_no = ev.cust_req_no;
    }

    /// 规范键提取: 遇 NUL 截止, 裁剪尾部空格 (C 串源与空格定长源归一为同键) —— 内部细节
    static std::string norm_key(const char *p, size_t max_len)
    {
      size_t n = 0;
      while (n < max_len && p[n] != '\0')
        ++n;
      while (n > 0 && p[n - 1] == ' ')
        --n;
      return std::string(p, n);
    }

    /// 值不变不写 (RUL-15): 空格填充目标与现值逐字节比较, 不同才写 —— 恒等回填
    /// 零写入, 维持"发布(置 OK)后身份字段不再写"的屏障前提 (KNOWN-1 源②关闭;
    /// 屏障成立条件 = 柜台侧账户身份恒等, 见文件头 §3; 与个微 fill_if_changed 同构)
    template <size_t N>
    static void fill_if_changed(std::array<char, N> &dst, const char *src)
    {
      if (src == nullptr || src[0] == '\0')
        return; ///< 空值不覆盖 (既有语义)
      std::array<char, N> tnew;
      size_t n = 0;
      while (n < N && src[n] != '\0')
      {
        tnew[n] = src[n];
        ++n;
      }
      for (size_t i = n; i < N; ++i)
        tnew[i] = ' ';
      if (tnew != dst)
        dst = tnew;
    }

    std::mutex mutex_;
    std::unordered_map<std::string, C98SessionInfo> sessions_;
  };

  // ====== 内联实现 ======

  inline void C98SessionCache::create_session(const acc_login_event_info &login_req)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = norm_key(login_req.fund_account_id, sizeof(login_req.fund_account_id));
    auto ret = sessions_.emplace(key, C98SessionInfo{}); ///< 不存在则插入 (构造 reset 初值, log_98=NONE)
    C98SessionInfo &info = ret.first->second;
    if (ret.second || !info.log_98.is_logined())
    {
      // 不存在: 完整构建插入 (登录请求字段全量, 身份字段以事件值为初值)
      // 未登录成功也完整构建，防止用户第一次输入错误，以便第二次输出正确能更新
      memcpy(info.fund_account_id.data(), login_req.fund_account_id, info.fund_account_id.size());
      memcpy(info.cust_id.data(), login_req.cust_id, info.cust_id.size());
      memcpy(info.account_id.data(), login_req.account_id, info.account_id.size());
      memcpy(info.branch_id.data(), login_req.branch_id, info.branch_id.size());
      memcpy(info.password.data(), login_req.password, info.password.size());
    }
    // 已存在: cust_id/account_id/branch_id 保持上次 fill_session_login_ok 的值,
    // log_98 状态机不动; 仅刷新凭据 (原整节点替换会清掉状态机与应答身份, 已消除)
    copy_credentials(info, login_req);
  }

  inline C98SessionInfo *C98SessionCache::get_session(const char *fund_account_id, size_t len)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(norm_key(fund_account_id, len));
    if (it != sessions_.end())
      return &it->second;
    return nullptr;
  }

  inline void C98SessionCache::fill_session_login_ok(C98SessionInfo *sess, const char *cust_id,
                                                     const char *branch_id, const char *account_id)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    fill_if_changed(sess->cust_id, cust_id);
    fill_if_changed(sess->branch_id, branch_id);
    fill_if_changed(sess->account_id, account_id);
  }

  inline void C98SessionCache::update_session_info(C98SessionInfo *sess, const acc_login_event_info &login_req)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    copy_credentials(*sess, login_req); ///< 凭据类字段统一走 copy_credentials (增刷 order_way_ext)
  }

  inline void C98SessionCache::copy_pack_password(const C98SessionInfo *sess,
                                                  std::array<char, INNER_PASSWORD_LEN> &o_password,
                                                  std::array<char, 2> &o_order_way_ext,
                                                  std::array<char, INNER_USER_INFO_LEN> &o_user_info)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    o_password = sess->password;
    o_order_way_ext = sess->order_way_ext;
    o_user_info = sess->user_info;
  }

  inline void C98SessionCache::copy_pack_credentials(const C98SessionInfo *sess,
                                                     std::array<char, INNER_PASSWORD_LEN> &o_password,
                                                     std::array<char, INNER_USER_INFO_LEN> &o_user_info)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    o_password = sess->password;
    o_user_info = sess->user_info;
  }

  inline void C98SessionCache::clear_all_pending()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &kv : sessions_)
    {
      kv.second.log_98.clear_pending();
    }
  }

  inline void C98SessionCache::clear_all()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.clear();
  }

} // namespace lb_api
