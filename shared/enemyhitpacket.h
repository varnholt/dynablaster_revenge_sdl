#pragma once

#include "packet.h"

#include <cstdint>

//! a boss took a hit and survived
class EnemyHitPacket : public Packet
{
public:
   // read constructor
   EnemyHitPacket();

   // write constructor
   EnemyHitPacket(int16_t id, int8_t hit_points);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int16_t getId() const;
   [[nodiscard]] int8_t getHitPoints() const;

private:
   int16_t _id = 0;
   int8_t _hit_points = 0;
};
