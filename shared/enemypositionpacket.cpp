#include "enemypositionpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "EnemyPosition";
}

EnemyPositionPacket::EnemyPositionPacket() : Packet(Packet::ENEMYPOSITION)
{
   _packet_name = PACKETNAME;
}

EnemyPositionPacket::EnemyPositionPacket(int16_t id, float x, float y, float angle, float dx, float dy, int8_t flags)
    : Packet(Packet::ENEMYPOSITION),
      _id(id),
      _x(x),
      _y(y),
      _angle(angle),
      _dx(dx),
      _dy(dy),
      _flags(flags)
{
   _packet_name = PACKETNAME;
}

int16_t EnemyPositionPacket::getId() const
{
   return _id;
}

float EnemyPositionPacket::getX() const
{
   return _x;
}

float EnemyPositionPacket::getY() const
{
   return _y;
}

float EnemyPositionPacket::getAngle() const
{
   return _angle;
}

float EnemyPositionPacket::getDx() const
{
   return _dx;
}

float EnemyPositionPacket::getDy() const
{
   return _dy;
}

int8_t EnemyPositionPacket::getFlags() const
{
   return _flags;
}

void EnemyPositionPacket::enqueue(BinaryWriter& out)
{
   out << _id;
   out << _x;
   out << _y;
   out << _angle;
   out << _dx;
   out << _dy;
   out << _flags;
}

void EnemyPositionPacket::dequeue(BinaryReader& in)
{
   in >> _id >> _x >> _y >> _angle >> _dx >> _dy >> _flags;
}

void EnemyPositionPacket::debug()
{
   qDebug("EnemyPositionPacket: id: %d, x: %f, y: %f, angle: %f, dx: %f, dy: %f, flags: %d", static_cast<int32_t>(_id), _x, _y, _angle, _dx, _dy, static_cast<int32_t>(_flags));
}
