#include "leavegameresponsepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "LeaveGameResponse";
}

LeaveGameResponsePacket::LeaveGameResponsePacket() : Packet(Packet::LEAVEGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

LeaveGameResponsePacket::LeaveGameResponsePacket(int32_t game_id, int32_t player_id)
    : Packet(Packet::LEAVEGAMERESPONSE), _game_id(game_id), _player_id(player_id)
{
   _packet_name = PACKETNAME;
}

void LeaveGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _game_id << _player_id;
}

void LeaveGameResponsePacket::dequeue(BinaryReader& in)
{
   in >> _game_id >> _player_id;
}

void LeaveGameResponsePacket::debug()
{
   qDebug("LeaveGameResponsePacket:debug: no members");
}

void LeaveGameResponsePacket::setPlayerId(int32_t id)
{
   _player_id = id;
}

int32_t LeaveGameResponsePacket::getPlayerId() const
{
   return _player_id;
}

void LeaveGameResponsePacket::setGameId(int32_t id)
{
   _game_id = id;
}

int32_t LeaveGameResponsePacket::getGameId() const
{
   return _game_id;
}
