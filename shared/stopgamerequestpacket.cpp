#include "stopgamerequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "StopGameRequest";
}

StopGameRequestPacket::StopGameRequestPacket(int32_t id) : Packet(Packet::STOPGAMEREQUEST), _id(id)
{
   _packet_name = PACKETNAME;
}

StopGameRequestPacket::StopGameRequestPacket() : Packet(Packet::STOPGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

int32_t StopGameRequestPacket::getId() const
{
   return _id;
}

void StopGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _id;
}

void StopGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _id;
}

void StopGameRequestPacket::debug()
{
   qDebug("StopGameRequestPacket:debug: id: %d", _id);
}
