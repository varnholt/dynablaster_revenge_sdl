#pragma once

#include "packet.h"

class PositionPacket : public Packet
{
public:
   // write constructor
   PositionPacket(
      int8_t player_id,
      int8_t directions,
      float x,
      float y,
      float angle = 0.0f,
      float delta_x = 0.0f,
      float delta_y = 0.0f,
      float angle_delta = 0.0f,
      float speed = 0.0f
   );

   // read constructor
   PositionPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int8_t getPlayerId() const;
   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;

   // player orientation angle
   [[nodiscard]] float getAngle() const;

   [[nodiscard]] int8_t getDirections() const;

   [[nodiscard]] float getDeltaX() const;
   void setDeltaX(float delta_x);

   [[nodiscard]] float getDeltaY() const;
   void setDeltaY(float delta_y);

   // rotation delta
   [[nodiscard]] float getAngleDelta() const;
   void setAngleDelta(float angle_delta);

   [[nodiscard]] float getSpeed() const;

private:
   int8_t _player_id = 0;
   int8_t _directions = 0;
   float _x = 0.0f;
   float _y = 0.0f;
   float _dx = 0.0f;
   float _dy = 0.0f;
   float _angle = 0.0f;
   float _angle_delta = 0.0f;
   float _speed = 0.0f;
};
