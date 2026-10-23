#pragma once

#include "packet.h"

#include <cstdint>

//! a story mode enemy died (or vanished, see reason)
class EnemyKilledPacket : public Packet
{
public:
   //! why the enemy is gone
   enum Reason : int8_t
   {
      ReasonKilled,
      ReasonRemoved
   };

   // read constructor
   EnemyKilledPacket();

   // write constructor
   EnemyKilledPacket(int16_t id, int8_t killer_id, int8_t reason, int32_t points);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int16_t getId() const;
   [[nodiscard]] int8_t getKillerId() const;
   [[nodiscard]] int8_t getReason() const;
   [[nodiscard]] int32_t getPoints() const;

private:
   int16_t _id = 0;
   int8_t _killer_id = 0;
   int8_t _reason = 0;
   int32_t _points = 0;
};
