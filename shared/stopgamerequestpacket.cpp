#include "stopgamerequestpacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StopGameRequest";
}

StopGameRequestPacket::StopGameRequestPacket(int32_t id) : Packet(Packet::STOPGAMEREQUEST), _id(id)
{
   _packet_name = PACKETNAME;
}

StopGameRequestPacket::StopGameRequestPacket() : Packet(Packet::STOPGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

int32_t StopGameRequestPacket::getId() const
{
   return _id;
}

void StopGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _id;
}

void StopGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _id;
}

void StopGameRequestPacket::debug()
{
   Log::info(std::format("StopGameRequestPacket:debug: id: {}", _id));
}
