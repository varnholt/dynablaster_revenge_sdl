#pragma once

#include <cstdint>

#include "constants.h"
#include "packet.h"

class MapItemMovePacket : public Packet
{
public:
   // write constructor
   MapItemMovePacket(int32_t map_item_id, float speed, Constants::Direction direction, int32_t nominal_x = -1, int32_t nominal_y = -1);

   // read constructor
   MapItemMovePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getMapItemId() const;
   [[nodiscard]] float getSpeed() const;
   [[nodiscard]] Constants::Direction getDirection() const;
   [[nodiscard]] int32_t getNominalX() const;
   [[nodiscard]] int32_t getNominalY() const;

private:
   int32_t _map_item_id = 0;
   float _speed = 0.0f;
   Constants::Direction _direction = Constants::DirectionUnknown;
   int32_t _nominal_x = 0;
   int32_t _nominal_y = 0;
};
