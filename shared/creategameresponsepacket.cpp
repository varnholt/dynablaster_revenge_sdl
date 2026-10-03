#include "creategameresponsepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "CreateGameResponse";
}

CreateGameResponsePacket::CreateGameResponsePacket(const GameInformation& game_information)
    : Packet(Packet::CREATEGAMERESPONSE), _game_information(game_information)
{
   _packet_name = PACKETNAME;
}

CreateGameResponsePacket::CreateGameResponsePacket() : Packet(Packet::CREATEGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

const GameInformation& CreateGameResponsePacket::getGameInformation() const
{
   return _game_information;
}

void CreateGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _game_information;
}

void CreateGameResponsePacket::dequeue(BinaryReader& in)
{
   in >> _game_information;
}

void CreateGameResponsePacket::debug()
{
   qDebug("CreateGameResponsePacket:debug: id: %d, player id: %d", _game_information.getId(), _game_information.getCreatorId());
}
