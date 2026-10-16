// lb_counter98_protocol.h - 98柜台协议统一引入头文件
//
// 将 counter98 目录下的协议文件统一引入, 并在 lb_counter98 namespace 下
// re-export counter98 代码实现所需的关键类型和常量.
//
// counter98.h / counter98.cpp 只需 include 本文件即可使用全部98协议类型.

#pragma once

// ============================================================================
// 协议基础框架 (mp namespace)
// ============================================================================
#include "serialize_buffer.h"
#include "message_encoder.h"
#include "message_decoder.h"
#include "msg_proto_types.h"
#include "root_packet.h"
#include "error_code.h"
#include "string_util.h"
#include "factory.h"
#include "mp_export.h"
#include "export_macro.h"

// ============================================================================
// 98柜台消息协议定义 (atp::gw namespace)
// ============================================================================
#include "types_agw_external.h"
#include "constants_agw_external.h"
// messages_agw_external.h 体积很大, 但包含所有消息类的完整定义
// 忽略 json 相关部分, 只使用 nonjson 的 Encode/Decode
#include "messages_agw_external.h"

// ============================================================================
// lb_counter98 namespace: 统一 re-export counter98 代码所需类型
// ============================================================================
namespace lb_counter98 {

// ---- mp 框架类型 ----
using mp::RootPacket;
using mp::MessageEncoder;
using mp::MessageDecoder;
using mp::SerializeBuffer;
using mp::ErrorCode;
using mp::error_code; // atp::gw 中 typedef mp::ErrorCode error_code

// ---- 消息头 ----
using atp::gw::AgwHeader;

// ---- 登录/心跳消息 ----
using atp::gw::AgwMsgLogin;
using atp::gw::AgwMsgLogout;
using atp::gw::AgwMsgHeartBeat;
using atp::gw::AgwMsgCustLoginReq;
using atp::gw::AgwMsgCustLoginResp;
using atp::gw::AgwMsgCustLogoutReq;
using atp::gw::AgwMsgCustLogoutResp;

// ---- 委托/撤单/回报消息 ----
using atp::gw::AgwMsgOrder;
using atp::gw::AgwNewMsgOrder;
using atp::gw::AgwMsgCashAuctionOrder;
using atp::gw::AgwMsgCancelOrder;
using atp::gw::AgwMsgOrderReport;
using atp::gw::AgwMsgOrderStatusAck;
using atp::gw::AgwMsgCashAuctionTradeER;
using atp::gw::AgwMsgTradeExecutionReportSeqNum;

// ---- 查询消息 ----
using atp::gw::AgwMsgExtQueryBase;
using atp::gw::AgwMsgExtQueryResultBase;
using atp::gw::AgwMsgExtQueryOrder;
using atp::gw::AgwMsgExtQueryResultOrder;
using atp::gw::AgwOrderArray;
using atp::gw::AgwMsgExtQueryTradeOrder;
using atp::gw::AgwMsgExtQueryResultTradeOrder;
using atp::gw::AgwTradeOrderArray;
using atp::gw::AgwMsgExtQueryFund;
using atp::gw::AgwMsgExtQueryResultFund;
using atp::gw::AgwMsgExtQueryShare;
using atp::gw::AgwMsgExtQueryResultShare;
using atp::gw::AgwMsgExtQueryMaxOrderQty;
using atp::gw::AgwMsgExtQueryMaxOrderQtyResult;
using atp::gw::AgwMsgExternalQueryTotalFundAsset;
using atp::gw::AgwMsgExtQueryResultTotalFundAsset;
using atp::gw::AgwTotalFundAssetArray;
using atp::gw::AgwMsgExtQueryFundMaxdraw;
using atp::gw::AgwMsgExtQueryResultFundMaxdraw;
using atp::gw::AgwFundMaxdrawArray;

// ---- 消息类型号常量 ----
using atp::gw::kAgwMsgLogin;
using atp::gw::kAgwMsgLogout;
using atp::gw::kAgwMsgHeartBeat;
using atp::gw::kAgwMsgCustLoginReq;
using atp::gw::kAgwMsgCustLoginResp;
using atp::gw::kAgwMsgCashAuctionOrder;
using atp::gw::kAgwMsgCancelOrder;
using atp::gw::kAgwMsgOrderStatusAck;
using atp::gw::kAgwMsgCashAuctionTradeER;
using atp::gw::kAgwMsgExtQueryOrder;
using atp::gw::kAgwMsgExtQueryResultOrder;
using atp::gw::kAgwMsgExtQueryTradeOrder;
using atp::gw::kAgwMsgExtQueryResultTradeOrder;
using atp::gw::kAgwMsgExtQueryFund;
using atp::gw::kAgwMsgExtQueryResultFund;
using atp::gw::kAgwMsgExtQueryShare;
using atp::gw::kAgwMsgExtQueryResultShare;
using atp::gw::kAgwMsgExtQueryMaxOrderQty;
using atp::gw::kAgwMsgExtQueryMaxOrderQtyResult;
using atp::gw::kAgwMsgExternalQueryTotalFundAsset;
using atp::gw::kAgwMsgExtQueryResultTotalFundAsset;
using atp::gw::kAgwMsgExtQueryFundMaxdraw;
using atp::gw::kAgwMsgExtQueryResultFundMaxdraw;

// ---- 协议字段类型别名 (常用) ----
using atp::gw::MsgTypeNo_def;
using atp::gw::VersionNo_def;
using atp::gw::BodyLength_def;
using atp::gw::SeqNum_def;
using atp::gw::SeqNumHeader_def;
using atp::gw::Reserver_def;
using atp::gw::ClOrdNO_def;
using atp::gw::FundAccountID_def;
using atp::gw::CustID_def;
using atp::gw::BranchID_def;
using atp::gw::AccountID_def;
using atp::gw::SecurityID_def;
using atp::gw::Password_def;
using atp::gw::SenderCompID_def;
using atp::gw::TargetCompID_def;
using atp::gw::DefaultAppVerID_def;
using atp::gw::HeartBtInt_def;
using atp::gw::ClientFeatureCode_def;
using atp::gw::UserInfo_def;
using atp::gw::Side_def;
using atp::gw::OrdType_def;
using atp::gw::OrdStatus_def;
using atp::gw::Price_def;
using atp::gw::Qty_def;
using atp::gw::MarketID_def;
using atp::gw::Amt_def;
using atp::gw::ExecID_def;
using atp::gw::ExecType_def;
using atp::gw::TimeStamp_def;
using atp::gw::Time_def;
using atp::gw::String_def;
using atp::gw::BusinessType_def;
using atp::gw::OrderWay_def;
using atp::gw::ReportIndex_def;
using atp::gw::TotalNum_def;
using atp::gw::ReturnSeq_def;
using atp::gw::Bool_def;
using atp::gw::ExErrMsg_def;
using atp::gw::RejectReasonCode_def;
using atp::gw::CancelFlag_def;
using atp::gw::CancelOrderFlag_def;
using atp::gw::ClOrdID_def;
using atp::gw::OrderID_def;
using atp::gw::Currency_def;
using atp::gw::PBUID_def;
using atp::gw::BSFlag2_def;
using atp::gw::CreditDigest_def;
using atp::gw::CreditDigestName_def;
using atp::gw::SecuritySymbol_def;
using atp::gw::SecurityFullSymbol_def;
using atp::gw::ExchangeRate_def;
using atp::gw::SellRepaySource_def;
using atp::gw::LinkNum_def;
using atp::gw::LinkSpeed_def;
using atp::gw::AccessMode_def;
using atp::gw::License_def;
using atp::gw::PWD_def;
using atp::gw::OutsideInfo_def;
using atp::gw::EnforceFlag_def;

// ---- AgwHeader 固定大小 (16 bytes) ----
static constexpr size_t AGW_HEADER_SIZE = 16;

} // namespace lb_counter98
