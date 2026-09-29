#include "startgamerequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "StartGameRequest";
}

StartGameRequestPacket::StartGameRequestPacket(int32_t id) : Packet(Packet::STARTGAMEREQUEST), _id(id)
{
   _packet_name = PACKETNAME;
}

StartGameRequestPacket::StartGameRequestPacket() : Packet(Packet::STARTGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

int32_t StartGameRequestPacket::getId() const
{
   return _id;
}

void StartGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _id;
}

void StartGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _id;
}

void StartGameRequestPacket::debug()
{
   qDebug("StartGameRequestPacket:debug: id: %d", _id);
}
