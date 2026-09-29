#pragma once

#include "mapitempacket.h"

class MapItem;

class MapItemRemovedPacket : public MapItemPacket
{
public:
   // write constructor
   explicit MapItemRemovedPacket(MapItem* item);

   // read constructor
   MapItemRemovedPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;
};
