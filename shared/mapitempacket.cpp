// header
#include "mapitempacket.h"
#include "mapitem.h"

// Qt
#include "logging.h"

// defines
#define PACKETNAME "MapItem"

MapItemPacket::MapItemPacket(Packet::TYPE type) : Packet(type), mX(0), mY(0), mUniqueId(-1), mItemType(MapItem::Unknown)
{
   mPacketName = PACKETNAME;
}

MapItemPacket::MapItemPacket(Packet::TYPE type, MapItem* item)
    : Packet(type), mX(item->getX()), mY(item->getY()), mUniqueId(item->getUniqueId()), mItemType(item->getType())
{
   mPacketName = PACKETNAME;
}

int32_t MapItemPacket::getX() const
{
   return mX;
}

int32_t MapItemPacket::getY() const
{
   return mY;
}

MapItem::ItemType MapItemPacket::getItemType() const
{
   return mItemType;
}

int32_t MapItemPacket::getUniqueId() const
{
   return mUniqueId;
}

void MapItemPacket::enqueue(BinaryWriter& out)
{
   // write members
   out << static_cast<int32_t>(mItemType);
   out << mX;
   out << mY;
   out << mUniqueId;
}

void MapItemPacket::dequeue(BinaryReader& in)
{
   // read members
   int32_t itemType = 0;
   in >> itemType;
   mItemType = static_cast<MapItem::ItemType>(itemType);

   in >> mX >> mY >> mUniqueId;
}

void MapItemPacket::debug()
{
   qDebug("MapItemPacketPacket: x: %d, y: %d", mX, mY);
}
