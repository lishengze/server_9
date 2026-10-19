// c98_session_cache - 98 柜台专属的账户登录缓存 (counter98 持有)
//
// counter98 成员 (每 api 实例一份), 以 fund_account_id 为主键, 与个微柜台缓存
// (gw_login_cache.h) 互不共享 (RUL-5)。数据条目复用 GwSessionInfo (字段宽度与
// counter98 build_*_msg 的目标协议字段一致: branch_id 10B / account_id 12B /
// cust_id 16B)，避免 api-compare 版 C98SessionInfo 的 branch_id 12B 与 98 协议
// 字段不兼容导致的编译/组包错位。
//
// 设计契约:
//   1. 条目在登录事件处理时创建/凭据暂存 (deal_cust_login 经 create_session,
//      upsert 语义: 已存在仅刷新凭据, 保留身份字段), 条目永不删除 —— 节点指针
//      稳定, 组包经指针直读;
//   2. 身份字段 (cust/account/branch) 以 98 登录成功应答 (deal_cust_login_ans)
//      为权威源回填 (fill_session_login_ok), 值不变不写 (RUL-15) 维持发布屏障;
//   3. 断链仅清全部条目在途 (clear_all_pending), 登录状态保持, 重连后按已登录
//      条目批量自动重登 (collect_relogin_events)。

#pragma once

#include "api_event_msg.h"
#include "gw_session_cache.h" // GwSessionInfo / OrderLocator

#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace lb_api
{

  /// 98 账户登录缓存管理器 (counter98 成员)
  /// 键规范 (NUL 截止 + 裁尾空格) 为内部实现细节, 全部接口直接接收原始账户标识。
  class C98SessionCache
  {
  public:
    C98SessionCache() = default;
    ~C98SessionCache() = default;
    C98SessionCache(const C98SessionCache &) = delete;
    C98SessionCache &operator=(const C98SessionCache &) = delete;

    /// 登录事件处理时的凭据暂存 (upsert): 不存在 → 完整构建插入; 已存在 → 仅刷新
    /// 凭据, 身份字段 (cust/account/branch) 保持上次 fill_session_login_ok 的值。
    /// 返回条目指针 (长期有效)。
    GwSessionInfo *create_session(const acc_login_event_info &login_req);

    /// 获取条目 (找不到返回 nullptr)
    GwSessionInfo *get_session(const char *fund_account_id, size_t len);

    /// 登录成功应答回填 (cust_id/branch_id/account_id, C 串源, 空值不覆盖)。
    void fill_session_login_ok(GwSessionInfo *info, const char *cust_id, const char *branch_id,
                               const char *account_id);

    /// 收集已登录账户的自动重登事件 (user_origin=0, 内部重登)
    void collect_relogin_events(std::vector<acc_login_event_info> &out);

    /// 清除全部条目在途 (断链: 只清在途不清状态)
    void clear_all_pending();

    /// 全量清除 (仅实例销毁时)
    void clear_all();

  private:
    /// 凭据类字段刷新 (事件为权威源)
    static void copy_credentials(GwSessionInfo &info, const acc_login_event_info &ev)
    {
      memcpy(info.order_way_ext.data(), ev.order_way_ext, info.order_way_ext.size());
      memcpy(info.user_info.data(), ev.user_info, info.user_info.size());
      memcpy(info.password.data(), ev.password, info.password.size());
    }

    /// 值不变不写 (RUL-15): 与现值逐字节比较, 不同才写 —— 恒等回填零写入
    template <size_t N>
    static void fill_if_changed(std::array<char, N> &dst, const char *src)
    {
      if (src == nullptr || src[0] == '\0')
        return; ///< 空值不覆盖
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

    /// 规范键提取: 遇 NUL 截止, 裁剪尾部空格
    static std::string norm_key(const char *p, size_t max_len)
    {
      size_t n = 0;
      while (n < max_len && p[n] != '\0')
        ++n;
      while (n > 0 && p[n - 1] == ' ')
        --n;
      return std::string(p, n);
    }

    std::mutex mutex_;
    std::unordered_map<std::string, GwSessionInfo> sessions_;
  };

  // ====== 内联实现 ======

  inline GwSessionInfo *C98SessionCache::create_session(const acc_login_event_info &login_req)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = norm_key(login_req.fund_account_id, sizeof(login_req.fund_account_id));
    auto ret = sessions_.emplace(key, GwSessionInfo{});
    GwSessionInfo &info = ret.first->second;
    if (ret.second)
    {
      // 不存在: 完整构建插入 (登录请求字段全量)
      info.reset();
      memcpy(info.fund_account_id.data(), login_req.fund_account_id, info.fund_account_id.size());
      memcpy(info.cust_id.data(), login_req.cust_id, info.cust_id.size());
      memcpy(info.account_id.data(), login_req.account_id, info.account_id.size());
      memcpy(info.branch_id.data(), login_req.branch_id, info.branch_id.size());
      memcpy(info.password.data(), login_req.password, info.password.size());
    }
    // 已存在: 身份字段保持上次 fill_session_login_ok 的值, 仅刷新凭据
    copy_credentials(info, login_req);
    return &info;
  }

  inline GwSessionInfo *C98SessionCache::get_session(const char *fund_account_id, size_t len)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(norm_key(fund_account_id, len));
    if (it != sessions_.end())
      return &it->second;
    return nullptr;
  }

  inline void C98SessionCache::fill_session_login_ok(GwSessionInfo *info, const char *cust_id,
                                                     const char *branch_id, const char *account_id)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    fill_if_changed(info->cust_id, cust_id);
    fill_if_changed(info->branch_id, branch_id);
    fill_if_changed(info->account_id, account_id);
  }

  inline void C98SessionCache::collect_relogin_events(std::vector<acc_login_event_info> &out)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &kv : sessions_)
    {
      GwSessionInfo &s = kv.second;
      // 98 无 per-account 状态机, 以 fund_account_id 非空格为"曾登录"标志
      if (s.fund_account_id[0] == '\0' || s.fund_account_id[0] == ' ')
        continue;
      acc_login_event_info ev;
      std::memset(&ev, 0, sizeof(ev));
      memcpy(ev.fund_account_id, s.fund_account_id.data(), sizeof(ev.fund_account_id));
      memcpy(ev.cust_id, s.cust_id.data(), sizeof(ev.cust_id));
      memcpy(ev.account_id, s.account_id.data(), sizeof(ev.account_id));
      memcpy(ev.branch_id, s.branch_id.data(), sizeof(ev.branch_id));
      memcpy(ev.order_way_ext, s.order_way_ext.data(), sizeof(ev.order_way_ext));
      memcpy(ev.user_info, s.user_info.data(), sizeof(ev.user_info));
      memcpy(ev.password, s.password.data(), sizeof(ev.password));
      out.push_back(ev);
    }
  }

  inline void C98SessionCache::clear_all_pending()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // 98 无 per-account 状态机, 无在途位可清; 保留此接口以与 gw_login_cache 契约一致
  }

  inline void C98SessionCache::clear_all()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.clear();
  }

} // namespace lb_api