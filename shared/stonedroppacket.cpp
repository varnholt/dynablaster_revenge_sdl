#include "stonedroppacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "StoneDrop";
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
   qDebug("StoneDropPacket: x: %d, y: %d", _x, _y);
}
