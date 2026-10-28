#include "storycontinuepacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StoryContinue";
}

StoryContinuePacket::StoryContinuePacket() : Packet(Packet::STORYCONTINUE)
{
   _packet_name = PACKETNAME;
}

StoryContinuePacket::StoryContinuePacket(bool accepted) : Packet(Packet::STORYCONTINUE), _accepted(accepted ? 1 : 0)
{
   _packet_name = PACKETNAME;
}

bool StoryContinuePacket::isAccepted() const
{
   return _accepted != 0;
}

void StoryContinuePacket::enqueue(BinaryWriter& out)
{
   out << _accepted;
}

void StoryContinuePacket::dequeue(BinaryReader& in)
{
   in >> _accepted;
}

void StoryContinuePacket::debug()
{
   Log::info(std::format("StoryContinuePacket: accepted: {}", static_cast<int32_t>(_accepted)));
}
