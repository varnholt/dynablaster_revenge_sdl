#include "keypacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Key";
}

KeyPacket::KeyPacket() : Packet(Packet::KEY)
{
   _packet_name = PACKETNAME;
}

KeyPacket::KeyPacket(int8_t player_id, int8_t keys) : Packet(Packet::KEY), _player_id(player_id), _keys(keys)
{
   _packet_name = PACKETNAME;
}

void KeyPacket::enqueue(BinaryWriter& out)
{
   out << _player_id;
   out << _keys;
}

void KeyPacket::dequeue(BinaryReader& in)
{
   in >> _player_id >> _keys;
}

int8_t KeyPacket::getKeys() const
{
   return _keys;
}

int8_t KeyPacket::getPlayerId() const
{
   return _player_id;
}

void KeyPacket::debug()
{
   qDebug("KeyPacket: player id: %d, keys: %d", _player_id, _keys);
}
