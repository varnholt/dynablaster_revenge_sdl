#pragma once

#include "constants.h"
#include "packet.h"

class PlayerInfectedPacket : public Packet
{
public:
   // read constructor
   PlayerInfectedPacket();

   // write constructor
   PlayerInfectedPacket(int32_t player_id, Constants::SkullType type);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getPlayerId() const;
   [[nodiscard]] Constants::SkullType getSkullType() const;

   [[nodiscard]] int32_t getInfectorId() const;
   void setInfectorId(int32_t value);

   // position where the extra has been picked up
   void setExtraPos(uint8_t x, uint8_t y);
   [[nodiscard]] uint8_t getExtraPosX() const;
   [[nodiscard]] uint8_t getExtraPosY() const;

private:
   int32_t _player_id = 0;
   Constants::SkullType _skull_type = Constants::SkullAutofire;
   int32_t _infector_id = -1;
   uint8_t _extra_pos_x = 0;
   uint8_t _extra_pos_y = 0;
};
