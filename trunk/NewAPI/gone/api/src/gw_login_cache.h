// gw_login_cache - 个微柜台专属的账户登录缓存 (gw_counter_direct 持有)
//
// gw_counter_direct 成员 (每 api 实例一份), 以 fund_account_id 为主键, 与 98 柜台
// 缓存 (c98_session_cache.h) 互不共享。设计契约 (与 98 柜台缓存拆分, RUL-5):
//   1. 状态维护复用 log_state_ctl 状态机 (三场景语义见 api_event_msg.h, 范式同
//      fpga per-customer log_state): 幂等回调 / 在途合并 / 内部重登, 单原子字全
//      CAS 可跨线程直接调用;
//   2. 条目在登录事件处理时创建/凭据刷新 (deal_cust_login 经 prepare_login,
//      幂等/合并路径同样刷新 —— 盘中改密重登即时生效), 条目永不删除 —— 节点指针
//      稳定, 组包经指针直读; 身份字段 (cust/account/branch) 发布屏障 = log_gw
//      的 release(置 OK)/acquire(is_logined) 配对, 成立前提 "发布后不再写" 由
//      fill_login_ok 值不变不写保证 (RUL-15, 成立条件 = 柜台侧账户身份恒等,
//      与 c98_session_cache.h §3 同口径);
//   3. 断链仅清全部条目在途 (clear_all_pending, 无登录回调, 用户经链接断开事件
//      感知), 登录状态保持, 重连后按 is_logined 条目批量自动重登。
//   4. 单/多客户两种存储模式 (need_multi_cust 配置: 0=单客户 / 非0=多客户):
//      单客户模式 (set_one_cust) 条目直存成员 local_, 零哈希零 map 查找, 发单路径
//      find 退化为键匹配比对 (不匹配拒单); 状态机/回填/重登收集/清在途语义与
//      多客户完全一致 —— 模式只是存储策略, 不改任何登录行为 (断线自动重登照常)。
//      唯一例外: local_ 条目一经绑定资金账户 (首次登录写入), 其他资金账户的登录
//      事件一律拒绝 (含登出后 —— 换户 = 销毁实例重建; 返回 nullptr, 调用方回调
//      失败)。绑定门同时守住两个窗口: 登录/重登在途时条目字段不可被 B 改写 (否则
//      A 应答失配被当迟到丢弃、条目楔死到断链); 登出后也不可被 B 整块重建 (否则
//      与发单路径对条目的无锁直读构成数据竞争, 委托包可撕裂出 A/B 混合身份)。

#pragma once

#include "api_event_msg.h"
#include "gw_head.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace lb_api
{

  /// 个微账户登录信息
  /// 现实对象：个微柜台的客户登录会话条目（身份+凭据+登录状态机）
  struct GwLoginInfo
  {
    log_state_ctl log_gw;                       ///< 个微登录控制状态机 (原子, 状态+在途)
    std::array<char, 16> fund_account_id;       ///< 资金账号（主键, 空格定长）
    std::array<char, 16> cust_id;               ///< 客户号（从登录应答回填）
    std::array<char, 12> account_id;            ///< 股东账户（从登录应答回填）
    std::array<char, 10> branch_id;             ///< 分支机构代码
    std::array<char, 2> order_way_ext;          ///< 客户委托方式
    std::array<char, 64> user_info;             ///< 用户私有信息（重登组包用）
    std::array<char, 256> password;             ///< 客户登录密码（重登组包用）
    std::array<char, 1024> client_feature_code; ///< 客户终端信息（重登组包用）
    int64_t agw_seq_id = 0;                     ///< 个微会话序号（登录应答回填）
    int64_t last_cust_req_no = 0;               ///< 最近一次登录请求号（重登事件关联用）

    /// 现实操作：对会话条目的复位（身份空格化、状态机归零）
    void reset()
    {
      log_gw.log_state = 0;
      fund_account_id.fill(' ');
      cust_id.fill(' ');
      account_id.fill(' ');
      branch_id.fill(' ');
      order_way_ext.fill(' ');
      user_info.fill(' ');
      password.fill('\0');
      client_feature_code.fill('\0');
      agw_seq_id = 0;
      last_cust_req_no = 0;
    }

    /// 现实操作：无
    GwLoginInfo() { reset(); }
  };

  /// 个微登录缓存管理器 (gw_counter_direct 成员)
  /// 键规范 (NUL 截止 + 裁尾空格) 为内部实现细节, 全部接口直接接收原始账户标识。
  /// 现实对象：个微柜台的本实例客户登录会话表（单/多客户两种存储模式）
  class GwLoginCache
  {
  public:
    GwLoginCache() = default;
    ~GwLoginCache() = default;
    GwLoginCache(const GwLoginCache &) = delete;
    GwLoginCache &operator=(const GwLoginCache &) = delete;

    /// 登录事件处理时的凭据暂存 (upsert):
    ///   条目不存在 → 完整构建信息插入 (登录请求字段全量);
    ///   条目已存在 → 仅刷新凭据类字段, cust_id/account_id/branch_id 保持上次
    ///     fill_login_ok 的值 (身份字段以柜台登录应答为权威源, 不被后续事件覆盖);
    ///     状态机与 agw_seq_id 不动。
    /// 返回条目指针 (长期有效, 见文件头 §2)。
    /// 现实操作：对客户登录会话条目的凭据暂存/刷新
    GwLoginInfo *prepare_login(const acc_login_event_info &login_req);

    /// 获取条目 (找不到返回 nullptr)。状态机 (log_gw) 为原子字可跨线程直接调用;
    /// 组包字段读取线程模型见文件头 §2。
    /// 现实操作：按资金账号取客户登录会话条目
    GwLoginInfo *find(const char *fund_account_id, size_t len);

    /// 登录成功应答回填 (cust_id/branch_id/account_id/agw_seq_id, C 串源, 空值不覆盖)。
    /// 回填完成后由调用方驱动 log_gw.ans_login_ok (先回填再置 OK 形成屏障)。
    /// 现实操作：对客户登录会话条目的身份回填（应答值）
    void fill_login_ok(GwLoginInfo *info, const char *cust_id, const char *branch_id,
                       const char *account_id, int64_t agw_seq_id);

    /// 收集已登录 (log_gw is_logined) 账户的自动重登事件 (user_origin=0, 内部重登)
    /// 现实操作：收集全部已登录客户的自动重登事件
    void collect_relogin_events(std::vector<acc_login_event_info> &out);

    /// 清除全部条目在途 (断链: 只清在途不清状态)
    /// 现实操作：清除全部客户登录会话的在途占位
    void clear_all_pending();

    /// 单/多客户存储模式设置 (gw_counter_direct::init 按 need_multi_cust 配置设置:
    /// 单客户=单条目直存, 多客户=逐账户 map; 见文件头 §4)
    /// 现实操作：对登录会话表存储模式的设置
    void set_one_cust(bool on) { one_cust_ = on; }

    /// 现实操作：读登录会话表存储模式（单客户与否）
    bool one_cust() const { return one_cust_; }

  private:
    /// 条目身份字段完整构建 (登录请求字段全量; 单/多客户两分支共用, 语义=
    /// "未登录成功也完整构建": 首次/失败重试/换户后重建, 已登录成功则不走此路)
    static void build_identity(GwLoginInfo &info, const acc_login_event_info &ev)
    {
      memcpy(info.fund_account_id.data(), ev.fund_account_id, info.fund_account_id.size());
      memcpy(info.cust_id.data(), ev.cust_id, info.cust_id.size());
      memcpy(info.account_id.data(), ev.account_id, info.account_id.size());
      memcpy(info.branch_id.data(), ev.branch_id, info.branch_id.size());
      memcpy(info.password.data(), ev.password, info.password.size());
    }

    /// 凭据类字段刷新 (事件为权威源: 委托方式/私有信息/密码/终端信息/请求号)
    static void copy_credentials(GwLoginInfo &info, const acc_login_event_info &ev)
    {
      memcpy(info.order_way_ext.data(), ev.order_way_ext, info.order_way_ext.size());
      memcpy(info.user_info.data(), ev.user_info, info.user_info.size());
      memcpy(info.password.data(), ev.password, info.password.size());
      memcpy(info.client_feature_code.data(), ev.client_feature_code, info.client_feature_code.size());
      info.last_cust_req_no = ev.cust_req_no;
    }

    /// 值不变不写 (RUL-15): 空格填充目标与现值逐字节比较, 不同才写 —— 恒等回填
    /// 零写入, 维持"发布(置 OK)后身份字段不再写"的屏障前提 (KNOWN-1 源②关闭;
    /// 屏障成立条件 = 柜台侧账户身份恒等, 见文件头 §3)
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

    /// 定长 array → C 串缓冲 (NUL 结尾, 裁尾空格; 源可能空格填充或 NUL 填充)
    template <size_t M>
    static void to_cstr(char *dst, size_t dst_len, const std::array<char, M> &src)
    {
      size_t n = 0;
      while (n < M && src[n] != '\0')
        ++n;
      while (n > 0 && src[n - 1] == ' ')
        --n;
      if (n > dst_len - 1)
        n = dst_len - 1;
      for (size_t i = 0; i < n; ++i)
        dst[i] = src[i];
      for (size_t i = n; i < dst_len; ++i)
        dst[i] = '\0';
    }

    /// 会话条目 → 自动重登事件 (user_origin=0; 多客户遍历与单客户条目共用)
    static void build_relogin_event(const GwLoginInfo &s, acc_login_event_info &ev)
    {
      std::memset(&ev, 0, sizeof(ev));
      to_cstr(ev.fund_account_id, sizeof(ev.fund_account_id), s.fund_account_id);
      to_cstr(ev.cust_id, sizeof(ev.cust_id), s.cust_id);
      to_cstr(ev.account_id, sizeof(ev.account_id), s.account_id);
      to_cstr(ev.branch_id, sizeof(ev.branch_id), s.branch_id);
      ev.order_way_ext[0] = s.order_way_ext[0];
      ev.order_way_ext[1] = s.order_way_ext[1];
      to_cstr(ev.user_info, sizeof(ev.user_info), s.user_info);
      std::memcpy(ev.password, s.password.data(), sizeof(ev.password));
      std::memcpy(ev.client_feature_code, s.client_feature_code.data(), sizeof(ev.client_feature_code));
      ev.cust_req_no = s.last_cust_req_no;
      ev.user_origin = 0; ///< 内部重登 (应答成功静默)
      ev.filled = 0;
    }

    std::mutex mutex_; ///< 保护 logins_/local_ (互斥访问)
    ///< (one_cust_ = true 时 logins_ 不用, §4)(set_one_cust 设置)
    bool one_cust_ = false;                               ///< 单客户模式标志
    GwLoginInfo local_;                                   ///< 单客户模式条目
    std::unordered_map<std::string, GwLoginInfo> logins_; ///< 多客户模式: 逐账户条目表
  };

  // ====== 内联实现 ======

  inline GwLoginInfo *GwLoginCache::prepare_login(const acc_login_event_info &login_req)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (one_cust_)
    {
      // 单客户模式 (§4): 条目直存成员, 构建语义与多客户完全一致
      GwLoginInfo &info = local_;
      // 绑定门: 条目一经绑定资金账户, 异账户登录一律拒 (含登出后) —— 在途改写会
      // 楔死条目, 登出后整块重建会与发单路径的无锁直读构成数据竞争 (§4)
      const bool tbound =
          !norm_key(info.fund_account_id.data(), info.fund_account_id.size()).empty();
      if (tbound &&
          norm_key(login_req.fund_account_id, sizeof(login_req.fund_account_id)) !=
              norm_key(info.fund_account_id.data(), info.fund_account_id.size()))
      {
        return nullptr;
      }
      if (!info.log_gw.is_logined())
      {
        // 未登录成功(NONE/LOGOUT/在途首登): 完整构建 (登录请求字段全量, 语义同
        // 多客户分支的"未登录也完整构建"; 已登录成功则身份保持应答回填值)
        build_identity(info, login_req);
      }
      copy_credentials(info, login_req);
      return &info;
    }
    std::string key = norm_key(login_req.fund_account_id, sizeof(login_req.fund_account_id));
    auto ret = logins_.emplace(key, GwLoginInfo{}); ///< 不存在则插入 (默认构造 reset(): 空格填充 + log_state=NONE)
    GwLoginInfo &info = ret.first->second;
    if (ret.second || !info.log_gw.is_logined())
    {
      // 不存在: 完整构建插入 (登录请求字段全量, 身份字段以事件值为初值)
      // 未登录成功也完整构建，防止用户第一次输入错误，以便第二次输出正确能更新
      build_identity(info, login_req);
    }
    // 已存在: cust_id/account_id/branch_id 保持上次 fill_login_ok 的值, 仅刷新凭据
    copy_credentials(info, login_req);
    return &info;
  }

  inline GwLoginInfo *GwLoginCache::find(const char *fund_account_id, size_t len)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (one_cust_)
    {
      // 单客户模式: 唯一条目键匹配 (资金账号与登录会话不符 → 拒单, 见 §4)
      if (norm_key(fund_account_id, len) ==
          norm_key(local_.fund_account_id.data(), local_.fund_account_id.size()))
      {
        return &local_;
      }
      return nullptr;
    }
    auto it = logins_.find(norm_key(fund_account_id, len));
    if (it != logins_.end())
      return &it->second;
    return nullptr;
  }

  inline void GwLoginCache::fill_login_ok(GwLoginInfo *info, const char *cust_id, const char *branch_id,
                                          const char *account_id, int64_t agw_seq_id)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    fill_if_changed(info->cust_id, cust_id);
    fill_if_changed(info->branch_id, branch_id);
    fill_if_changed(info->account_id, account_id);
    /// agw_seq_id 每会话必变, 直写; 仅作会话记录, 无读者 (重登组包恒携带 0, )
    lb_common::atomic_store64(&info->agw_seq_id, agw_seq_id);
  }

  inline void GwLoginCache::collect_relogin_events(std::vector<acc_login_event_info> &out)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (one_cust_)
    {
      // 单客户模式: 唯一条目已登录则收集一条 (§4, 语义与多客户一致)
      if (local_.log_gw.is_logined())
      {
        acc_login_event_info ev;
        build_relogin_event(local_, ev);
        out.push_back(ev);
      }
      return;
    }
    for (auto &kv : logins_)
    {
      GwLoginInfo &s = kv.second;
      if (!s.log_gw.is_logined())
        continue;
      acc_login_event_info ev;
      build_relogin_event(s, ev);
      out.push_back(ev);
    }
  }

  inline void GwLoginCache::clear_all_pending()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (one_cust_)
    {
      local_.log_gw.clear_pending();
      return;
    }
    for (auto &kv : logins_)
    {
      kv.second.log_gw.clear_pending();
    }
  }

} // namespace lb_api
