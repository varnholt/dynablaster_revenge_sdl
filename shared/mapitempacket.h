#pragma once

#include <cstdint>

#include "mapitem.h"
#include "packet.h"

class MapItemPacket : public Packet
{
public:
   explicit MapItemPacket(Packet::TYPE type);

   // item is only read from, not owned by this packet
   MapItemPacket(Packet::TYPE packet_type, const MapItem& item);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getX() const;
   [[nodiscard]] int32_t getY() const;
   [[nodiscard]] MapItem::ItemType getItemType() const;
   [[nodiscard]] int32_t getUniqueId() const;

private:
   int32_t _x = 0;
   int32_t _y = 0;
   int32_t _unique_id = -1;
   MapItem::ItemType _item_type = MapItem::Unknown;
};
