#include "playermodifiedpacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "PlayerModified";
}

PlayerModifiedPacket::PlayerModifiedPacket(Constants::Color color) : Packet(Packet::PLAYERMODIFIED), _color(color)
{
   _packet_name = PACKETNAME;
}

PlayerModifiedPacket::PlayerModifiedPacket() : Packet(Packet::PLAYERMODIFIED)
{
   _packet_name = PACKETNAME;
}

Constants::Color PlayerModifiedPacket::getColor() const
{
   return static_cast<Constants::Color>(_color);
}

void PlayerModifiedPacket::enqueue(BinaryWriter& out)
{
   out << _color;
}

void PlayerModifiedPacket::dequeue(BinaryReader& in)
{
   in >> _color;
}

void PlayerModifiedPacket::debug()
{
   Log::info(std::format("PlayerModifiedPacket: player color: {}", _color));
}
