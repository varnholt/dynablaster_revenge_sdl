#include "listgamesrequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "ListGameRequest";
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
   qDebug("ListGamesRequestPacket:debug: no members");
}
