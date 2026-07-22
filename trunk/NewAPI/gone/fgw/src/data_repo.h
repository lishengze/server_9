// data_repo - 公共数据存储（跨模块共享索引，含 agw_user 和 board_user_id 结构）
//
// Step 5 决策：data_repo 是 agw_user / board_user_id 的拥有者
//   - agw_user：data_repo::agwusers_ 持有，故定义在 data_repo.h
//   - board_user_id：data_repo::cust_map_ 的 value 类型，故定义在 data_repo.h
//
// 已做修正 #6：跨线程访问用 hash_map_mth（自带表级自旋锁）
// 已做修正 #19：securities_ 不在 data_repo（不是共享数据，由 data_engine 私有）
// 已做修正 #20：CustomerByKey 从 Board 移至 data_repo（cust_map_）
// 已做修正 #21：fdms_ value 是 fdm_board*（hash_map_mth 引用语义）
// 已做修正 #16：独立 Index 模块 → 改为 data_repo 子模块
// R-13：大型数据集合抽象为独立子模块，由一个引擎（data_engine）拥有，
//       其他模块通过裸指针只读访问
//
// Step 8 增量：仅增加 public 方法，不增加数据成员（骨架定稿后成员锁死）

#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "fdm_board.h"

#include "comm_sys.h"
#include "data_def.h"
#include "g1comdefine.h"
#include "g1trademsg.h"
#include "hash_comm.h"
#include "hash_map_mth.h"
#include "mevent.h"
#include "mlock.h"

namespace lb_fgw {

// ---- 哈希函数对象（hash_map_mth 要求的第 4 模板参数）----

/// uint16_t 键哈希（板卡号）
struct fdm_board_hash {
  uint64_t operator()(uint16_t k) const noexcept { return static_cast<uint64_t>(k); }
};

struct fgw_fundacc_key {
  char fund_account_id[16]; ///< 客户资金账号
  char branch_id[16];       ///< 分支机构代码 (协议 BRANCHID_LEN=12, 此处对齐到 16)

  /// 默认构造
  fgw_fundacc_key(){};
  /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
  fgw_fundacc_key(const fgw_fundacc_key &src) {
    *((uint64_t *)(fund_account_id)) = *((uint64_t *)(src.fund_account_id));
    *((uint64_t *)(&(fund_account_id[8]))) = *((uint64_t *)(&(src.fund_account_id[8])));
    *((uint64_t *)(branch_id)) = *((uint64_t *)(src.branch_id));
    *((uint64_t *)(&(branch_id[8]))) = *((uint64_t *)(&(src.branch_id[8])));
  }
  /// 字符串构造
  fgw_fundacc_key(const char *fund_account, const char *branch) {
    int32_t i = 0;
    while (fund_account[i] != '\0' && i < 16) {
      fund_account_id[i] = fund_account[i];
      i++;
    }
    while (i < 16) {
      fund_account_id[i] = '\0';
      i++;
    }
    i = 0;
    while (branch[i] != '\0' && i < 16) {
      branch_id[i] = branch[i];
      i++;
    }
    while (i < 16) {
      branch_id[i] = '\0';
      i++;
    }
  }
  /// 拷贝赋值
  fgw_fundacc_key &operator=(const fgw_fundacc_key &src) {
    *((uint64_t *)(fund_account_id)) = *((uint64_t *)(src.fund_account_id));
    *((uint64_t *)(&(fund_account_id[8]))) = *((uint64_t *)(&(src.fund_account_id[8])));
    *((uint64_t *)(branch_id)) = *((uint64_t *)(src.branch_id));
    *((uint64_t *)(&(branch_id[8]))) = *((uint64_t *)(&(src.branch_id[8])));
    return *this;
  }
  /// 相等比较 (按 64-bit 块)
  bool operator==(const fgw_fundacc_key &src) const {
    if (*((uint64_t *)(fund_account_id)) != *((uint64_t *)(src.fund_account_id)))
      return false;
    if (*((uint64_t *)(&(fund_account_id[8]))) != *((uint64_t *)(&(src.fund_account_id[8]))))
      return false;
    if (*((uint64_t *)(branch_id)) != *((uint64_t *)(src.branch_id)))
      return false;
    if (*((uint64_t *)(&(branch_id[8]))) != *((uint64_t *)(&(src.branch_id[8]))))
      return false;
    return true;
  }
  /// 析构
  ~fgw_fundacc_key(){};
};

/// FPGA 多客户 hash 函数 (按 fund_account_id + branch_id 哈希)
struct fpga_fundacc_hash {
  uint64_t operator()(const fgw_fundacc_key &key) const noexcept {
    uint64_t kl = lb_common::hash_fm8<16>(key.fund_account_id);
    uint64_t kh = lb_common::hash_fm8<16>(key.branch_id);
    return (kh | kl);
  }
};

struct agw_user_key {
  char user_name[G1_SESSION_LEN];

  agw_user_key(){};
  /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
  agw_user_key(const agw_user_key &src) { std::memcpy(user_name, src.user_name, sizeof(user_name)); }
  agw_user_key(const char *name) {
    int32_t i = 0;
    while (name[i] != '\0' && i < 16) {
      user_name[i] = name[i];
      i++;
    }
    while (i < G1_SESSION_LEN) {
      user_name[i] = '\0';
      i++;
    }
  }
  /// 拷贝赋值
  agw_user_key &operator=(const agw_user_key &src) {
    std::memcpy(user_name, src.user_name, sizeof(user_name));
    return *this;
  }
  /// 相等比较 (按 64-bit 块)
  bool operator==(const agw_user_key &src) const {
    if (std::memcmp(user_name, src.user_name, sizeof(user_name)) == 0)
      return true;
    return false;
  }
  /// 析构
  ~agw_user_key(){};
};

struct agw_user_hash {
  uint32_t operator()(const agw_user_key &key) const noexcept { return lb_common::hash_fm8<16>(key.user_name); }
};

/// (板卡号, 板内 user_id) 复合索引（不含自身 key，R-9 自包含）
/// 用于 cust_map_：由 (branch_id, fund_account_id) 反查 (board_no, user_id)
struct board_user_id {
  uint16_t board_no_; ///< 板卡号
  uint16_t user_id_;  ///< 板内 user_id
};

struct board_session_id {
  uint16_t board_no_;   ///< 板卡号
  uint32_t session_id_; ///< 板内 session_id
};

/// agwuser 业务实体（"账户与通道绑定"的业务概念）
class agw_user {
private:
  char user_name_[G1_SESSION_LEN]; ///< agwuser 账户名（api 登录传入）
  lb_common::cmutex mlock_;
  api_link *link_;                       ///< 该 agwuser 绑定的 api 链接（普通指针，R-8）
  std::vector<board_session_id> fdm_ses; ///< 反向追踪引用该 agwuser 的所有 fdm_session（免锁，仅 fdm recv 线程写）

public:
  agw_user() : link_(nullptr) {
    std::memset(user_name_, 0, sizeof(user_name_));
    fdm_ses.clear();
  }
  ~agw_user() {
    if (nullptr != link_) {
      link_->sub_ref();
      link_ = nullptr;
    }
  }
  int32_t init(const char *agw_user_name, api_link *new_link);
  int32_t attach_api_link(api_link *new_link);
  void add_fdm_session(uint16_t board, uint32_t session);
  bool detach_api_link(std::vector<board_session_id> &o_ses, api_link *cmp_link);
};

// ---- api_link 解除引用事件（投递到 fdm recv 线程执行）----

/// 解除引用事件数据（≤56 字节，event_info::buf 上限）
struct api_link_deref_data {
  fdm_board *board;
  uint32_t session_id;
  api_link *expected; ///< 期望的 api_link 值（add_ref 保护，不悬空）
};

/// 解除 api_link 引用事件处理器（deal 在 fdm recv 线程执行）
class api_link_deref_op : public lb_common::event_op {
public:
  void deal(char *pbuf) override {
    auto *d = reinterpret_cast<api_link_deref_data *>(pbuf);
    d->board->detach_fdm_session_link(d->session_id, d->expected);
  }
};

/// 公共数据存储（跨模块共享索引）
/// 数据成员 = Step 5 定稿，不新增；Step 8 只增量追加 public 方法
class data_repo {
public:
  // 析构：释放 fdm_board*/agw_user*（hash_map_mth 析构不 delete value 指针）
  ~data_repo() {
    for (auto *b : fdm_vec_)
      delete b;
    for (auto *u : agwuser_vec_)
      delete u;
  }

  // 初始化 3 个哈希表
  int32_t init();

  // ---- fdms_（board_no → fdm_board*）板卡路由 ----
  int32_t add_fdm(uint16_t board_no, fdm_board *board);
  FORCE_INLINE int32_t find_fdm(fdm_board *&o_board, uint16_t board_no) { return fdms_.find(o_board, board_no, 0); }
  /// 获取全部板卡指针（供 fdm_engine 心跳/重连/建链等业务遍历）
  void get_fdm_boards(std::vector<fdm_board *> &o_boards);

  // ---- agwusers_（fundacc_key → agw_user*）登录中转 ----
  // 返回 agw_user*：新建/复用后绑定 link 并 add_ref；失败返回 nullptr
  agw_user *add_agwuser_link(const agw_user_key &key, api_link *link);
  // 解绑 link_、sub_ref 之前向 fdm_ses 关联的 fdm_link recv 线程投递解除引用事件；未找到静默返回
  void detach_agwuser_link(api_link *link);

  // ---- cust_map_（fundacc_key → board_user_id）客户索引 ----
  int32_t add_cust(const fgw_fundacc_key &key, const board_user_id &id);
  FORCE_INLINE int32_t find_cust(board_user_id &o_id, const fgw_fundacc_key &key) {
    // 不带锁
    return cust_map_.find(o_id, key, 0);
  }

  // ---- secs_ 数据加载与证券信息推送的遍历 ----
  void add_sec(std::vector<sec_info> &db_secs);
  FORCE_INLINE int32_t get_sec_size() { return secs.size(); }
  int32_t patch_secs_msg(sec_push_head *buf_head, int32_t buf_len, int32_t start_idx);

private:
  // ---- hash_map_mth：跨线程并发访问（自带表级自旋锁）----

  /// fdm_board 数量不固定，编号不连续（已做修正 #5）
  /// value 是裸指针（已做修正 #21）
  lb_common::hash_map_mth<64, uint16_t, fdm_board *, fdm_board_hash> fdms_;

  /// agwuser（普通指针管理，R-8）
  lb_common::hash_map_mth<256, agw_user_key, agw_user *, agw_user_hash> agwusers_;

  /// 客户 (fund_account_id, branch_id) → (board_no, user_id)
  /// [I] S8 修正：原 agw_user_key(user_name) 与注释不符，改为 fgw_fundacc_key
  lb_common::hash_map_mth<256, fgw_fundacc_key, board_user_id, fpga_fundacc_hash> cust_map_;

  // ---- cmutex + vector：保护裸指针 vector 的添加与遍历 ----
  lb_common::cmutex vec_lock_;          // 保护下方两个 vector 的添加/遍历
  std::vector<sec_info> secs;           ///< 证券信息
  std::vector<fdm_board *> fdm_vec_;    ///< 仅添加，用于遍历和析构释放
  std::vector<agw_user *> agwuser_vec_; ///< 仅添加，用于遍历和析构释放

  api_link_deref_op deref_op_; ///< 解除引用事件处理器（单例）
};

} // namespace lb_fgw
