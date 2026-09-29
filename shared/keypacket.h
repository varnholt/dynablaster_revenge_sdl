#pragma once

#include "constants.h"
#include "packet.h"

class KeyPacket : public Packet
{
public:
   // read constructor
   KeyPacket();

   // write constructor
   KeyPacket(int8_t player_id, int8_t keys);

   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;
   void debug() override;

   // key combination (Constants::Key flags)
   [[nodiscard]] int8_t getKeys() const;
   [[nodiscard]] int8_t getPlayerId() const;

private:
   int8_t _player_id = 0;
   int8_t _keys = 0;
};
