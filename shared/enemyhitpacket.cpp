#include "enemyhitpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "EnemyHit";
}

EnemyHitPacket::EnemyHitPacket() : Packet(Packet::ENEMYHIT)
{
   _packet_name = PACKETNAME;
}

EnemyHitPacket::EnemyHitPacket(int16_t id, int8_t hit_points)
    : Packet(Packet::ENEMYHIT),
      _id(id),
      _hit_points(hit_points)
{
   _packet_name = PACKETNAME;
}

int16_t EnemyHitPacket::getId() const
{
   return _id;
}

int8_t EnemyHitPacket::getHitPoints() const
{
   return _hit_points;
}

void EnemyHitPacket::enqueue(BinaryWriter& out)
{
   out << _id;
   out << _hit_points;
}

void EnemyHitPacket::dequeue(BinaryReader& in)
{
   in >> _id >> _hit_points;
}

void EnemyHitPacket::debug()
{
   qDebug("EnemyHitPacket: id: %d, hit_points: %d", static_cast<int32_t>(_id), static_cast<int32_t>(_hit_points));
}
