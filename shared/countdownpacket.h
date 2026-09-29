#pragma once

#include "packet.h"

class CountdownPacket : public Packet
{
public:
   // write constructor
   CountdownPacket(int8_t time_left);

   // read constructor
   CountdownPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int8_t getTimeLeft() const;

private:
   int8_t _time_left = 0;
};
