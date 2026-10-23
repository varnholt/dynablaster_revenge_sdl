#include "stopgameresponsepacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StopGameResponse";
}

StopGameResponsePacket::StopGameResponsePacket(int32_t id, bool finished) : Packet(Packet::STOPGAMERESPONSE), _id(id), _finished(finished)
{
   _packet_name = PACKETNAME;
}

StopGameResponsePacket::StopGameResponsePacket() : Packet(Packet::STOPGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

int32_t StopGameResponsePacket::getId() const
{
   return _id;
}

bool StopGameResponsePacket::isFinished() const
{
   return _finished;
}

void StopGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _id << _finished;
}

void StopGameResponsePacket::dequeue(BinaryReader& in)
{
   in >> _id >> _finished;
}

void StopGameResponsePacket::debug()
{
   Log::info(std::format("StopGameResponsePacket:debug: id: {}, all rounds finished: {}", _id, _finished));
}
