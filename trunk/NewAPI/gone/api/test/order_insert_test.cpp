#include "order_insert_test_support.h"
#include "api_config_impl.h"
#include "api_interface.h"
#include "counter98.h"
#include "fpga_counter_direct.h"
#include "fpga_counter_gateway.h"
#include "gw_counter_direct.h"
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <arpa/inet.h>

using namespace lb_api;

static int failures = 0;
static int checks = 0;
static void expect(bool ok, const char *scenario)
{
  ++checks;
  if (!ok)
  {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", scenario);
  }
}

void order_test_callback::on_order_rtn(const StreamInfo &, const OrderRtn &r) { orders.push_back(r); }
void order_test_callback::on_trade_rtn(const StreamInfo &, const TradeRtn &r) { trades.push_back(r); }
void order_test_callback::on_order_query_ans(const OrderRtn *r, const QueryAnsCtl &c)
{ queried_orders.insert(queried_orders.end(), r, r + c.count); }
void order_test_callback::on_trade_query_ans(const TradeInfo *r, const QueryAnsCtl &c)
{ queried_trades.insert(queried_trades.end(), r, r + c.count); }
void order_test_callback::on_event(api_event_type, int32_t code, const char *) { errors.push_back(code); }

template <typename Counter>
static void fpga_rejects_unsupported_orders()
{
  api_config_impl cfg;
  cfg.set_attr(config_name::market_type, static_cast<int8_t>(102));
  lb_common::lb_log log;
  log.open_log("/tmp", "gone-order-tests", 0, 0, 0);
  Counter counter;
  counter.init(cfg, nullptr, &log);
  OrderReq req{};
  req.market_type = 102;
  req.side = 'D';
  req.order_type = '2';
  req.order_price = 10000;
  req.order_qty = 10000;
  expect(counter.deal_order_req(req) == -3, "FPGA ETF goes to fallback, even before login");
  req.side = '1';
  req.order_type = '1';
  expect(counter.deal_order_req(req) == -2, "FPGA market buy fails by type, without fallback");
  req.market_type = 109;
  expect(counter.deal_order_req(req) == -3, "BSE market buy is unsupported business on FPGA");
}

// 外部柜台报文与发送队列是测试边界；真实柜台对象、登录缓存和回调管理器全部参与。
template <size_t N>
static void text_field(std::array<char, N> &field, const char *value)
{
  field.fill(' ');
  std::memcpy(field.data(), value, std::min(N, std::strlen(value)));
}

static OrderReq order(char side, int16_t market)
{
  OrderReq req{};
  std::strcpy(req.fund_account_id.data(), "FUND");
  std::strcpy(req.branch_id.data(), "BRANCH");
  std::strcpy(req.security_id.data(), "123456");
  req.market_type = market;
  req.side = side;
  req.order_type = '2';
  req.order_price = 123400;
  req.order_qty = 45600;
  req.stop_price = 120000;
  req.client_seq_id = 12345;
  req.policy_id = 7;
  return req;
}

static std::vector<char> packet98(mp::RootPacket &msg)
{
  mp::SerializeBuffer body;
  expect(msg.PushFields(body, false) == mp::kSuccess, "test counter encodes response");
  std::vector<char> bytes(16 + body.Size() + 4, 0);
  uint32_t type = msg.GetTypeID(), size = body.Size();
  uint16_t version = msg.GetMsgVer();
  std::memcpy(bytes.data(), &type, 4);
  std::memcpy(bytes.data() + 4, &version, 2);
  std::memcpy(bytes.data() + 12, &size, 4);
  std::memcpy(bytes.data() + 16, body.Begin(), body.Size());
  uint32_t checksum = 0;
  for (size_t i = 0; i < bytes.size() - 4; ++i) checksum += static_cast<unsigned char>(bytes[i]);
  checksum = htonl(checksum % 256);
  std::memcpy(bytes.data() + bytes.size() - 4, &checksum, 4);
  return bytes;
}

static void receive98(counter98 &counter, mp::RootPacket &msg)
{
  auto bytes = packet98(msg);
  expect(counter.deal_recv_msg(bytes.data(), bytes.size(), LINK_TYPE_98) == static_cast<int>(bytes.size()),
         "98 consumes complete incoming message");
}

static std::vector<char> take_packet(lb_common::que_mth_buf &queue)
{
  char *data = nullptr;
  auto pos = queue.read_get(data);
  expect(pos > 0, "one request appears in send queue");
  if (pos <= 0) return {};
  auto *event = reinterpret_cast<link_send_event *>(data);
  std::vector<char> bytes(event->data, event->data + event->data_len);
  queue.read_cmt(static_cast<int32_t>(sizeof(link_send_event) + event->data_len));
  return bytes;
}

static void login98(counter98 &counter, lb_common::que_mth_buf &queue, int16_t market)
{
  counter.deal_link_connect(LINK_TYPE_98, 0);
  lb_counter98::AgwMsgLogin agw;
  receive98(counter, agw);
  acc_login_event_info login{};
  std::strcpy(login.fund_account_id, "FUND");
  std::strcpy(login.branch_id, "BRANCH");
  std::strcpy(login.account_id, "ACCOUNT");
  login.user_origin = 1;
  char buf[4096];
  expect(counter.deal_cust_login(login, buf, sizeof(buf)) > 0, "98 accepts customer login event");
  lb_counter98::AgwMsgCustLoginResp response;
  text_field(response.cust_id, "CUSTOMER");
  response.fund_account_array.emplace_back();
  auto &fund = response.fund_account_array.back();
  text_field(fund.fund_account_id, "FUND");
  text_field(fund.branch_id, "BRANCH");
  fund.account_array.emplace_back();
  text_field(fund.account_array.back().account_id, "ACCOUNT");
  fund.account_array.back().market_id = market;
  receive98(counter, response);
  take_packet(queue); // 账户登录成功后交给极速柜台的登录事件。
}

template <typename Message>
static Message sends98(counter98 &counter, lb_common::que_mth_buf &queue,
                       order_test_callback &callback, OrderReq req,
                       uint32_t message_id, uint8_t business, char side, char wire_type = '2')
{
  Message msg;
  int ret = counter.deal_order_req(req);
  expect(ret == 0, "supported 98 business enqueues request");
  if (ret != 0) return msg;
  auto bytes = take_packet(queue);
  if (bytes.size() < 20) return msg;
  uint32_t type = 0;
  std::memcpy(&type, bytes.data(), 4);
  expect(type == message_id, "98 uses business-specific message id");
  expect(msg.PopFields(bytes.data() + 16, bytes.size() - 20, false) == mp::kSuccess, "98 native message decodes");
  expect(msg.business_type == business && msg.side == side, "98 business and side are mapped");
  expect(msg.market_id == req.market_type && msg.order_type == wire_type, "98 market and price type are mapped");
  expect(msg.order_qty == req.order_qty && msg.price == req.order_price && msg.client_seq_id == req.client_seq_id,
         "quantity, price and client sequence retain their units and identity");
  expect(std::string(msg.account_id.data(), 7) == "ACCOUNT", "98 gets shareholder account from login");
  size_t count = callback.orders.size();
  counter.deal_send_error(bytes.data(), bytes.size(), LINK_TYPE_98, -29);
  expect(callback.orders.size() == count + 1, "every 98 business reports an asynchronous send failure");
  if (callback.orders.size() != count)
  {
    const auto &r = callback.orders.back();
    expect(r.side == req.side && r.err_code == -29 && r.order_status == 8,
           "98 failed send restores API side and reports rejection");
  }
  return msg;
}

template <typename Report>
static void reports98(counter98 &counter, order_test_callback &callback, uint8_t business, char wire_side,
                      char api_side, int index)
{
  Report msg;
  msg.partition = 1;
  msg.index = index;
  msg.business_type = business;
  msg.side = wire_side;
  msg.exec_type = 'F';
  msg.last_px = 123400;
  msg.last_qty = 100;
  msg.client_seq_id = 12345;
  size_t count = callback.trades.size();
  receive98(counter, msg);
  expect(callback.trades.size() == count + 1, "native business trade reaches user callback");
  if (callback.trades.size() != count)
    expect(callback.trades.back().side == api_side && callback.trades.back().exec_qty == 100 &&
               callback.trades.back().order_type == 0,
           "native trade restores API business and quantity and leaves type unset");
  receive98(counter, msg);
  expect(callback.trades.size() == count + 1, "duplicate native trade is suppressed");
  msg.index++;
  msg.exec_type = '8';
  count = callback.orders.size();
  receive98(counter, msg);
  expect(callback.orders.size() == count + 1 && callback.orders.back().side == api_side,
         "non-trade business event reaches order callback");
}

static void all_98_businesses(int16_t market)
{
  using namespace lb_counter98;
  lb_common::lb_log log;
  log.open_log("/tmp", "gone-98-tests", 0, 0, 0);
  order_test_callback cb;
  callback_manager callbacks;
  callbacks.init(&log, &cb, callback_mode::direct, -1, 1, 0);
  api_config_impl cfg;
  cfg.set_attr(config_name::market_type, static_cast<int8_t>(market));
  cfg.set_attr(config_name::counter98_mode, int32_t(1));
  counter98 counter;
  counter.init(cfg, &callbacks, &log);
  lb_common::que_mth_buf queue;
  queue.init(1024 * 1024);
  link_engine_outop output;
  counter.init_gateway(&queue, &output);
  counter.init_trade(&queue, &output);
  login98(counter, queue, market);
  for (char side : {'1', '2'}) sends98<AgwMsgCashAuctionOrder>(counter, queue, cb, order(side, market), 100101, 1, side);
  for (char side : {'3', '4'}) sends98<AgwMsgRepoAuctionOrder>(counter, queue, cb, order(side, market), 100201, 2, side);
  sends98<AgwMsgIssueOrder>(counter, queue, cb, order('9', market), 101301, 13, '9');
  sends98<AgwMsgRightsIssueOrder>(counter, queue, cb, order('S', market), 101401, 14, '9');
  for (char side : {'H', 'I'}) sends98<AgwMsgPledgeInOrOutOrder>(counter, queue, cb, order(side, market), 101901, 19, side);
  for (char side : {'D', 'E'})
  {
    auto req = order(side, market);
    req.order_price = 0;
    auto msg = sends98<AgwMsgETFRedemptionOrder>(counter, queue, cb, req, 101201, 12, side);
    const auto &own = market == 101 ? msg.sh_account_id : msg.sz_account_id;
    const auto &other = market == 101 ? msg.sz_account_id : msg.sh_account_id;
    expect(std::string(own.data(), 7) == "ACCOUNT", "ETF fills current market shareholder account");
    expect(std::all_of(other.begin(), other.end(), [](char c) { return c == ' '; }), "ETF leaves other market account empty");
  }
  sends98<AgwMsgCreditAuctionOrder>(counter, queue, cb, order('T', market), 7100701, 200, '1');
  sends98<AgwMsgCreditAuctionOrder>(counter, queue, cb, order('U', market), 7100701, 200, '2');
  sends98<AgwMsgMarginTradingOrder>(counter, queue, cb, order('V', market), 7101701, 201, '1');
  sends98<AgwMsgMarginTradingOrder>(counter, queue, cb, order('W', market), 7101701, 201, '2');
  const char wire_types[] = {'1', '2', 'K', 'U', '1', '1', '1'};
  for (int i = 0; i < 7; ++i)
  {
    auto req = order('1', market);
    req.order_type = '1' + i;
    if (i != 1) req.order_price = 0;
    sends98<AgwMsgCashAuctionOrder>(counter, queue, cb, req, 100101, 1, '1', wire_types[i]);
  }
  if (market == 102)
    sends98<AgwMsgCashAuctionOrder>(counter, queue, cb, order('1', 109), 100101, 1, '1');
  else
    expect(counter.deal_order_req(order('1', 109)) == -3, "Shanghai instance rejects BSE");
  expect(counter.deal_order_req(order('1', market == 101 ? 102 : 101)) == -1, "wrong instance market is rejected");
  reports98<AgwMsgCashAuctionTradeER>(counter, cb, 1, '1', '1', 1);
  reports98<AgwMsgRepoAuctionTradeER>(counter, cb, 2, '3', '3', 3);
  reports98<AgwMsgPledgeInOrOutTradeER>(counter, cb, 19, 'H', 'H', 5);
  reports98<AgwMsgETFRedemptionTradeER>(counter, cb, 12, 'D', 'D', 7);
  reports98<AgwMsgCreditAuctionTradeER>(counter, cb, 200, '1', 'T', 9);
  reports98<AgwMsgMarginTradingTradeER>(counter, cb, 201, '2', 'W', 11);
  AgwMsgOrderStatusAck ack;
  ack.partition = 2; ack.index = 1; ack.business_type = 201; ack.side = '1'; ack.ord_type = 'U';
  const auto orders_before_ack = cb.orders.size();
  receive98(counter, ack);
  expect(cb.orders.size() == orders_before_ack + 1 && cb.orders.back().side == 'V' &&
             cb.orders.back().order_type == 0,
         "order acknowledgment restores side and leaves type unset");
  AgwMsgExtQueryResultOrder orders;
  orders.order_array.emplace_back();
  orders.order_array.back().business_type = 14;
  orders.order_array.back().side = '9';
  orders.order_array.back().ord_type = 'K';
  receive98(counter, orders);
  expect(!cb.queried_orders.empty() && cb.queried_orders.back().side == 'S',
         "order query restores API side");
  AgwMsgExtQueryResultTradeOrder trades;
  trades.order_array.emplace_back();
  trades.order_array.back().business_type = 200;
  trades.order_array.back().side = '2';
  trades.order_array.back().ord_type = 'U';
  receive98(counter, trades);
  expect(!cb.queried_trades.empty() && cb.queried_trades.back().side == 'U',
         "trade query restores API side");
  expect(cb.errors.empty(), "supported 98 business messages cause no protocol-error events");
  expect(counter.deal_order_req(order('X', market)) == -3, "unconfirmed sell-to-repay is explicitly unsupported");
  expect(counter.deal_order_req(order('0', market)) == -3, "query-all is not a trading operation");
}

static void gewei_businesses()
{
  lb_common::lb_log log;
  log.open_log("/tmp", "gone-gw-tests", 0, 0, 0);
  order_test_callback cb;
  callback_manager callbacks;
  callbacks.init(&log, &cb, callback_mode::direct, -1, 1, 0);
  api_config_impl cfg;
  cfg.set_attr(config_name::market_type, int8_t(102));
  gw_counter_direct counter;
  counter.init(cfg, &callbacks, &log);
  for (char side : {'3','4','9','H','I','S','T','U','V','W','X','0'})
    expect(counter.deal_order_req(order(side,102)) == -3, "gewei rejects unsupported business before connection check");
  expect(counter.deal_order_req(order('1',109)) == -3, "gewei BSE goes to 98");
  lb_common::que_mth_buf queue;
  queue.init(1024*1024);
  link_engine_outop output;
  counter.init_trade(&queue,&output);
  counter.deal_link_connect(LINK_TYPE_SPEED_TRADE,0);
  acc_login_event_info login{};
  std::strcpy(login.fund_account_id,"FUND");
  login.user_origin=1;
  char loginbuf[4096];
  expect(counter.deal_cust_login(login,loginbuf,sizeof(loginbuf))>0,"gewei login starts through public event entry");
  gw_message::LogOnAns answer;
  answer.reset();
  text_field(answer.fund_account_id,"FUND");
  text_field(answer.account_id,"ACCOUNT");
  std::vector<char> bytes(8+sizeof(answer)+4,0);
  uint32_t id=htonl(2001), length=htonl(sizeof(answer));
  std::memcpy(bytes.data(),&id,4); std::memcpy(bytes.data()+4,&length,4);
  answer.encode(bytes.data()+8,sizeof(answer));
  uint32_t sum=0;
  for(size_t i=0;i<bytes.size()-4;++i) sum+=static_cast<unsigned char>(bytes[i]);
  sum=htonl(sum%256); std::memcpy(bytes.data()+bytes.size()-4,&sum,4);
  counter.deal_recv_msg(bytes.data(),bytes.size(),LINK_TYPE_SPEED_TRADE);
  for (char side : {'1', '2', 'D', 'E'})
  {
    const bool etf = side == 'D' || side == 'E';
    const char first_type = etf ? ORDER_TYPE_FIX : ORDER_TYPE_JDSC;
    const char last_type = etf ? ORDER_TYPE_FIX : ORDER_TYPE_FOK;
    for (char type = first_type; type <= last_type; ++type)
    {
      auto req = order(side, 102);
      req.order_type = type;
      const auto ret = counter.deal_order_req(req);
      expect(ret == 0, "gewei supported business enqueues request");
      if (ret != 0) return;

      auto sent = take_packet(queue);
      if (sent.size() < 12) return;
      gw_message::PktNewHeader head;
      head.decode(sent.data(), sent.size());
      expect(head.msg_id == (etf ? 1010u : 1003u), "gewei chooses ETF or cash message");
      gw_message::TradeOrderReq body;
      body.reset();
      body.decode(sent.data() + 8, head.msg_len);
      expect(body.side == side, "gewei passes the requested side");
      if (!etf)
        expect(body.order_type == type, "gewei passes the API cash order type");
      expect(body.order_price == req.order_price && body.order_qty == req.order_qty,
             "gewei passes price and quantity unchanged");
      auto count = cb.orders.size();
      counter.deal_send_error(sent.data(), sent.size(), LINK_TYPE_SPEED_TRADE, -29);
      expect(cb.orders.size() == count + 1 && cb.orders.back().side == side,
             "gewei send failure covers ETF and cash");
    }
  }
}

static void api_routes_orders(int16_t market, counter_type kind, bool multi)
{
  api_config_impl cfg;
  cfg.set_attr(config_name::market_type, static_cast<int8_t>(market));
  cfg.set_attr(config_name::fast_counter_type, static_cast<int32_t>(kind));
  cfg.set_attr(config_name::need_multi_cust, int32_t(multi));
  cfg.set_attr(config_name::speed_link_type_name, int32_t(multi ? 0 : 1));
  cfg.set_attr(config_name::counter98_addr, net_addr{"127.0.0.1", 39101});
  cfg.set_attr(config_name::speed_counter_addr, net_addr{"127.0.0.1", 39102});
  cfg.set_attr(config_name::agw98_user, "offline-test");
  cfg.set_attr(config_name::agw98_user_password, "offline-test");
  cfg.set_attr(config_name::api_instance_name, "gone-order-api-test");
  cfg.set_attr(config_name::log_output_dir, "/tmp");
  order_test_callback cb;
  api_interface *api = nullptr;
  expect(api_interface::create_instance(api, cfg, &cb) == 0, "public API instance initializes offline");
  if (!api) return;
  // 不启动收发线程、不连接外部柜台。请求只到本地支持判定和连接前提检查。
  if (kind == counter_type::fpga)
  {
    auto req = order('1', market);
    req.order_type = '1';
    expect(api->order_insert(req) == -2, "API returns FPGA market order type error");
  }
  expect(api->order_insert(order('1', 109)) == (market == 102 ? -22 : -3),
         "BSE buy checks connection in Shenzhen and is unsupported in Shanghai");
  api_interface::release_instance(api);
}

int main()
{
  fpga_rejects_unsupported_orders<fpga_counter_direct>();
  fpga_rejects_unsupported_orders<fpga_counter_gateway>();
  all_98_businesses(101);
  all_98_businesses(102);
  gewei_businesses();
  for (int16_t market : {101, 102})
  {
    api_routes_orders(market, counter_type::fpga, false);
    api_routes_orders(market, counter_type::fpga, true);
    api_routes_orders(market, counter_type::gewei, false);
  }
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
