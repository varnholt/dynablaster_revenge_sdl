#pragma once

#include "packet.h"

#include <cstdint>

//! story mode after the last life: the player's answer whether to continue the stage
class StoryContinuePacket : public Packet
{
public:
   // read constructor
   StoryContinuePacket();

   // write constructor
   explicit StoryContinuePacket(bool accepted);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   //! true to play the stage again, false to end the story
   [[nodiscard]] bool isAccepted() const;

private:
   int8_t _accepted = 0;
};
