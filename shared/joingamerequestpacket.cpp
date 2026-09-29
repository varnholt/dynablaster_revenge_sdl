#include "joingamerequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "JoinGameRequest";
}

JoinGameRequestPacket::JoinGameRequestPacket(int32_t id) : Packet(Packet::JOINGAMEREQUEST), _id(id)
{
   _packet_name = PACKETNAME;
}

JoinGameRequestPacket::JoinGameRequestPacket() : Packet(Packet::JOINGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

int32_t JoinGameRequestPacket::getId() const
{
   return _id;
}

void JoinGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _id;
}

void JoinGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _id;
}

void JoinGameRequestPacket::debug()
{
   qDebug("JoinGameRequestPacket:debug: id: %d", _id);
}
