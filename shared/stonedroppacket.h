#pragma once

#include "packet.h"

class StoneDropPacket : public Packet
{
public:
   // write constructor
   StoneDropPacket(int8_t x, int8_t y);

   // read constructor
   StoneDropPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // stone position
   [[nodiscard]] int8_t getX() const;
   [[nodiscard]] int8_t getY() const;

private:
   int8_t _x = 0;
   int8_t _y = 0;
};
