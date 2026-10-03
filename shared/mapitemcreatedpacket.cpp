#include "mapitemcreatedpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "MapItemCreated";
}

MapItemCreatedPacket::MapItemCreatedPacket(const MapItem& item, int8_t creator)
    : MapItemPacket(Packet::MAPITEMCREATED, item), _appearance(item.getAppearance()), _player_id(creator)
{
   _packet_name = PACKETNAME;
}

MapItemCreatedPacket::MapItemCreatedPacket() : MapItemPacket(Packet::MAPITEMCREATED)
{
   _packet_name = PACKETNAME;
}

int32_t MapItemCreatedPacket::getAppearance() const
{
   return _appearance;
}

int8_t MapItemCreatedPacket::getPlayerId() const
{
   return _player_id;
}

void MapItemCreatedPacket::enqueue(BinaryWriter& out)
{
   MapItemPacket::enqueue(out);

   out << _appearance;
   out << _player_id;
}

void MapItemCreatedPacket::dequeue(BinaryReader& in)
{
   MapItemPacket::dequeue(in);

   in >> _appearance;
   in >> _player_id;
}

void MapItemCreatedPacket::debug()
{
   qDebug("MapItemCreatedPacket: x: %d, y: %d, type: %d, appearance: %d", getX(), getY(), getType(), _appearance);
}
