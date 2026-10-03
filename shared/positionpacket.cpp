#include "positionpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "Position";
}

PositionPacket::PositionPacket(
   int8_t player_id,
   int8_t directions,
   float x,
   float y,
   float angle,
   float delta_x,
   float delta_y,
   float angle_delta,
   float speed
)
    : Packet(Packet::POSITION),
      _player_id(player_id),
      _directions(directions),
      _x(x),
      _y(y),
      _dx(delta_x),
      _dy(delta_y),
      _angle(angle),
      _angle_delta(angle_delta),
      _speed(speed)
{
   _packet_name = PACKETNAME;
}

PositionPacket::PositionPacket() : Packet(Packet::POSITION)
{
   _packet_name = PACKETNAME;
}

float PositionPacket::getX() const
{
   return _x;
}

float PositionPacket::getDeltaX() const
{
   return _dx;
}

void PositionPacket::setDeltaX(float delta_x)
{
   _dx = delta_x;
}

float PositionPacket::getY() const
{
   return _y;
}

float PositionPacket::getDeltaY() const
{
   return _dy;
}

void PositionPacket::setDeltaY(float delta_y)
{
   _dy = delta_y;
}

float PositionPacket::getAngle() const
{
   return _angle;
}

float PositionPacket::getAngleDelta() const
{
   return _angle_delta;
}

void PositionPacket::setAngleDelta(float angle_delta)
{
   _angle_delta = angle_delta;
}

float PositionPacket::getSpeed() const
{
   return _speed;
}

int8_t PositionPacket::getDirections() const
{
   return _directions;
}

int8_t PositionPacket::getPlayerId() const
{
   return _player_id;
}

void PositionPacket::enqueue(BinaryWriter& out)
{
   out << _player_id;
   out << _directions;

   out << _x;
   out << _y;
   out << _angle;

   out << _dx;
   out << _dy;
   out << _angle_delta;
   out << _speed;
}

void PositionPacket::dequeue(BinaryReader& in)
{
   in >> _player_id >> _directions >> _x >> _y >> _angle >> _dx >> _dy >> _angle_delta >> _speed;
}

void PositionPacket::debug()
{
   qDebug("PositionPacket: player id: %d, position: (%f, %f, %f)  dir: (%f,%f,%f)", _player_id, _x, _y, _angle, _dx, _dy, _angle_delta);
}
