#include "bombpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Bomb";
}

BombPacket::BombPacket(int8_t player_id, uint8_t x, uint8_t y) : Packet(Packet::BOMB), _player_id(player_id), _x(x), _y(y)
{
   _packet_name = PACKETNAME;
}

BombPacket::BombPacket() : Packet(Packet::BOMB)
{
   _packet_name = PACKETNAME;
}

uint8_t BombPacket::getX() const
{
   return _x;
}

int8_t BombPacket::getPlayerId() const
{
   return _player_id;
}

uint8_t BombPacket::getY() const
{
   return _y;
}

void BombPacket::enqueue(BinaryWriter& out)
{
   out << _player_id;
   out << _x;
   out << _y;
}

void BombPacket::dequeue(BinaryReader& in)
{
   in >> _player_id >> _x >> _y;
}

void BombPacket::debug()
{
   qDebug("BombPacket:bomb: player: %d (%d, %d)", _player_id, _x, _y);
}
