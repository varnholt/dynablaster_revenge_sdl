#pragma once

#include "packet.h"

class StopGameRequestPacket : public Packet
{
public:
   // write constructor
   explicit StopGameRequestPacket(int32_t id);

   // read constructor
   StopGameRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // game id
   [[nodiscard]] int32_t getId() const;

private:
   int32_t _id = 0;
};
