#ifndef ATP_GW_MSG_MAP__H_
#define ATP_GW_MSG_MAP__H_
#include "messages_agw_external.h"
#include "factory.h"
namespace atp
{
namespace gw
{
class FactoryClassATP_GW { };
#define MP_REGISTER_MESSAGE_ATP_GW(cls, packet_no,ver) static mp::ClassRegister<mp::RootPacket, cls, atp::gw::MsgTypeNo_def,atp::gw::FactoryClassATP_GW> cls##Register(packet_no,ver);
#define MP_CREATE_MESSAGE_ATP_GW(type,ver) mp::Factory<mp::RootPacket, atp::gw::MsgTypeNo_def,atp::gw::FactoryClassATP_GW>::Create(type,ver);
}
}
#endif // ATP_GW_MSG_MAP__H_