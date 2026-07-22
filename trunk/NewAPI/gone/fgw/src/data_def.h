#pragma once

#include "comm_sys.h"
#include "g1comdefine.h"

#include <cstdint>
#include <cstring>

namespace lb_fgw {

struct board_load_info {
  int32_t tradeday_;        ///< 交易日
  uint16_t board_no_;       ///< 板卡号（fpga 分配，稳定）
  int16_t market_type_;     ///< 市场类型
  char data_status_;        ///< DataStatus '0-未上场，1-上场完成'
  int32_t data_load_time_;  ///< DataLoadTime 上场成功结束时间
  int32_t gw_port_;         ///< GWPort 网关端口
  char gw_ip_[32];          ///< GWIp 网关IP
};

/// 客户业务实体（投资者账户在 fgw 内的表示）
/// 数量级：每板卡数千；标识（板内）：user_id（fpga 分配，0 连续）
/// 标识（全局）：(branch_id, fund_account_id) 复合 key
struct customer {
  uint16_t board_no_;                          ///< 板卡号（fpga 分配，稳定）
  uint16_t user_id_;                           ///< 板内用户标识（fpga 分配，0 连续）
  int16_t market_type_;                        ///< 市场类型
  int16_t onboard_state;                       ///< 用户在板卡上的状态：1- 正常，2-故障（来自 fpga OFFLINE_PUSH）
  char fund_account_id_[G1_FUNDACCOUNTID_LEN]; ///< 资金账号
  char branch_id_[G1_BRANCHID_LEN];            ///< 分支机构代码
  char cust_id_[G1_CUSTID_LEN];                ///< 客户号
  char holder_acc_[G1_HOLDERACC_LEN];          ///< 股东账号（DB 列名 AccountID，结构内保留协议命名 holder_acc）

  FORCE_INLINE bool is_load() { return fund_account_id_[0] != '\0' && board_no_ != 0; }
  customer() : board_no_(0), user_id_(0), market_type_(0), onboard_state(1) {
    std::memset(fund_account_id_, 0, sizeof(fund_account_id_));
    std::memset(branch_id_, 0, sizeof(branch_id_));
    std::memset(cust_id_, 0, sizeof(cust_id_));
    std::memset(holder_acc_, 0, sizeof(holder_acc_));
  }
};

/// 证券业务实体（data_engine 私有持有，多 Board 共享同一份）
/// 数量级：≤10万；标识：sec_index_（全局 0 连续，写入者板卡持久）
/// 同步来源：特定 fdm 板卡（"写入者"），**非 db**
struct sec_info {
  uint16_t sec_index_;                     ///< 证券在 FPGA 内的索引（全局 0 连续）
  int16_t market_type_;                    ///< 市场类型
  int32_t _pad_;                           ///< 对齐填充
  char security_id_[G1_SECURITYID_MAXLEN]; ///< 证券代码（8 字节定长）
  int32_t buy_qty_unit_;                   ///< 买数量单位
  int32_t sell_qty_unit_;                  ///< 卖数量单位
  int64_t price_unit_;                     ///< 价格单位

  FORCE_INLINE bool is_load() { return security_id_[0] != '\0'; }
  sec_info() : sec_index_(0), market_type_(0) { std::memset(security_id_, 0, sizeof(security_id_)); }
};

} // namespace lb_fgw