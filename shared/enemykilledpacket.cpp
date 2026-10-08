#include "enemykilledpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "EnemyKilled";
}

EnemyKilledPacket::EnemyKilledPacket() : Packet(Packet::ENEMYKILLED)
{
   _packet_name = PACKETNAME;
}

EnemyKilledPacket::EnemyKilledPacket(int16_t id, int8_t killer_id, int8_t reason, int32_t points)
    : Packet(Packet::ENEMYKILLED),
      _id(id),
      _killer_id(killer_id),
      _reason(reason),
      _points(points)
{
   _packet_name = PACKETNAME;
}

int16_t EnemyKilledPacket::getId() const
{
   return _id;
}

int8_t EnemyKilledPacket::getKillerId() const
{
   return _killer_id;
}

int8_t EnemyKilledPacket::getReason() const
{
   return _reason;
}

int32_t EnemyKilledPacket::getPoints() const
{
   return _points;
}

void EnemyKilledPacket::enqueue(BinaryWriter& out)
{
   out << _id;
   out << _killer_id;
   out << _reason;
   out << _points;
}

void EnemyKilledPacket::dequeue(BinaryReader& in)
{
   in >> _id >> _killer_id >> _reason >> _points;
}

void EnemyKilledPacket::debug()
{
   qDebug("EnemyKilledPacket: id: %d, killer_id: %d, reason: %d, points: %d", static_cast<int32_t>(_id), static_cast<int32_t>(_killer_id), static_cast<int32_t>(_reason), static_cast<int32_t>(_points));
}
