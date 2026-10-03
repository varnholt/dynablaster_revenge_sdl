#include "playersynchronizepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "PlayerSynchronize";
}

PlayerSynchronizePacket::PlayerSynchronizePacket() : Packet(Packet::PLAYERSYNCHRONIZEPACKET)
{
   _packet_name = PACKETNAME;
}

PlayerSynchronizePacket::PlayerSynchronizePacket(PlayerSynchronizePacket::SynchronizeProcess process)
    : Packet(Packet::PLAYERSYNCHRONIZEPACKET), _synchronize_process(process)
{
   _packet_name = PACKETNAME;
}

void PlayerSynchronizePacket::setSynchronizeProcess(PlayerSynchronizePacket::SynchronizeProcess process)
{
   _synchronize_process = process;
}

PlayerSynchronizePacket::SynchronizeProcess PlayerSynchronizePacket::getSynchronizeProcess() const
{
   return _synchronize_process;
}

void PlayerSynchronizePacket::enqueue(BinaryWriter& out)
{
   out << static_cast<uint8_t>(getSynchronizeProcess());
}

void PlayerSynchronizePacket::dequeue(BinaryReader& in)
{
   uint8_t process = 0;
   in >> process;
   setSynchronizeProcess(static_cast<SynchronizeProcess>(process));
}

void PlayerSynchronizePacket::debug()
{
   qDebug("PlayerSynchronizePacket::debug(): process: %d", getSynchronizeProcess());
}
