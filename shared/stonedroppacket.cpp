#include "stonedroppacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StoneDrop";
}

StoneDropPacket::StoneDropPacket(int8_t x, int8_t y) : Packet(Packet::STONEDROP), _x(x), _y(y)
{
   _packet_name = PACKETNAME;
}

StoneDropPacket::StoneDropPacket() : Packet(Packet::STONEDROP)
{
   _packet_name = PACKETNAME;
}

int8_t StoneDropPacket::getX() const
{
   return _x;
}

int8_t StoneDropPacket::getY() const
{
   return _y;
}

void StoneDropPacket::enqueue(BinaryWriter& out)
{
   out << _x;
   out << _y;
}

void StoneDropPacket::dequeue(BinaryReader& in)
{
   in >> _x >> _y;
}

void StoneDropPacket::debug()
{
   Log::info(std::format("StoneDropPacket: x: {}, y: {}", _x, _y));
}
