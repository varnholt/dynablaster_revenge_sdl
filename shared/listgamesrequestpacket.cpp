#include "listgamesrequestpacket.h"

#include "log.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "ListGameRequest";
}

ListGamesRequestPacket::ListGamesRequestPacket() : Packet(Packet::LISTGAMESREQUEST)
{
   _packet_name = PACKETNAME;
}

void ListGamesRequestPacket::enqueue(BinaryWriter& /*out*/)
{
}

void ListGamesRequestPacket::dequeue(BinaryReader& /*in*/)
{
}

void ListGamesRequestPacket::debug()
{
   Log::info("ListGamesRequestPacket:debug: no members");
}
