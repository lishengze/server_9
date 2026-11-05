// test_order_dispatch - 委托入口合一后三柜台支持域判定单测（逻辑 seam）
//
// 只测三个柜台的公共静态 check_order_domain(req, inst_mkt)（side×市场×类型 → 错误码），
// 不触碰会话缓存/队列/链接（那些是时序 seam，联调覆盖）。
// 口径来源：方案纸 claude-task/2026-11-04-order_insert委托合一.md
// （问号 ?1~?6 人答；2026-11-04 E9 人裁：qty/price 一律交柜台服务端裁决，
//  三柜台域判定不再查数量/价格，唯 fpga 保留 qty%100 整除映射拒绝）。

#include "counter98.h"
#include "fpga_counter_base.h"
#include "gw_counter_direct.h"

#include <cstdio>
#include <cstring>

using namespace lb_api;

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

/// 造一笔各字段可指定的委托
static OrderReq make_order(char side, int16_t market, char order_type, int64_t qty, int64_t price)
{
  OrderReq req;
  std::memset(&req, 0, sizeof(req));
  req.side = side;
  req.market_type = market;
  req.order_type = order_type;
  req.order_qty = qty;
  req.order_price = price;
  return req;
}

// ---- 98 柜台：全业务，市场 = 实例市场 ∪ 北交所(仅深实例、仅买卖) ----
static void test_counter98_domain()
{
  const int16_t SZ = MARKET_TYPE_SZ;
  const int16_t SH = MARKET_TYPE_SH;

  // 本市场全 side 放行（假定1: 新业务 price 不检；?2: 市价透传）
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_OK, "98 深实例现货买");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_SELL, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_OK, "98 深实例现货卖");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_REPO, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 回购 price=0 透传(假定1)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_ANTI_REPO, SZ, ORDER_TYPE_JDSC, 100, 0), SZ),
           LBAPI_OK, "98 逆回购市价透传(?2)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_CALL, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 认购(假定1)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_PLEDGE_IN, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 入库(假定1)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_PLEDGE_OUT, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 出库(假定1)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_SUBSCRIPTION, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 配股(假定1)");

  // ETF 申赎: 数量/价格交柜台服务端裁决, 域判定放行 (E9 人裁)
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_PURCHASE, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 申赎 price=0 放行");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_REDEEM, SZ, ORDER_TYPE_FIX, 100, -1), SZ),
           LBAPI_OK, "98 赎回 price<0 透传(交柜台)");

  // 北交所: 仅深实例、仅买卖 (?4 + 假定2)
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_OK, "98 深实例北交所买");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SH),
           LBAPI_ERR_UNSUPPORTED_OP, "98 沪实例北交所买(?4)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_REPO, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "98 北交所回购(假定2)");
  // 交叠区优先级钉死: 北交所+非买卖+非深实例, side 规则先于实例规则 → INVALID_PARAM
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_REPO, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SH),
           LBAPI_ERR_INVALID_PARAM, "98 沪实例北交所回购(side 规则优先)");

  // 沪实例本市场放行路径
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, SH, ORDER_TYPE_FIX, 100, 100000), SH),
           LBAPI_OK, "98 沪实例现货买");

  // 市场: 异市场拒 (假定4 98 终裁)
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, SH, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "98 深实例报沪市场");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, 0, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "98 市场空");

  // side 防线; 数量/价格交柜台 (E9 人裁, qty=0/price=0 放行)
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_ALL, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "98 side='0'所有(假定3)");
  CHECK_EQ(counter98::check_order_domain(make_order('X', SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "98 side 乱值");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 0, 100000), SZ),
           LBAPI_OK, "98 qty=0 透传(交柜台)");
  CHECK_EQ(counter98::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "98 买卖 price=0 透传(交柜台)");
}

// ---- 个微柜台: 仅买卖+ETF 申赎, 市场==实例 ----
static void test_gw_domain()
{
  const int16_t SZ = MARKET_TYPE_SZ;

  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_OK, "个微现货买");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_PURCHASE, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "个微申赎 price=0");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_REDEEM, SZ, ORDER_TYPE_JDSC, 100, 0), SZ),
           LBAPI_OK, "个微赎回市价透传(?2)");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_REPO, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "个微回购不支持");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_ALL, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "个微 side='0'所有不降级(假定3)");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order('X', SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "个微 side 乱值不降级");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_SUBSCRIPTION, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "个微配股不支持");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "个微北交所不支持(假定4)");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_SH, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "个微微实例报沪");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 0, 100000), SZ),
           LBAPI_OK, "个微 qty=0 透传(交柜台)");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "个微买卖 price=0 透传(交柜台)");
  CHECK_EQ(gw_counter_direct::check_order_domain(make_order(ORDER_SIDE_PURCHASE, SZ, ORDER_TYPE_FIX, 100, -1), SZ),
           LBAPI_OK, "个微申赎 price<0 透传(交柜台)");
}

// ---- FPGA 柜台: 仅买卖+仅限价, 市场==实例 ----
static void test_fpga_domain()
{
  const int16_t SZ = MARKET_TYPE_SZ;

  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_OK, "fpga 现货限价买");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_JDSC, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_TYPE, "fpga 市价 UNSUPPORTED_TYPE");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_SELL, SZ, ORDER_TYPE_FAK, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_TYPE, "fpga 深市FAK 也非限价");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_REPO, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "fpga 回购 UNSUPPORTED_OP");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_ALL, SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "fpga side='0'所有不降级(假定3)");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order('X', SZ, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "fpga side 乱值不降级");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_PURCHASE, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "fpga 申赎 UNSUPPORTED_OP");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_BSE, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "fpga 北交所(假定4)");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, MARKET_TYPE_SH, ORDER_TYPE_FIX, 100, 100000), SZ),
           LBAPI_ERR_UNSUPPORTED_OP, "fpga 深实例报沪");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 0, 100000), SZ),
           LBAPI_OK, "fpga qty=0 透传(交柜台)");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 150, 100000), SZ),
           LBAPI_ERR_INVALID_PARAM, "fpga qty 非100整倍拒(g1 整型股映射)");
  CHECK_EQ(fpga_counter_base::check_order_domain(make_order(ORDER_SIDE_BUY, SZ, ORDER_TYPE_FIX, 100, 0), SZ),
           LBAPI_OK, "fpga price=0 透传(交柜台)");
}

int main()
{
  test_counter98_domain();
  test_gw_domain();
  test_fpga_domain();
  std::printf("order dispatch domain tests: %d passed, %d failed\n", g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
