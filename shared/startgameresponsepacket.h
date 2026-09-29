#pragma once

#include "packet.h"

class StartGameResponsePacket : public Packet
{
public:
   // write constructor, id -1 if not accepted
   StartGameResponsePacket(int32_t id, bool started);

   // read constructor
   StartGameResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // game id
   [[nodiscard]] int32_t getId() const;
   [[nodiscard]] bool isStarted() const;

private:
   int32_t _id = -1;
   bool _started = false;
};
