#pragma once

#include "packet.h"

class BombPacket : public Packet
{
public:
   // write constructor
   BombPacket(int8_t player_id, uint8_t x, uint8_t y);

   // read constructor
   BombPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int8_t getPlayerId() const;

   // the bomb's field position
   [[nodiscard]] uint8_t getX() const;
   [[nodiscard]] uint8_t getY() const;

private:
   int8_t _player_id = 0;
   uint8_t _x = 0;
   uint8_t _y = 0;
};
