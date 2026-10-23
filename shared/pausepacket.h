#pragma once

#include "packet.h"

#include <cstdint>

//! story mode pause: the player asks for it, the server confirms it to everyone in the game
class PausePacket : public Packet
{
public:
   // read constructor
   PausePacket();

   // write constructor
   explicit PausePacket(bool paused);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] bool isPaused() const;

private:
   int8_t _paused = 0;
};
