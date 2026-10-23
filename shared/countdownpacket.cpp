#include "countdownpacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "Countdown";
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
   Log::info(std::format("CountdownPacket:debug: time left: {}", _time_left));
}
