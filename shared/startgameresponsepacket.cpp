#include "startgameresponsepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StartGameResponse";
}

StartGameResponsePacket::StartGameResponsePacket(int32_t id, bool started) : Packet(Packet::STARTGAMERESPONSE), _id(id), _started(started)
{
   _packet_name = PACKETNAME;
}

StartGameResponsePacket::StartGameResponsePacket() : Packet(Packet::STARTGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

int32_t StartGameResponsePacket::getId() const
{
   return _id;
}

bool StartGameResponsePacket::isStarted() const
{
   return _started;
}

void StartGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _id << _started;
}

void StartGameResponsePacket::dequeue(BinaryReader& in)
{
   in >> _id >> _started;
}

void StartGameResponsePacket::debug()
{
   qDebug("StartGameResponsePacket:debug: id: %d, started: %d", _id, _started);
}
