#include "joingameresponsepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "JoinGameResponse";
}

JoinGameResponsePacket::JoinGameResponsePacket(
   bool success,
   int32_t game_id,
   int32_t player_id,
   const std::string& nick,
   Constants::Color color
)
    : Packet(Packet::JOINGAMERESPONSE), _success(success), _game_id(game_id), _player_id(player_id), _nick(nick), _color(color)
{
   _packet_name = PACKETNAME;
}

JoinGameResponsePacket::JoinGameResponsePacket() : Packet(Packet::JOINGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

void JoinGameResponsePacket::setPlayerId(int32_t id)
{
   _player_id = id;
}

int32_t JoinGameResponsePacket::getPlayerId() const
{
   return _player_id;
}

void JoinGameResponsePacket::setGameId(int32_t id)
{
   _game_id = id;
}

int32_t JoinGameResponsePacket::getGameId() const
{
   return _game_id;
}

void JoinGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _success << _game_id << _player_id << _nick << static_cast<int32_t>(_color);
}

void JoinGameResponsePacket::dequeue(BinaryReader& in)
{
   int32_t color = 0;

   in >> _success >> _game_id >> _player_id >> _nick >> color;

   _color = static_cast<Constants::Color>(color);
}

void JoinGameResponsePacket::debug()
{
   qDebug("JoinGameResponsePacket:loginresponse: game: %d, player: %d (%s)", _game_id, _player_id, _success ? "accepted" : "denied");
}

const std::string& JoinGameResponsePacket::getNick() const
{
   return _nick;
}

bool JoinGameResponsePacket::isSuccessful() const
{
   return _success;
}

void JoinGameResponsePacket::setColor(Constants::Color color)
{
   _color = color;
}

Constants::Color JoinGameResponsePacket::getColor() const
{
   return _color;
}
