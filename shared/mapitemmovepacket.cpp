#include "mapitemmovepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "MapItemMove";
}

MapItemMovePacket::MapItemMovePacket(int32_t map_item_id, float speed, Constants::Direction direction, int32_t nominal_x, int32_t nominal_y)
    : Packet(Packet::MAPITEMMOVE),
      _map_item_id(map_item_id),
      _speed(speed),
      _direction(direction),
      _nominal_x(nominal_x),
      _nominal_y(nominal_y)
{
   _packet_name = PACKETNAME;
}

MapItemMovePacket::MapItemMovePacket() : Packet(Packet::MAPITEMMOVE)
{
   _packet_name = PACKETNAME;
}

float MapItemMovePacket::getSpeed() const
{
   return _speed;
}

Constants::Direction MapItemMovePacket::getDirection() const
{
   return _direction;
}

int32_t MapItemMovePacket::getNominalX() const
{
   return _nominal_x;
}

int32_t MapItemMovePacket::getNominalY() const
{
   return _nominal_y;
}

int32_t MapItemMovePacket::getMapItemId() const
{
   return _map_item_id;
}

void MapItemMovePacket::enqueue(BinaryWriter& out)
{
   out << _map_item_id;
   out << _speed;
   out << static_cast<int8_t>(_direction);
   out << _nominal_x;
   out << _nominal_y;
}

void MapItemMovePacket::dequeue(BinaryReader& in)
{
   int8_t direction = 0;
   in >> _map_item_id >> _speed >> direction >> _nominal_x >> _nominal_y;
   _direction = static_cast<Constants::Direction>(direction);
}

void MapItemMovePacket::debug()
{
   qDebug("MapItemMovePacket: unique id: %d, speed: %f, nominal: %d, %d", _map_item_id, _speed, _nominal_x, _nominal_y);
}
