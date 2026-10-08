#pragma once

#include "packet.h"

#include <cstdint>

//! a story mode enemy appeared
class EnemyCreatedPacket : public Packet
{
public:
   // read constructor
   EnemyCreatedPacket();

   // write constructor
   EnemyCreatedPacket(int16_t id, int8_t type, float x, float y, float angle);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int16_t getId() const;
   [[nodiscard]] int8_t getType() const;
   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;
   [[nodiscard]] float getAngle() const;

private:
   int16_t _id = 0;
   int8_t _type = 0;
   float _x = 0;
   float _y = 0;
   float _angle = 0;
};
