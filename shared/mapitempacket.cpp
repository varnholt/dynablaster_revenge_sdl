#include "mapitempacket.h"

#include "logging.h"

#include <string_view>
#include "mapitem.h"

namespace
{
constexpr std::string_view PACKETNAME = "MapItem";
}

MapItemPacket::MapItemPacket(Packet::TYPE type) : Packet(type)
{
   _packet_name = PACKETNAME;
}

MapItemPacket::MapItemPacket(Packet::TYPE packet_type, const MapItem& item)
    : Packet(packet_type), _x(item.getX()), _y(item.getY()), _unique_id(item.getUniqueId()), _item_type(item.getType())
{
   _packet_name = PACKETNAME;
}

int32_t MapItemPacket::getX() const
{
   return _x;
}

int32_t MapItemPacket::getY() const
{
   return _y;
}

MapItem::ItemType MapItemPacket::getItemType() const
{
   return _item_type;
}

int32_t MapItemPacket::getUniqueId() const
{
   return _unique_id;
}

void MapItemPacket::enqueue(BinaryWriter& out)
{
   out << static_cast<int32_t>(_item_type);
   out << _x;
   out << _y;
   out << _unique_id;
}

void MapItemPacket::dequeue(BinaryReader& in)
{
   int32_t item_type = 0;
   in >> item_type;
   _item_type = static_cast<MapItem::ItemType>(item_type);

   in >> _x >> _y >> _unique_id;
}

void MapItemPacket::debug()
{
   qDebug("MapItemPacketPacket: x: %d, y: %d", _x, _y);
}
