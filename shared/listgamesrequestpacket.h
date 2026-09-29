#pragma once

#include "packet.h"

class ListGamesRequestPacket : public Packet
{
public:
   ListGamesRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;
};
