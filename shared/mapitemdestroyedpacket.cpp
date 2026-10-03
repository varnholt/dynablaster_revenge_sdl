#include "mapitemdestroyedpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "MapItemDestroyed";
}

MapItemDestroyedPacket::MapItemDestroyedPacket(const MapItem& item, int32_t player_id, Constants::Direction direction, float intensity)
    : MapItemPacket(Packet::MAPITEMDESTROYED, item), _player_id(player_id), _direction(direction), _intensity(intensity)
{
   _packet_name = PACKETNAME;
}

MapItemDestroyedPacket::MapItemDestroyedPacket() : MapItemPacket(Packet::MAPITEMDESTROYED)
{
   _packet_name = PACKETNAME;
}

void MapItemDestroyedPacket::enqueue(BinaryWriter& out)
{
   MapItemPacket::enqueue(out);

   out << _player_id;
   out << static_cast<int32_t>(_direction);
   out << _intensity;
}

void MapItemDestroyedPacket::dequeue(BinaryReader& in)
{
   MapItemPacket::dequeue(in);

   int32_t direction = 0;
   in >> _player_id >> direction >> _intensity;
   _direction = static_cast<Constants::Direction>(direction);
}

void MapItemDestroyedPacket::debug()
{
   qDebug("MapItemDestroyedPacket: x: %d, y: %d, player: %d, intensity: %f", getX(), getY(), _player_id, _intensity);
}

int32_t MapItemDestroyedPacket::getPlayerId() const
{
   return _player_id;
}

Constants::Direction MapItemDestroyedPacket::getDirection() const
{
   return _direction;
}

float MapItemDestroyedPacket::getIntensity() const
{
   return _intensity;
}
