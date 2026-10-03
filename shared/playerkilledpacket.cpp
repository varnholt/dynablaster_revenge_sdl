#include "playerkilledpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "PlayerKilled";
}

PlayerKilledPacket::PlayerKilledPacket(int32_t player_id, int32_t player_killed_by_id, Constants::Direction direction, float intensity)
    : Packet(Packet::PLAYERKILLED),
      _player_id(player_id),
      _player_killed_by_id(player_killed_by_id),
      _direction(direction),
      _intensity(intensity)
{
   _packet_name = PACKETNAME;
}

PlayerKilledPacket::PlayerKilledPacket() : Packet(Packet::PLAYERKILLED)
{
   _packet_name = PACKETNAME;
}

int32_t PlayerKilledPacket::getPlayerId() const
{
   return _player_id;
}

void PlayerKilledPacket::enqueue(BinaryWriter& out)
{
   out << _player_id;
   out << _player_killed_by_id;
   out << static_cast<int32_t>(_direction);
   out << _intensity;
}

void PlayerKilledPacket::dequeue(BinaryReader& in)
{
   int32_t direction = 0;
   in >> _player_id >> _player_killed_by_id >> direction >> _intensity;
   _direction = static_cast<Constants::Direction>(direction);
}

void PlayerKilledPacket::debug()
{
   qDebug("PlayerKilledPacket: player: %d, killed by: %d, intensity: %f", _player_id, _player_killed_by_id, _intensity);
}
