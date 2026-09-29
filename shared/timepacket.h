#pragma once

#include "constants.h"
#include "packet.h"

class TimePacket : public Packet
{
public:
   // read constructor
   TimePacket();

   // write constructor
   explicit TimePacket(int32_t time_left);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getTimeLeft() const;

private:
   // time left in ms
   int32_t _time_left = 0;
};
