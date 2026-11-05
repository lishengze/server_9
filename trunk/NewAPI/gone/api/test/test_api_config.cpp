// test_api_config - counter_type 三值化 + need_multi_cust 配置单测（逻辑 seam）
//
// 只测 api_config 对象：need_multi_cust 属性存取/默认值/copy_from、validate() 组合矩阵。
// 柜台类型用 int 数值字面量（公开契约：
// 0=fixed_98 / 1=gewei / 2=fpga；配置与回调跨 ABI 边界传的就是 int 值，
// 钉数值即钉兼容——不引用枚举名，旧代码上可编译，红绿切换只看断言）。
// 注：tcpdirect 用例按无 HAS_TCPDIRECT 的 socket-only 编译断 UNSUPPORT_LINK。

#include "api_config.h"
#include "api_errno.h"

#include <cstdio>
#include <cstring>

using namespace lb_api;

/// need_multi_cust 属性名按字符串字面量引用（attr 名是跨边界的字符串契约；
/// 实现前 NEED_MULTI_CUST 常量尚不存在，字面量让本测试在旧代码上可编译）
static constexpr const char *NEED_MULTI_CUST = "need_multi_cust";

static int g_failed = 0;
static int g_passed = 0;

#define CHECK_EQ(actual, expected, msg)                                        \
  do                                                                           \
  {                                                                            \
    if ((actual) == (expected))                                                \
    {                                                                          \
      ++g_passed;                                                              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      ++g_failed;                                                              \
      std::printf("FAIL: %s (line %d): expect %d, got %d\n", msg, __LINE__,    \
                  static_cast<int>(expected), static_cast<int>(actual));       \
    }                                                                          \
  } while (0)

/// 填齐 validate() 要求的全部必填项，留柜台类型/多客户/链接类型三个变量
static api_config *make_full_cfg(int fast_ct, int need_multi, int slt)
{
  api_config *cfg = api_config::create_config();
  CHECK_EQ(cfg->set_attr(config_name::fast_counter_type, static_cast<int32_t>(fast_ct)), LBAPI_OK, "set fast_counter_type");
  CHECK_EQ(cfg->set_attr(NEED_MULTI_CUST, static_cast<int32_t>(need_multi)), LBAPI_OK, "set need_multi_cust");
  CHECK_EQ(cfg->set_attr(config_name::speed_link_type_name, static_cast<int32_t>(slt)), LBAPI_OK, "set speed_link_type");
  CHECK_EQ(cfg->set_attr(config_name::market_type, static_cast<int8_t>(101)), LBAPI_OK, "set market_type");
  CHECK_EQ(cfg->set_attr(config_name::speed_counter_addr, net_addr{"10.1.1.1", 33001}), LBAPI_OK, "set speed_counter_addr");
  CHECK_EQ(cfg->set_attr(config_name::counter98_addr, net_addr{"10.1.1.2", 9001}), LBAPI_OK, "set counter98_addr");
  CHECK_EQ(cfg->set_attr(config_name::agw98_user, "agw_user_01"), LBAPI_OK, "set agw98_user");
  CHECK_EQ(cfg->set_attr(config_name::agw98_user_password, "agw_pwd_01"), LBAPI_OK, "set agw98_user_password");
  CHECK_EQ(cfg->set_attr(config_name::api_instance_name, "test_api_config"), LBAPI_OK, "set api_instance_name");
  return cfg;
}

static void test_need_multi_cust_attr()
{
  // 默认 0（不支持多客户）
  api_config *cfg = api_config::create_config();
  int32_t v = -1;
  CHECK_EQ(cfg->get_attr(NEED_MULTI_CUST, v), LBAPI_OK, "get need_multi_cust default");
  CHECK_EQ(v, 0, "need_multi_cust default value");
  // 设 1 回读 1；设回 0 回读 0
  CHECK_EQ(cfg->set_attr(NEED_MULTI_CUST, static_cast<int32_t>(1)), LBAPI_OK, "set need_multi_cust=1");
  CHECK_EQ(cfg->get_attr(NEED_MULTI_CUST, v), LBAPI_OK, "get need_multi_cust after set");
  CHECK_EQ(v, 1, "need_multi_cust set/get value");
  CHECK_EQ(cfg->set_attr(NEED_MULTI_CUST, static_cast<int32_t>(0)), LBAPI_OK, "set need_multi_cust=0");
  CHECK_EQ(cfg->get_attr(NEED_MULTI_CUST, v), LBAPI_OK, "get need_multi_cust after reset");
  CHECK_EQ(v, 0, "need_multi_cust reset value");
  api_config::destroy_config(cfg);

  // copy_from 带走 need_multi_cust
  api_config *src = make_full_cfg(1, 1, 1);
  api_config *dst = api_config::create_config();
  CHECK_EQ(dst->copy_from(*src), LBAPI_OK, "copy_from");
  int32_t dv = -1;
  CHECK_EQ(dst->get_attr(NEED_MULTI_CUST, dv), LBAPI_OK, "get copied need_multi_cust");
  CHECK_EQ(dv, 1, "copied need_multi_cust value");
  api_config::destroy_config(src);
  api_config::destroy_config(dst);
}

static void test_validate_matrix()
{
  struct case_t
  {
    const char *name;
    int fast_ct;   // 0=fixed_98 / 1=gewei / 2=fpga
    int need_multi;
    int slt;       // 0=socket_shared / 1=socket_single / 2=tcpdirect
    int expected;  // 期望 validate() 返回码
  };
  const case_t cases[] = {
      {"fixed_98 拒绝", 0, 0, 1, LBAPI_ERR_UNSUPPORT_COUNTER},
      {"fixed_98 多客户也拒绝", 0, 1, 1, LBAPI_ERR_UNSUPPORT_COUNTER},
      {"gewei 单客户+独占socket 通过", 1, 0, 1, LBAPI_OK},
      {"gewei 单客户+共享socket 拒绝", 1, 0, 0, LBAPI_ERR_CFG_INVALID},
      {"gewei 多客户+独占socket 通过", 1, 1, 1, LBAPI_OK},
      {"gewei 多客户+共享socket 拒绝", 1, 1, 0, LBAPI_ERR_CFG_INVALID},
      {"gewei 单客户+tcpdirect 无环境拒绝", 1, 0, 2, LBAPI_ERR_UNSUPPORT_LINK},
      {"fpga 单客户+独占socket 通过", 2, 0, 1, LBAPI_OK},
      {"fpga 单客户+共享socket 拒绝(直连禁共享)", 2, 0, 0, LBAPI_ERR_CFG_INVALID},
      {"fpga 多客户+共享socket 通过(网关模式)", 2, 1, 0, LBAPI_OK},
      {"fpga 多客户+独占socket 拒绝(网关必共享)", 2, 1, 1, LBAPI_ERR_CFG_INVALID},
      {"fpga 多客户+tcpdirect 拒绝(网关必共享)", 2, 1, 2, LBAPI_ERR_CFG_INVALID},
  };
  for (const case_t &c : cases)
  {
    api_config *cfg = make_full_cfg(c.fast_ct, c.need_multi, c.slt);
    CHECK_EQ(cfg->validate(), c.expected, c.name);
    api_config::destroy_config(cfg);
  }
}

int main()
{
  test_need_multi_cust_attr();
  test_validate_matrix();
  std::printf("%s: %d passed, %d failed\n", g_failed == 0 ? "PASS" : "RED",
              g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
