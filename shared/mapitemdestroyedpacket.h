#pragma once

#include "constants.h"
#include "mapitemremovedpacket.h"

class MapItemDestroyedPacket : public MapItemPacket
{
public:
   // read constructor
   MapItemDestroyedPacket();

   // write constructor
   MapItemDestroyedPacket(const MapItem& item, int32_t player_id, Constants::Direction direction, float intensity);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // destroyer id
   [[nodiscard]] int32_t getPlayerId() const;

   // direction the item was destroyed from
   [[nodiscard]] Constants::Direction getDirection() const;

   // intensity the item was destroyed with
   [[nodiscard]] float getIntensity() const;

private:
   int32_t _player_id = 0;
   Constants::Direction _direction = Constants::DirectionUp;
   float _intensity = 0.0f;
};
