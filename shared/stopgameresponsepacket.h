#pragma once

#include "packet.h"

class StopGameResponsePacket : public Packet
{
public:
   // write constructor, id -1 if not accepted
   StopGameResponsePacket(int32_t id, bool finished);

   // read constructor
   StopGameResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // game id
   [[nodiscard]] int32_t getId() const;
   [[nodiscard]] bool isFinished() const;

private:
   int32_t _id = -1;
   bool _finished = false;
};
