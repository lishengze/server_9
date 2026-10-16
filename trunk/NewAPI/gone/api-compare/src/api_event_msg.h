#pragma once

#include "comm_sys.h"
#include "hash_comm.h"
#include "matomic.h"

#include "g1msghead.h"
#include "order_trade_type.h"
#include "api_errno.h"

#include <cstdint>
#include <cstring>

namespace lb_api
{

  using lb_common::int16;
  using lb_common::int32;
  using lb_common::int64;
  using lb_common::int8;
  using lb_common::uint16;
  using lb_common::uint32;
  using lb_common::uint64;
  using lb_common::uint8;

  /// ---- api 内部字段长度定义 (多柜台支持: 取各柜台协议最大宽度) ----
  /// 与 api 对外接口一致的直接引用对外常量 (order_trade_type.h 的 MAX_*_LEN);
  /// 大于对外宽度的取柜台协议定义 (branch 取 fpga 的 G1_BRANCHID_LEN=12, err_msg 取 98 临时协议 128)
  static constexpr int32_t INNER_CUST_ID_LEN = MAX_CUST_ID_LEN;                 ///< 客户号 (各层均 16)
  static constexpr int32_t INNER_FUND_ACCOUNT_ID_LEN = MAX_FUND_ACCOUNT_ID_LEN; ///< 资金账号 (各层均 16)
  static constexpr int32_t INNER_ACCOUNT_ID_LEN = MAX_ACCOUNT_ID_LEN;           ///< 股东账号 (各层均 12)
  static constexpr int32_t INNER_BRANCH_ID_LEN = G1_BRANCHID_LEN;               ///< 分支机构 (fpga 12 > 对外 10)
  static constexpr int32_t INNER_PASSWORD_LEN = MAX_PASSWORD_LEN;               ///< 密码 (98 256; fpga 协议无)
  static constexpr int32_t INNER_USER_INFO_LEN = MAX_USER_INFO_LEN;             ///< 用户私有信息 (98 64; fpga 协议无)
  static constexpr int32_t INNER_FEATURE_CODE_LEN = MAX_FEATURE_CODE_LEN;       ///< 终端信息 (各层均 1024)
  static constexpr int32_t INNER_ERR_MSG_LEN = 128;                             ///< 错误信息 (98 128 = fpga 128 > 对外 124)
  static constexpr int32_t INNER_SECURITY_ID_LEN = MAX_SECURITY_ID_LEN;         ///< 证券代码 (各层均 8)
  static constexpr int32_t INNER_EXEC_ID_LEN = MAX_EXEC_ID_LEN;                 ///< 成交编号 (= 98 ATP ExecID 32, 宽于 fpga 16)
  static constexpr int32_t INNER_AGW_USER_LEN = MAX_AGW_USER_LEN;               ///< agw 用户名 (c98 32)
  static constexpr int32_t INNER_SESSION_LEN = G1_SESSION_LEN;                  ///< agw 会话号 (98/fpga 均 32)
  static constexpr int32_t INNER_IP_ADDR_LEN = G1_IPADDR_LEN;                   ///< ip 地址 (fpga 16; 对外配置 net_addr.ip 同为 16 宽, 用途不同)
  static constexpr int32_t INNER_VERSION_LEN = G1_VERSION_LEN;                  ///< 协议版本 (98/fpga 均 64)

  /// 柜台账户键 (客户资金账号 + 分支机构代码, 定长 16+16 字节)
  /// 跨柜台公共 (fpga 网关客户表 / 98 账户登录集合 等按此键索引)
  struct fpga_fundacc_key
  {
    char fund_account_id[INNER_FUND_ACCOUNT_ID_LEN]; ///< 客户资金账号
    char branch_id[16];                              ///< 分支机构代码 (协议 BRANCHID_LEN=12, 此处对齐到 16)

    /// 默认构造 (全零初始化: operator== 按全宽 64-bit 块比较, 不清零会比较未定义尾部)
    fpga_fundacc_key()
    {
      std::memset(fund_account_id, 0, sizeof(fund_account_id));
      std::memset(branch_id, 0, sizeof(branch_id));
    };
    /// 拷贝构造 (按 64-bit 块复制, 避免逐字节)
    fpga_fundacc_key(const fpga_fundacc_key &src)
    {
      *((uint64_t *)(fund_account_id)) = *((uint64_t *)(src.fund_account_id));
      *((uint64_t *)(&(fund_account_id[8]))) = *((uint64_t *)(&(src.fund_account_id[8])));
      *((uint64_t *)(branch_id)) = *((uint64_t *)(src.branch_id));
      *((uint64_t *)(&(branch_id[8]))) = *((uint64_t *)(&(src.branch_id[8])));
    }
    /// 字符串构造
    fpga_fundacc_key(const char *fund_account, const char *branch)
    {
      int32_t i = 0;
      while (i < INNER_FUND_ACCOUNT_ID_LEN && fund_account[i] != '\0')
      {
        fund_account_id[i] = fund_account[i];
        i++;
      }
      while (i < INNER_FUND_ACCOUNT_ID_LEN)
      {
        fund_account_id[i] = '\0';
        i++;
      }
      i = 0;
      while (i < INNER_BRANCH_ID_LEN && branch[i] != '\0')
      {
        branch_id[i] = branch[i];
        i++;
      }
      while (i < 16)
      {
        branch_id[i] = '\0';
        i++;
      }
    }
    fpga_fundacc_key(const char *fund_acc, int32_t fund_len, const char *branch, int32_t branch_len)
    {
      int32_t i = 0;
      while (i < fund_len && fund_acc[i] != '\0')
      {
        fund_account_id[i] = fund_acc[i];
        i++;
      }
      while (i < INNER_FUND_ACCOUNT_ID_LEN)
      {
        fund_account_id[i] = '\0';
        i++;
      }
      i = 0;
      while (i < branch_len && branch[i] != '\0')
      {
        branch_id[i] = branch[i];
        i++;
      }
      while (i < 16)
      {
        branch_id[i] = '\0';
        i++;
      }
    }

    /// 拷贝赋值
    fpga_fundacc_key &operator=(const fpga_fundacc_key &src)
    {
      *((uint64_t *)(fund_account_id)) = *((uint64_t *)(src.fund_account_id));
      *((uint64_t *)(&(fund_account_id[8]))) = *((uint64_t *)(&(src.fund_account_id[8])));
      *((uint64_t *)(branch_id)) = *((uint64_t *)(src.branch_id));
      *((uint64_t *)(&(branch_id[8]))) = *((uint64_t *)(&(src.branch_id[8])));
      return *this;
    }
    /// 相等比较 (按 64-bit 块)
    bool operator==(const fpga_fundacc_key &src) const
    {
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
    ~fpga_fundacc_key() {};
  };

  /// 柜台账户键 hash 函数 (按 fund_account_id + branch_id 哈希)
  struct fpga_fundacc_hash
  {
    uint64_t operator()(const fpga_fundacc_key &key) const noexcept
    {
      uint64_t kl = lb_common::hash_fm8<16>(key.fund_account_id);
      uint64_t kh = lb_common::hash_fm8<16>(key.branch_id);
      return kh ^ kl; ///< XOR 合并 (原 OR 只置位不消位, 分布弱化; 运行期建桶无持久化兼容问题)
    }
  };

#define LINK_EVENT_TYPE_SEND_MSG 0          ///< 发送消息
#define LINK_EVENT_TYPE_SEND_HEART 1        ///< 发送心跳
#define LINK_EVENT_TYPE_LINK_CLOSE 2        ///< 链接关闭
#define LINK_EVENT_TYPE_LINK_CONNECT 3      ///< 连接/重连
#define LINK_EVENT_TYPE_ACCOUNT_LOGIN 4     ///< 账户登陆请求
#define LINK_EVENT_TYPE_FPGA_CORE_CONNECT 5 ///< 发起 fpga core 链接（fast_engine 同步 connect, 结果同步返回, 无应答类事件）
#define LINK_EVENT_TYPE_AGWUSER_LOGIN 6     ///< agw用户登陆请求

  /// 链接类型常量（按逻辑角色分类，不再用 idx）
  ///
  /// 命名约定：
  ///   - link_type 表达"什么类型的链接"
  ///   - 物理槽位是 engine 内部细节，外部用 link_type
  ///   - 引擎内部维护 type→slot 映射
  ///
  /// 三种类型（互不重复值）：
  ///   - LINK_TYPE_98          : 98 柜台链接（唯一）
  ///   - LINK_TYPE_SPEED_TRADE : 极速交易链接
  ///                           · 个微：个微业务链接（fast_engine 或 multi 槽 1）
  ///                           · fpga：fpga core 链接（fast_engine，fpga direct 模式专用）
  ///   - LINK_TYPE_SPEED_GW    : 极速网关链接（fpga 模式专用）
  ///                           · fpga direct：fpga GW 链接（登录/证券信息）
  ///                           · fpga gateway：fpga GW 链接（登录/证券信息/业务）
  ///
  /// 命名风格参考 fpga 柜台类（fpga_counter_direct / fpga_counter_gateway）
  constexpr int16_t LINK_TYPE_98 = 0;          ///< 98 柜台链接
  constexpr int16_t LINK_TYPE_SPEED_TRADE = 1; ///< 极速交易链接
  constexpr int16_t LINK_TYPE_SPEED_GW = 2;    ///< 极速网关链接
  constexpr int16_t LINK_TYPE_MAX = 3;         ///< 链接类型总数占位

  /// 客户登录状态字典 (fpga_cust_info.login_state / counter98 cust_login_map_ val / gw_counter_direct.login_state)
  static constexpr int32_t LOGIN_STATE_NONE = 0;   ///< 未登录(从未)
  static constexpr int32_t LOGIN_STATE_LOGOUT = 1; ///< 已登出(曾成功后登出)
  static constexpr int32_t LOGIN_STATE_OK = 2;     ///< 登录成功(保持至用户主动登出; 单客户模式绑定过账户后不支持换账户登录)
  static constexpr int32_t LOGIN_STATE_MASK = 3;   ///< LOGIN_STATE_LOGOUT | LOGIN_STATE_OK

  /// 账户登录在途标记字典 (log_state_ctl 位标志, 与登录状态位组合编码在同一原子字内)
  static constexpr int32_t LOGIN_PENDING_USER = 0x10;    ///< 用户在途(应答成功回调一次, 失败清状态回调)
  static constexpr int32_t LOGIN_PENDING_RELOGIN = 0x20; ///< 重连(内部)重登在途(成功静默; 失败清状态+回调+不再自动重登, 不关链)
  static constexpr int32_t LOGIN_PENDING_MASK = 0x30;    ///< LOGIN_PENDING_USER | LOGIN_PENDING_RELOGIN
  /// 登录控制状态机
  struct log_state_ctl
  {
    int32_t log_state;

    // 用户登出，检查设置 logout
    // 返回 ：1-设置成功 ， 0-原来就未登录，<0 - 存在在途请求
    int32_t to_logout()
    {
      do
      {
        int32_t t = lb_common::atomic_load32(&log_state);
        if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER) //!= 0)
          return LBAPI_ERR_STATE_LIMITED;
        if ((t & LOGIN_STATE_MASK) != LOGIN_STATE_OK)
          return 0;

        if (lb_common::atomic_cas32(&log_state, &t, LOGIN_STATE_LOGOUT))
          return 1;
      } while (true);
    }

    // 是否为初始
    bool is_free()
    {
      int32_t t = lb_common::atomic_load32(&log_state);
      return t == 0;
    }

    // 是否登录成功，如链接建立，是否重登（fpga 还有用户是否下线相关）
    bool is_logined()
    {
      int32_t t = lb_common::atomic_load32(&log_state);
      return ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK);
    }

    // 处理登录事件，检查设置
    // 该函数保证同一客户只有一个在途登录请求到后台
    // o_havelog , 返回是否为登录状态 (仅返回 0 时有意义: 1=幂等回调用户登录成功)
    // 返回 1- 发到后台 ， 0-终止
    int32_t to_login(int32_t &o_havelog, int32_t is_user_login)
    {
      do
      {
        int32_t t = lb_common::atomic_load32(&log_state);
        if (is_user_login == 1)
        {
          if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK)
          {
            // 用户重复已经登录成功，直接返回成功，不需发到后台
            // o_havelog = 1 , 回调用户登录成功
            o_havelog = 1;
            return 0;
          }
          if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER)
          {
            // 用户重复在途登录，合并，不发送后台，等待应答
            o_havelog = 0;
            return 0;
          }
          if ((t & LOGIN_PENDING_MASK) == 0)
          {
            // 无在途: 占位用户在途并发包 (状态位置 NONE, LOGOUT 位顺带清除
            // —— 用户并发调用登录登出的窗口本就极小)
            if (lb_common::atomic_cas32(&log_state, &t, LOGIN_PENDING_USER))
            {
              o_havelog = 0;
              return 1;
            }
          }
          else
          {
            // 防御分支(正常不可达: RELOGIN 仅与 OK 共存, OK 已被上面拦截):
            // 重连已经在途登录，合并改用户在途，不发送后台，等待应答
            if (lb_common::atomic_cas32(&log_state, &t, LOGIN_PENDING_USER))
            {
              o_havelog = 0;
              return 0;
            }
          }
        }
        else
        {
          if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK)
          {
            o_havelog = 1;
          }
          else
          {
            // 链接触发的登录，非登录成功状态，静默终止登录
            o_havelog = 0;
            return 0;
          }
          if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER)
          {
            // 存在用户在途登录，合并，不发送后台
            o_havelog = 0;
            return 0;
          }
          int32_t n = ((t & LOGIN_STATE_MASK) | LOGIN_PENDING_RELOGIN);
          if (lb_common::atomic_cas32(&log_state, &t, n))
          {
            return 1;
          }
        }
      } while (true);
    }

    // 链接关闭或异步处理失败, 清除pending
    // 返回原来的在途登录：0-无在途，1-用户在途，2-重登在途
    // 由于重连重登时，后台登录失败，回调客户，所以异步处理失败，也应回调客户
    int32_t clear_pending()
    {
      do
      {
        int32_t t = lb_common::atomic_load32(&log_state);
        if ((t & LOGIN_PENDING_MASK) == 0)
          return 0;
        int32_t ret;
        if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER)
        {
          ret = 1;
        }
        else
        {
          ret = 2;
        }
        int32_t n = (t & LOGIN_STATE_MASK);
        if (lb_common::atomic_cas32(&log_state, &t, n))
          return ret;
      } while (true);
    }

    // 后台登录应答成功，设置状态
    // 返回：1- 回调用户成功，0- 不回调用户
    int32_t ans_login_ok()
    {
      do
      {
        int32_t t = lb_common::atomic_load32(&log_state);
        int32_t ret = ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER);
        if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_LOGOUT)
        {
          // 用户主动登出了,LOGOUT 不可能有用户pending,不处理
          return ret;
        }
        if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_OK)
        {
          // 已经是登录成功状态，之前已经回调过，不再回调
          ret = 0;
        }
        // 清除 pending ,设置登录成功
        if (lb_common::atomic_cas32(&log_state, &t, LOGIN_STATE_OK))
          return ret;
      } while (true);
    }

    // 后台登录应答失败，设置状态
    // 返回：1- 回调用户失败，0- 不回调用户
    // (USER→清状态 NONE 回调; RELOGIN→清状态 NONE 回调[曾成功客户重登被拒视为异常, 不再自动重登];
    //  无在途迟到/LOGOUT 后迟到→忽略)
    int32_t ans_login_err()
    {
      do
      {
        int32_t t = lb_common::atomic_load32(&log_state);

        if ((t & LOGIN_STATE_MASK) == LOGIN_STATE_LOGOUT)
        {
          // 用户主动登出了,LOGOUT 不可能有用户pending,不处理
          return 0;
        }
        // 后台应答错误，可能无 pending，比如重连重登应答前，用户调用logout，又立即login
        // 此时，有两个在途请求，第二个可能失败，此时pending 被清除，不需回调用户，用户的login 取决于第一个应答。
        if ((t & LOGIN_PENDING_MASK) == LOGIN_PENDING_USER)
        {
          if (lb_common::atomic_cas32(&log_state, &t, 0))
            return 1;
        }
        else if ((t & LOGIN_PENDING_MASK) == 0)
        {
          return 0;
        }
        else
        {
          // 客户已登录成功后的重连登录，正常情况后台不应该失败，
          // 所以这种情况下失败，也清除登录成功,不再自动重连重新登录，回调通知客户，由客户处理或重新登录
          if (lb_common::atomic_cas32(&log_state, &t, 0))
            return 1;
        }
      } while (true);
    }
  };

  /// 链接发送事件 (柔性数组, 写入时按 total = sizeof(link_send_event) + data_len 申请)
  struct link_send_event
  {
    int16 link_type; ///< 目标链接类型 (LINK_TYPE_98 / LINK_TYPE_SPEED_TRADE / LINK_TYPE_SPEED_GW)
    int16 type;      ///< 事件类型
    int32 data_len;  ///< 数据长度 (心跳/关闭/重连/链接事件时为 0)
    char data[0];    ///< 数据柔性数组
  };

  /// 账户登录事件信息 (从 c98.deal_log_req 转换得到, 投递到对应引擎)
  struct acc_login_event_info
  {
    int64_t cust_req_no;                              ///< 客户私有请求号
    char cust_id[INNER_CUST_ID_LEN];                  ///< 客户号
    char fund_account_id[INNER_FUND_ACCOUNT_ID_LEN];  ///< 客户资金账号
    char branch_id[INNER_BRANCH_ID_LEN];              ///< 分支机构代码 (内部取 fpga 12 字节宽度)
    char account_id[INNER_ACCOUNT_ID_LEN];            ///< 客户股东账号
    char order_way_ext[2];                            ///< 客户委托方式
    char session[INNER_SESSION_LEN];                  ///< agw 用户登陆返回的会话号
    char password[INNER_PASSWORD_LEN];                ///< 密码
    char user_info[INNER_USER_INFO_LEN];              ///< 用户私有信息
    char client_feature_code[INNER_FEATURE_CODE_LEN]; ///< 客户终端信息
    int32_t user_origin;                              ///< 登录来源: 0-内部自动(断线重连重登, 应答成功不回调), 1-用户主动 login() (应答成功需回调)
    int32_t filled;
  };

  /// 链接关闭事件信息
  struct link_close_event_info
  {
    int32 err_code; ///< 错误码
    int32 filled;   ///< 占位对齐
  };

  /// 链接连接/重连事件信息
  struct link_connect_event_info
  {
    int32 need_switch; ///< 是否切到备地址 (1=切, 0=不切)
    int32 filled;      ///< 占位对齐
  };

  struct fpga_core_connect_info
  {
    int32_t trade_port;           ///< core 链接端口（从 fpga login_ans 提取）
    char trade_ip[INNER_IP_ADDR_LEN]; ///< core 链接 IP（从 fpga login_ans 提取）
  };

  class link_engine_outop
  {
  public:
    virtual void deal_heart_msg_ans(int16 link_type) {};
    virtual void trigger_send() {};
    virtual void deal_close_link(int16 link_type, int32 err_code) {};
  };

} // namespace lb_api