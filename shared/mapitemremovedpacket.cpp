#include "mapitemremovedpacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "MapItemRemoved";
}

MapItemRemovedPacket::MapItemRemovedPacket(const MapItem& item) : MapItemPacket(Packet::MAPITEMREMOVED, item)
{
   _packet_name = PACKETNAME;
}

MapItemRemovedPacket::MapItemRemovedPacket() : MapItemPacket(Packet::MAPITEMREMOVED)
{
   _packet_name = PACKETNAME;
}

void MapItemRemovedPacket::enqueue(BinaryWriter& out)
{
   MapItemPacket::enqueue(out);
}

void MapItemRemovedPacket::dequeue(BinaryReader& in)
{
   MapItemPacket::dequeue(in);
}

void MapItemRemovedPacket::debug()
{
   Log::info(std::format("MapItemRemovedPacket: x: {}, y: {}", getX(), getY()));
}
