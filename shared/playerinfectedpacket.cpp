#include "playerinfectedpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "PlayerInfected";
}

PlayerInfectedPacket::PlayerInfectedPacket(int32_t player_id, Constants::SkullType type)
    : Packet(Packet::PLAYERINFECTEDPACKET), _player_id(player_id), _skull_type(type)
{
   _packet_name = PACKETNAME;
}

PlayerInfectedPacket::PlayerInfectedPacket() : Packet(Packet::PLAYERINFECTEDPACKET)
{
   _packet_name = PACKETNAME;
}

int32_t PlayerInfectedPacket::getPlayerId() const
{
   return _player_id;
}

Constants::SkullType PlayerInfectedPacket::getSkullType() const
{
   return _skull_type;
}

void PlayerInfectedPacket::enqueue(BinaryWriter& out)
{
   out << _player_id;
   out << static_cast<int32_t>(_skull_type);
   out << _infector_id;
   out << _extra_pos_x;
   out << _extra_pos_y;
}

void PlayerInfectedPacket::dequeue(BinaryReader& in)
{
   int32_t skull = 0;
   in >> _player_id >> skull >> _infector_id >> _extra_pos_x >> _extra_pos_y;
   _skull_type = static_cast<Constants::SkullType>(skull);
}

void PlayerInfectedPacket::debug()
{
   qDebug("PlayerInfectedPacket: player: %d, skull type: %d", _player_id, _skull_type);
}

uint8_t PlayerInfectedPacket::getExtraPosY() const
{
   return _extra_pos_y;
}

uint8_t PlayerInfectedPacket::getExtraPosX() const
{
   return _extra_pos_x;
}

void PlayerInfectedPacket::setExtraPos(uint8_t x, uint8_t y)
{
   _extra_pos_x = x;
   _extra_pos_y = y;
}

int32_t PlayerInfectedPacket::getInfectorId() const
{
   return _infector_id;
}

void PlayerInfectedPacket::setInfectorId(int32_t value)
{
   _infector_id = value;
}
