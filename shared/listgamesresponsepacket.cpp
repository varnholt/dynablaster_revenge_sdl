#include "listgamesresponsepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "ListGameResponse";
}

ListGamesResponsePacket::ListGamesResponsePacket(const std::vector<GameInformation>& games, bool update)
    : Packet(Packet::LISTGAMESRESPONSE), _games(games), _update(update)
{
   _packet_name = PACKETNAME;
}

ListGamesResponsePacket::ListGamesResponsePacket() : Packet(Packet::LISTGAMESRESPONSE)
{
   _packet_name = PACKETNAME;
}

std::vector<GameInformation> ListGamesResponsePacket::getGames() const
{
   return _games;
}

void ListGamesResponsePacket::setUpdate(bool update)
{
   _update = update;
}

bool ListGamesResponsePacket::isUpdate() const
{
   return _update;
}

void ListGamesResponsePacket::enqueue(BinaryWriter& out)
{
   out << _update;
   out << _games;
}

void ListGamesResponsePacket::dequeue(BinaryReader& in)
{
   in >> _update;
   in >> _games;
}

void ListGamesResponsePacket::debug()
{
   qDebug("ListGamesResponsePacket:debug: number of games sent: %d, update: %d", static_cast<int32_t>(_games.size()), isUpdate());
}
