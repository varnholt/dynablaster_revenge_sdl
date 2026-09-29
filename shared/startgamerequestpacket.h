#pragma once

#include "packet.h"

class StartGameRequestPacket : public Packet
{
public:
   // write constructor
   explicit StartGameRequestPacket(int32_t id);

   // read constructor
   StartGameRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // game id
   [[nodiscard]] int32_t getId() const;

private:
   int32_t _id = 0;
};
