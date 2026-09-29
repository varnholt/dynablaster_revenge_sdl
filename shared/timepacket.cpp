#include "timepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Time";
}

TimePacket::TimePacket(int32_t time_left) : Packet(Packet::TIME), _time_left(time_left)
{
   _packet_name = PACKETNAME;
}

TimePacket::TimePacket() : Packet(Packet::TIME)
{
   _packet_name = PACKETNAME;
}

int32_t TimePacket::getTimeLeft() const
{
   return _time_left;
}

void TimePacket::enqueue(BinaryWriter& out)
{
   out << _time_left;
}

void TimePacket::dequeue(BinaryReader& in)
{
   in >> _time_left;
}

void TimePacket::debug()
{
   qDebug("TimePacket: game time left: %d", _time_left);
}
