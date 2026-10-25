#pragma once

#include "packet.h"

//! story mode cheat: the player asks the server to jump to the first stage of the next world
class StorySkipPacket : public Packet
{
public:
   StorySkipPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;
};
