#pragma once
#include <array>
#include <string>
#include <vector>   
#include <map>
#include <cstdint>

// 链接登陆;
class AgwMsgLogin {
std::array<char, 32> sender_comp_id;         ///< 发送方代码
std::array<char, 32> target_comp_id;         ///< 接收方代码
int32_t heart_bt_int;         ///< 心跳间隔时间
std::array<char, 32> password;         ///< 用户登录密码
std::array<char, 32> default_app_ver_id;         ///< 协议版本
uint8_t spec_cust_flag;         ///< 特殊客户标识,正常为0
std::array<char, 256> trade_pwd;///< 客户交易密码-特殊用户使用
};

class  AgwMsgCustLoginReq 
{
public:
int64_t time;            ///< 发起登验证时间
int64_t client_seq_id;   ///< 用户系统消息序号
char order_way;          ///< 委托方式
char pwd_type;           ///< 密码加密类型
std::array<char, 16> cust_id;   ///< 客户号
std::array<char, 16> fund_account_id;   ///< 资金账户ID
std::array<char, 10> branch_id; ///< origid+营业部ID,各占5个字节
std::array<char, 12> account_id;  ///< 账户ID
std::array<char, 64> user_info;   ///< 用户私有信息
std::array<char, 2> order_way_ext;   ///< 委托方式扩展
std::array<char, 128> password;   ///< 客户密码
std::array<char, 1024> client_feature_code;   ///< 终端识别码
std::array<char, 1024> extra_data;   ///< 自定义json字符串

};

// 个微登录请求
class LogOnReq
{
public:
    std::array<char, 16> fund_account_id;
    std::array<char, 10> branch_id;
    std::array<char, 12> account_id;
    std::array<char, 16> cust_id;     //暂不要求传入，需要根据资金账户信息回填
    int64_t client_seq_id;
    int64_t agw_seq_id;
    uint32_t heart_bt_int;
    std::array<char, 100>   password;
    std::array<char, 1024> client_feature_code;
    std::array<char, 32> agw_user;
    // int proto_version; // 协议版本号;
};

// 现货委托 100101
class  NewAgwMsgCashAuctionOrder 
{
public:
  char              side;    
  char              order_type;  
  int64_t             order_price;   
  int64_t             order_qty;   
  int64_t             stop_px;    
  int64_t             client_seq_id; 
  uint8_t               tgw_id;       // TGW编号 -- 适配多报盘改造;
  uint16_t               policy_id;   //  佣金编号,名字待定
  std::array<char, 16>  fund_account_id;
  std::array<char, 10>  branch_id;
  std::array<char, 12>  account_id; 
  std::array<char, 8>  security_id;
  uint16_t           market_id;
};

// 290001 委托应答
class AgwMsgOrderStatusAck 
{
public:
uint8_t partition;         ///< 分区号 -- 重传时使用 -- G1 单独分配;
int32_t index;         ///< 索引号 -- 重传时使用;
uint8_t business_type;         ///< 业务类型
int64_t order_no;         ///< 客户订单号 -- 柜台生成的委托号 
std::array<char, 8> security_id;         ///< 证券代码
uint16_t market_id;         ///< 市场ID
char exec_type;         ///< 当前委托执行类型 -- 和交易所一致,
uint8_t order_status;         ///< 当前申报的状态
std::array<char, 16> cust_id;         ///< 客户号ID
std::array<char, 16> fund_account_id;         ///< 资金账户ID
std::array<char, 12> account_id;         ///< 股东帐号 ;
int64_t price;         ///< 委托价格
int64_t order_qty;         ///< 委托数量
int64_t leaves_qty;         ///< 未成交数量 
int64_t cum_qty;         ///< 累计成交数量
int64_t cancel_qty;         ///< 撤单成交数量 --G1 单独分配;
char side;         ///< 买卖方向
int64_t transact_time;         ///< 回报时间 -- 待确认具体逻辑;
std::array<char, 64> user_info;         ///< 用户私有信息- G1 - 0;
std::array<char, 16> order_id;         ///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id;         ///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id;         ///< origid+营业部ID,各占5个字节
std::array<char, 64> key_error_msg;         ///< 错误信息 - 去除;
std::array<char, 64> key_error_code;         ///< 错误代码串 - 去除;
uint8_t key_error_level;         ///< 错误级别 - 去除;
int64_t client_seq_id;         ///< 用户系统消息序号
int64_t orig_cli_ord_no;         ///< 对于撤单返回的状态响应，为原始委托的客户合同号，指示被撤消订单的ClOrdID; 对于委托确认，取值为空
int64_t frozen_trade_value;         ///< 冻结交易金额
int64_t frozen_fee;         ///< 冻结费用
uint16_t reject_reason_code;         ///< 拒绝代码 - 个微
std::array<char, 5> ord_rej_reason;         ///< 拒绝原因描述信息
std::array<char, 128> ex_err_msg;         ///< 交易所错误信息描述 -- 去除，增加映射函数;
char ord_type;         ///< 订单类型
char time_in_force;         ///< 订单有效时间类型
char position_effect;         ///< 开仓/平仓
uint8_t covered_or_uncovered;         ///< 备兑标签
std::array<char, 6> account_sub_code;         ///< 账户子编码
};

// 撤单	190001
class  AgwMsgCancelOrder {
public:
uint8_t business_type;         ///< 业务类型
std::array<char, 8> security_id;         ///< 证券代码
uint16_t market_id;         ///< 市场ID
int64_t client_order_time;         ///< 委托时间
std::array<char, 16> cust_id;         ///< 客户号
std::array<char, 16> fund_account_id;         ///< 资金帐号
std::array<char, 12> account_id;         ///< 账户id
char side;         ///< 买卖方向
char order_type;         ///< 订单类型
int64_t price;         ///< 委托价格
int64_t order_qty;         ///< 申报数量(张)/ETF份额
int64_t client_seq_id;         ///< 用户系统消息序号
std::array<char, 512> client_feature_code;         ///< 终端识别码
char order_way;         ///< 委托方式
std::array<char, 64> user_info;         ///< 用户私有信息
std::array<char, 10> branch_id;         ///< origid+营业部ID,各占5个字节
uint16_t policy_id;         ///< 策略编号
int64_t orig_cli_ord_no;         ///< 原委托的客户订单编号
std::array<char, 2> order_way_ext;         ///< 委托方式扩展
int64_t orig_client_seq_id;         ///< 原客户端消息编号
};

//成交推送	210115
class AgwMsgCashAuctionTradeER 
{
public:
uint8_t partition;         ///< 分区号
int32_t index;         ///< 索引号
uint8_t business_type;         ///< 业务类型
int64_t client_order_no;         ///< 客户订单号
std::array<char, 8> security_id;         ///< 证券代码
uint16_t market_id;         ///< 市场ID
char exec_type;         ///< 当前委托执行类型
uint8_t order_status;         ///< 当前申报的状态
std::array<char, 16> cust_id;         ///< 客户号ID
std::array<char, 16> fund_account_id;         ///< 资金账户ID
std::array<char, 12> account_id;         ///< 投资者帐号
int64_t price;         ///< 委托价格
int64_t order_qty;         ///< 委托数量
int64_t leaves_qty;         ///< 未成交数量
int64_t cum_qty;         ///< 累计成交数量
char side;         ///< 买卖方向
int64_t transact_time;         ///< 回报时间 -- 精度 todo
std::array<char, 64> user_info;         ///< 用户私有信息
std::array<char, 16> order_id;         ///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id;         ///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id;         ///< origid+营业部ID,各占5个字节
std::array<char, 64> key_error_msg;         ///< 错误信息
std::array<char, 64> key_error_code;         ///< 错误代码串
uint8_t key_error_level;         ///< 错误级别
std::array<char, 32> exec_id;         ///< 成交编号
int64_t last_px;         ///< 成交价格
int64_t last_qty;         ///< 成交数量
int64_t total_value_traded;         ///< 成交金额
int64_t fee;         ///< 费用
int64_t client_seq_id;         ///< 用户系统消息序号
char cash_margin;         ///< 信用标识
};


// ETF申赎委托	101201
class AgwMsgETFRedemptionOrder
{
uint8_t business_type;         ///< 业务类型
std::array<char, 8> security_id;         ///< 证券代码
uint16_t market_id;         ///< 市场ID
int64_t client_order_time;         ///< 委托时间
std::array<char, 16> cust_id;         ///< 客户号
std::array<char, 16> fund_account_id;         ///< 资金帐号
std::array<char, 12> account_id;         ///< 账户id
char side;         ///< 买卖方向
char order_type;         ///< 订单类型
int64_t price;         ///< 委托价格
int64_t order_qty;         ///< 申报数量(张)/ETF份额
int64_t client_seq_id;         ///< 用户系统消息序号
std::string client_feature_code;         ///< 终端识别码
char order_way;         ///< 委托方式
std::array<char, 64> user_info;         ///< 用户私有信息
std::string password;         ///< 密码
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
uint16_t policy_id;   ///< 策略编号
char enforce_flag;         ///< 强制风险标识
std::array<char, 2> order_way_ext;   ///< 委托方式扩展
std::array<char, 12> sh_account_id;   ///< 沪A股东代码
std::array<char, 12> sz_account_id;   ///< 深A股东代码
};


class  AgwMsgConstituentStockUnit 
{
public:
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
int64_t qty;   ///< 成交数量
int64_t amt;   ///< 成交金额
int64_t price;   ///< 成交价格
uint8_t etf_trade_report_type;   ///< ETF成交回报类型
std::array<char, 32> exec_id;   ///< 执行编号
char margin_amt_type;   ///< 上交所现金替代资金类型
};

// ETF申赎成交回报	2012015
class AgwMsgETFRedemptionTradeER
{
public:
      ///< 费用
uint8_t partition;   ///< 分区号
int32_t index;   ///< 索引号
uint8_t business_type;   ///< 业务类型
int64_t client_order_no;   ///< 客户订单号
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
char exec_type;   ///< 当前委托执行类型
uint8_t order_status;   ///< 当前申报的状态
std::array<char, 16> cust_id;   ///< 客户号ID
std::array<char, 16> fund_account_id;   ///< 资金账户ID
std::array<char, 12> account_id;   ///< 投资者帐号
int64_t price;   ///< 委托价格
int64_t order_qty;   ///< 委托数量
int64_t leaves_qty;   ///< 未成交数量
int64_t cum_qty;   ///< 累计成交数量
char side;   ///< 买卖方向
int64_t transact_time;   ///< 回报时间
std::array<char, 64> user_info;   ///< 用户私有信息
std::array<char, 16> order_id;   ///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id;   ///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
std::string key_error_msg;   ///< 错误信息
std::string key_error_code;   ///< 错误代码串
uint8_t key_error_level;   ///< 错误级别      
std::array<char, 32> exec_id;   ///< 成交编号
int64_t last_px;   ///< 成交价格
int64_t last_qty;   ///< 成交数量
int64_t total_value_traded;   ///< 成交金额
int64_t fee;   ///< 费用      
int64_t client_seq_id;   ///< 用户系统消息序号      
std::vector<AgwMsgConstituentStockUnit> constituent_stock;         ///< 成分股与资金信息
};

// "信用账户普通买入/普通卖出业务委托"	7100701

class  AgwMsgCreditAuctionOrder
{
public:
uint8_t business_type;   ///< 业务类型
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
int64_t client_order_time;   ///< 委托时间
std::array<char, 16> cust_id;   ///< 客户号
std::array<char, 16> fund_account_id;   ///< 资金帐号
std::array<char, 12> account_id;   ///< 账户id
char side;   ///< 买卖方向
char order_type;   ///< 订单类型
int64_t price;   ///< 委托价格
int64_t order_qty;   ///< 申报数量(张)/ETF份额
int64_t client_seq_id;   ///< 用户系统消息序号
std::string client_feature_code;   ///< 终端识别码
char order_way;   ///< 委托方式
std::array<char, 64> user_info;   ///< 用户私有信息
std::string password;   ///< 密码
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
uint16_t policy_id;   ///< 策略编号
char enforce_flag;   ///< 强制风险标识
int64_t stop_px;   ///< 止损价
int64_t min_qty;   ///< 最低成交数量
uint16_t max_price_levels;   ///< 最多成交价位数
char time_in_force;   ///< 订单有效期
std::array<char, 2> order_way_ext;   ///< 委托方式扩展
};

// "信用账户普通买入/普通卖出成交回报"	7200715

class AgwMsgCreditAuctionTradeER 
{
uint8_t partition;   ///< 分区号
int32_t index;   ///< 索引号
uint8_t business_type;   ///< 业务类型
int64_t client_order_no;   ///< 客户订单号
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
char exec_type;   ///< 当前委托执行类型
uint8_t order_status;   ///< 当前申报的状态
std::array<char, 16> cust_id;   ///< 客户号ID
std::array<char, 16> fund_account_id;   ///< 资金账户ID
std::array<char, 12> account_id;   ///< 投资者帐号
int64_t price;   ///< 委托价格
int64_t order_qty;   ///< 委托数量
int64_t leaves_qty;   ///< 未成交数量
int64_t cum_qty;   ///< 累计成交数量
char side;   ///< 买卖方向
int64_t transact_time;   ///< 回报时间
std::array<char, 64> user_info;   ///< 用户私有信息
std::array<char, 16> order_id;   ///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id;   ///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
std::string key_error_msg;   ///< 错误信息
std::string key_error_code;   ///< 错误代码串
uint8_t key_error_level;   ///< 错误级别    
std::array<char, 32> exec_id;   ///< 成交编号
int64_t last_px;   ///< 成交价格
int64_t last_qty;   ///< 成交数量
int64_t total_value_traded;   ///< 成交金额
int64_t fee;   ///< 费用    
int64_t client_seq_id;   ///< 用户系统消息序号    
};


//"融资买入/融券卖出业务委托消息"	7101701

class  AgwMsgMarginTradingOrder {

public:
uint8_t business_type;   ///< 业务类型
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
int64_t client_order_time;   ///< 委托时间
std::array<char, 16> cust_id;   ///< 客户号
std::array<char, 16> fund_account_id;   ///< 资金帐号
std::array<char, 12> account_id;   ///< 账户id
char side;   ///< 买卖方向
char order_type;   ///< 订单类型
int64_t price;   ///< 委托价格
int64_t order_qty;   ///< 申报数量(张)/ETF份额
int64_t client_seq_id;   ///< 用户系统消息序号
std::string client_feature_code;   ///< 终端识别码
char order_way;   ///< 委托方式
std::array<char, 64> user_info;   ///< 用户私有信息
std::string password;   ///< 密码
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
uint16_t policy_id;   ///< 策略编号
char enforce_flag;   ///< 强制风险标识
int64_t stop_px;   ///< 止损价
int64_t min_qty;   ///< 最低成交数量
uint16_t max_price_levels;   ///< 最多成交价位数
char time_in_force;   ///< 订单有效期
std::array<char, 2> order_way_ext;   
};

//"融资买入/融券卖出成交回报"	7201715
class  AgwMsgMarginTradingTradeER 
{
public:
uint8_t partition;   ///< 分区号
int32_t index;   ///< 索引号
uint8_t business_type;   ///< 业务类型
int64_t client_order_no;   ///< 客户订单号
std::array<char, 8> security_id;   ///< 证券代码
uint16_t market_id;   ///< 市场ID
char exec_type;   ///< 当前委托执行类型
uint8_t order_status;   ///< 当前申报的状态
std::array<char, 16> cust_id;   ///< 客户号ID
std::array<char, 16> fund_account_id;   ///< 资金账户ID
std::array<char, 12> account_id;   ///< 投资者帐号
int64_t price;   ///< 委托价格
int64_t order_qty;   ///< 委托数量
int64_t leaves_qty;   ///< 未成交数量
int64_t cum_qty;   ///< 累计成交数量
char side;   ///< 买卖方向
int64_t transact_time;   ///< 回报时间
std::array<char, 64> user_info;   ///< 用户私有信息
std::array<char, 16> order_id;   ///< 交易所订单编号, 取值为数字
std::array<char, 10> cl_ord_id;   ///< 申报合同号,上交所：以QP1开头,表示为交易所保证金强制平仓；以CV1开头,表示为交易所备兑强制平仓；
std::array<char, 10> branch_id;   ///< origid+营业部ID,各占5个字节
std::string key_error_msg;   ///< 错误信息
std::string key_error_code;   ///< 错误代码串
uint8_t key_error_level;   ///< 错误级别
std::array<char, 32> exec_id;   ///< 成交编号
int64_t last_px;   ///< 成交价格
int64_t last_qty;   ///< 成交数量
int64_t total_value_traded;   ///< 成交金额
int64_t fee;   ///< 费用
int64_t client_seq_id;   ///< 用户系统消息序号
};
