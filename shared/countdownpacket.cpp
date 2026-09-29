#include "countdownpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Countdown";
}

CountdownPacket::CountdownPacket(int8_t time_left) : Packet(Packet::COUNTDOWN), _time_left(time_left)
{
   _packet_name = PACKETNAME;
}

CountdownPacket::CountdownPacket() : Packet(Packet::COUNTDOWN)
{
   _packet_name = PACKETNAME;
}

int8_t CountdownPacket::getTimeLeft() const
{
   return _time_left;
}

void CountdownPacket::enqueue(BinaryWriter& out)
{
   out << _time_left;
}

void CountdownPacket::dequeue(BinaryReader& in)
{
   in >> _time_left;
}

void CountdownPacket::debug()
{
   qDebug("CountdownPacket:debug: time left: %d", _time_left);
}
