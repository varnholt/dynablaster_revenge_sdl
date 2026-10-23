#include "pausepacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "Pause";
}

PausePacket::PausePacket() : Packet(Packet::PAUSE)
{
   _packet_name = PACKETNAME;
}

PausePacket::PausePacket(bool paused) : Packet(Packet::PAUSE), _paused(paused ? 1 : 0)
{
   _packet_name = PACKETNAME;
}

bool PausePacket::isPaused() const
{
   return _paused != 0;
}

void PausePacket::enqueue(BinaryWriter& out)
{
   out << _paused;
}

void PausePacket::dequeue(BinaryReader& in)
{
   in >> _paused;
}

void PausePacket::debug()
{
   Log::info(std::format("PausePacket: paused: {}", static_cast<int32_t>(_paused)));
}
