#include "detonationpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Detonation";
}

DetonationPacket::DetonationPacket(
   int32_t x,
   int32_t y,
   int8_t fields_up,
   int8_t fields_down,
   int8_t fields_left,
   int8_t fields_right,
   float intensity
)
    : Packet(Packet::DETONATION),
      _x(x),
      _y(y),
      _fields_up(fields_up),
      _fields_down(fields_down),
      _fields_left(fields_left),
      _fields_right(fields_right),
      _intensity(intensity)
{
   _packet_name = PACKETNAME;
}

DetonationPacket::DetonationPacket() : Packet(Packet::DETONATION)
{
   _packet_name = PACKETNAME;
}

void DetonationPacket::enqueue(BinaryWriter& out)
{
   out << _x;
   out << _y;
   out << _fields_up;
   out << _fields_down;
   out << _fields_left;
   out << _fields_right;
   out << _intensity;
}

void DetonationPacket::dequeue(BinaryReader& in)
{
   in >> _x >> _y >> _fields_up >> _fields_down >> _fields_left >> _fields_right >> _intensity;
}

int32_t DetonationPacket::getX() const
{
   return _x;
}

int32_t DetonationPacket::getY() const
{
   return _y;
}

int8_t DetonationPacket::getUp() const
{
   return _fields_up;
}

int8_t DetonationPacket::getDown() const
{
   return _fields_down;
}

int8_t DetonationPacket::getLeft() const
{
   return _fields_left;
}

int8_t DetonationPacket::getRight() const
{
   return _fields_right;
}

float DetonationPacket::getIntensity() const
{
   return _intensity;
}

void DetonationPacket::debug()
{
   qDebug(
      "DetonationPacket: x: %d, y: %d, "
      "up: %d, down: %d, left: %d, right: %d, "
      "intensity: %f",
      _x,
      _y,
      _fields_up,
      _fields_down,
      _fields_left,
      _fields_right,
      _intensity
   );
}
