#pragma once

#include <cstdint>

#include "mapitem.h"
#include "packet.h"

class MapItemPacket : public Packet
{
public:
   //! constructor
   MapItemPacket(Packet::TYPE type);

   //! alternative constructor - item is only read from, not owned by this packet
   MapItemPacket(Packet::TYPE packetType, MapItem* item);

   //! debug packet
   void debug();

   //! enqueue packet
   virtual void enqueue(BinaryWriter&);

   //! dequeue packet
   virtual void dequeue(BinaryReader&);

   //! getter for item's x pos
   [[nodiscard]] int32_t getX() const;

   //! getter for item's y pos
   [[nodiscard]] int32_t getY() const;

   //! getter for item type
   [[nodiscard]] MapItem::ItemType getItemType() const;

   //! getter for item's unique id
   [[nodiscard]] int32_t getUniqueId() const;

private:
   int32_t mX;
   int32_t mY;
   int32_t mUniqueId;
   MapItem::ItemType mItemType;
};
