#pragma once

#include "mapitempacket.h"

class MapItemCreatedPacket : public MapItemPacket
{
public:
   // write constructor
   explicit MapItemCreatedPacket(const MapItem& item, int8_t creator = -1);

   // read constructor
   MapItemCreatedPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getAppearance() const;

   // the mapitem's creator
   [[nodiscard]] int8_t getPlayerId() const;

private:
   int32_t _appearance = -1;
   int8_t _player_id = -1;
};
