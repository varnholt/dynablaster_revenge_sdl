#include "extramapitemcreatedpacket.h"

#include <string_view>

#include "extramapitem.h"

namespace
{
constexpr std::string_view PACKETNAME = "ExtraMapItemCreated";
}

ExtraMapItemCreatedPacket::ExtraMapItemCreatedPacket(const ExtraMapItem& item)
    : MapItemCreatedPacket(item), _extra_type(item.getExtraType())
{
   _packet_type = Packet::EXTRAMAPITEMCREATED;
   _packet_name = PACKETNAME;
}

ExtraMapItemCreatedPacket::ExtraMapItemCreatedPacket()
{
   _packet_type = Packet::EXTRAMAPITEMCREATED;
   _packet_name = PACKETNAME;
}

int32_t ExtraMapItemCreatedPacket::getExtraType() const
{
   return _extra_type;
}

void ExtraMapItemCreatedPacket::setSkullFaces(const std::vector<Constants::SkullType>& faces)
{
   _skull_faces = faces;
}

std::vector<Constants::SkullType> ExtraMapItemCreatedPacket::getSkullFaces() const
{
   return _skull_faces;
}

void ExtraMapItemCreatedPacket::enqueue(BinaryWriter& out)
{
   // six 4-bit skull faces packed into one word
   const auto faces = static_cast<uint32_t>(
      (_skull_faces[0] << 20) | (_skull_faces[1] << 16) | (_skull_faces[2] << 12) | (_skull_faces[3] << 8) | (_skull_faces[4] << 4) |
      (_skull_faces[5])
   );

   MapItemCreatedPacket::enqueue(out);

   out << _extra_type;
   out << faces;
}

void ExtraMapItemCreatedPacket::dequeue(BinaryReader& in)
{
   MapItemCreatedPacket::dequeue(in);

   uint32_t faces = 0;

   in >> _extra_type;
   in >> faces;

   _skull_faces[0] = static_cast<Constants::SkullType>((faces >> 20) & 0x0f);
   _skull_faces[1] = static_cast<Constants::SkullType>((faces >> 16) & 0x0f);
   _skull_faces[2] = static_cast<Constants::SkullType>((faces >> 12) & 0x0f);
   _skull_faces[3] = static_cast<Constants::SkullType>((faces >> 8) & 0x0f);
   _skull_faces[4] = static_cast<Constants::SkullType>((faces >> 4) & 0x0f);
   _skull_faces[5] = static_cast<Constants::SkullType>((faces) & 0x0f);
}
