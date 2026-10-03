#include "mapcreaterequestpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "MapCreateRequest";
}

MapCreateRequestPacket::MapCreateRequestPacket(
   int32_t width,
   int32_t height,
   int32_t stone_count,
   int32_t extra_bomb_count,
   int32_t extra_flame_count,
   const std::vector<Point>& start_positions
)
    : Packet(Packet::MAPCREATEREQUEST),
      _width(width),
      _height(height),
      _stone_count(stone_count),
      _extra_bomb_count(extra_bomb_count),
      _extra_flame_count(extra_flame_count),
      _start_positions(start_positions)
{
   _packet_name = PACKETNAME;
}

MapCreateRequestPacket::MapCreateRequestPacket() : Packet(Packet::MAPCREATEREQUEST)
{
   _packet_name = PACKETNAME;
}

void MapCreateRequestPacket::enqueue(BinaryWriter& out)
{
   out << _width;
   out << _height;
   out << _stone_count;
   out << _extra_bomb_count;
   out << _extra_flame_count;
   out << _start_positions;
}

void MapCreateRequestPacket::dequeue(BinaryReader& in)
{
   in >> _width >> _height >> _stone_count >> _extra_bomb_count >> _extra_flame_count >> _start_positions;
}

void MapCreateRequestPacket::debug()
{
   qDebug(
      "MapCreateRequestPacket:debug: width: %d, height: %d, stones: %d, "
      "bombextras: %d, flamextras: %d",
      _width,
      _height,
      _stone_count,
      _extra_bomb_count,
      _extra_flame_count
   );
}
