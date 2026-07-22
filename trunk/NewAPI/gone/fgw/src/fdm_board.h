// fdm_board - FDM 板卡信息（聚合根，含 customer 和 fdm_session 结构）
//
// Step 5 决策：fdm_board 是 customer / fdm_session 的拥有者
//   - customer：fdm_board::customers_ 持有，故定义在 fdm_board.h
//   - fdm_session：fdm_board::fdm_sessions_ 持有，故定义在 fdm_board.h
//
// 已做修正 #4：customers_/fdm_sessions_ 存对象不存指针（vector）
// 已做修正 #5：fdm_board 数量不固定、编号不连续 → data_repo 用 hash_map_mth
// R-14：热字段在前（board_no, max_user_id, load_status）
// R-15：字符串长度对齐 g1comdefine.h

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "api_link.h"
#include "fdm_link.h"

#include "comm_sys.h"
#include "data_def.h"
#include "g1msghead.h"
#include "mlock.h"

namespace lb_fgw {

struct fdm_session {
  uint32_t session_id_; ///< fdm 分配并持久化，跨 fdm 重启不变
  uint16_t board_no_;   ///< 板卡号（R-9 自包含）
  int16_t _pad_;        ///< 对齐填充
  api_link *ln_api_;    ///< 绑定的 api 链接（带引用计数，fdm recv 线程读写，免锁）
  fdm_link *ln_fdm_;    ///< 所属 fdm 链接（反向引用，用于事件投递）

  FORCE_INLINE bool is_load() { return board_no_ != 0; }
  fdm_session() : session_id_(0), board_no_(0), ln_api_(nullptr), ln_fdm_(nullptr){};
};

// ============================================================================
// 业务可观察状态（区别于 aio_tcp.is_work/is_close 等库状态）
// ============================================================================

/// fdm_board 数据加载状态（db engine 加载设置）
/// 0=未加载（含重新加载重置），1=已加载
static constexpr int BOARD_LOAD_NOT = 0;
static constexpr int BOARD_LOADED = 1;

// ============================================================================
// fdm_board 聚合根类
// ============================================================================

/// FDM 板卡信息（fpga 硬件板卡在 fgw 内的表示）
/// 数量级：≤16（动态增减）；标识：board_no_（db 主键）
class fdm_board {
public:
  // 初始化板卡号
  void init(uint16_t board_no, int16_t market) {
    board_no_ = board_no;
    load_status_ = BOARD_LOAD_NOT;
    market_type_ = market;
    customers_.clear();
    fdm_sessions_.clear();
    ilock.init();
    load_time_ = 0;
  }

  // 状态 / 属性
  FORCE_INLINE uint16_t get_board_no() const { return board_no_; }
  FORCE_INLINE fdm_link *get_link() { return &link_; }
  FORCE_INLINE int16_t get_market_type() const { return market_type_; }
  FORCE_INLINE bool is_loaded() const { return load_status_ == BOARD_LOADED; }

  void load_done(const char *ip, int32_t port, int32_t load_time);
  bool check_load_reset(int32_t new_load_time);
  bool get_addr(lb_common::csock_addr &o_addr);

  // 客户管理（load_custs 后填充，按 user_id dense 索引）
  void fill_customers(std::vector<customer> &custs);
  FORCE_INLINE customer *find_customer(uint16_t user_id) {
    if (likely(user_id < customers_.size())) {
      customer &tc = customers_[user_id];
      if (tc.is_load())
        return &tc;
    }
    return nullptr;
  }
  int32_t get_customer_count() const { return customers_.size(); }

  // fdm_session 管理（S9 登录应答成功时建，业务下行按 session_id 查找）
  FORCE_INLINE fdm_session *find_fdm_session(uint32_t session_id) {
    if (likely(session_id < fdm_sessions_.size())) {
      fdm_session &tc = fdm_sessions_[session_id];
      if (tc.is_load())
        return &tc;
    }
    return nullptr;
  }
  int32_t add_fdm_session(uint32_t session_id, api_link *ln_api);
  void detach_fdm_session_link(uint32_t session_id, api_link *ln_api);

  void dispatch_onboard_state(g1_msg_head *head);

private:
  int32_t load_status_; ///< 从数据库加载状态（0-初始或重新加载设置，1-加载完成设置）
  uint16_t board_no_;   ///< 板卡号（db 主键）
  // 保存从数据库中获取的 fdm 上场结束时间，
  // 若数据库中的时间大于此时间，需要重新加载板卡数据。
  int16_t market_type_; ///< 市场类型（board_load_info 加载，S7 网关登录用）

  // R-10/R-15：生命周期同 fdm_board，对象存
  fdm_link link_; ///< 该板卡的 fdm 链接（重连不新建）

  // R-4：存对象不存指针
  std::vector<customer> customers_;       ///< 客户列表（按 user_id dense 索引）
  std::vector<fdm_session> fdm_sessions_; ///< fdm 会话列表（按 session_id dense 1-连续）

  lb_common::atomic_lock ilock;
  int32_t load_time_;
  lb_common::csock_addr remote_; ///< 连接地址（do_work 从 board_load_info 填）
};

} // namespace lb_fgw
