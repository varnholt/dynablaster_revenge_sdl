#include "leavegamerequestpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "LeaveGameRequest";
}

LeaveGameRequestPacket::LeaveGameRequestPacket() : Packet(Packet::LEAVEGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

LeaveGameRequestPacket::LeaveGameRequestPacket(int32_t game_id, int32_t player_id)
    : Packet(Packet::LEAVEGAMEREQUEST), _game_id(game_id), _player_id(player_id)
{
   _packet_name = PACKETNAME;
}

void LeaveGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _game_id << _player_id;
}

void LeaveGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _game_id >> _player_id;
}

void LeaveGameRequestPacket::debug()
{
   qDebug("LeaveGameRequestPacket:debug: no members");
}

void LeaveGameRequestPacket::setPlayerId(int32_t id)
{
   _player_id = id;
}

int32_t LeaveGameRequestPacket::getPlayerId() const
{
   return _player_id;
}

void LeaveGameRequestPacket::setGameId(int32_t id)
{
   _game_id = id;
}

int32_t LeaveGameRequestPacket::getGameId() const
{
   return _game_id;
}
