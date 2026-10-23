#pragma once

#include "packet.h"

#include <cstdint>

//! where a story mode enemy is and where it is heading
class EnemyPositionPacket : public Packet
{
public:
   //! state flags
   enum Flags : int8_t
   {
      FlagMoving = 0x1,
      FlagShielded = 0x2,
      FlagInvulnerable = 0x4
   };

   // read constructor
   EnemyPositionPacket();

   // write constructor
   EnemyPositionPacket(int16_t id, float x, float y, float angle, float dx, float dy, int8_t flags);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int16_t getId() const;
   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;
   [[nodiscard]] float getAngle() const;
   [[nodiscard]] float getDx() const;
   [[nodiscard]] float getDy() const;
   [[nodiscard]] int8_t getFlags() const;

private:
   int16_t _id = 0;
   float _x = 0;
   float _y = 0;
   float _angle = 0;
   float _dx = 0;
   float _dy = 0;
   int8_t _flags = 0;
};
