#include "enemycreatedpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "EnemyCreated";
}

EnemyCreatedPacket::EnemyCreatedPacket() : Packet(Packet::ENEMYCREATED)
{
   _packet_name = PACKETNAME;
}

EnemyCreatedPacket::EnemyCreatedPacket(int16_t id, int8_t type, float x, float y, float angle)
    : Packet(Packet::ENEMYCREATED),
      _id(id),
      _type(type),
      _x(x),
      _y(y),
      _angle(angle)
{
   _packet_name = PACKETNAME;
}

int16_t EnemyCreatedPacket::getId() const
{
   return _id;
}

int8_t EnemyCreatedPacket::getType() const
{
   return _type;
}

float EnemyCreatedPacket::getX() const
{
   return _x;
}

float EnemyCreatedPacket::getY() const
{
   return _y;
}

float EnemyCreatedPacket::getAngle() const
{
   return _angle;
}

void EnemyCreatedPacket::enqueue(BinaryWriter& out)
{
   out << _id;
   out << _type;
   out << _x;
   out << _y;
   out << _angle;
}

void EnemyCreatedPacket::dequeue(BinaryReader& in)
{
   in >> _id >> _type >> _x >> _y >> _angle;
}

void EnemyCreatedPacket::debug()
{
   qDebug("EnemyCreatedPacket: id: %d, type: %d, x: %f, y: %f, angle: %f", static_cast<int32_t>(_id), static_cast<int32_t>(_type), _x, _y, _angle);
}
