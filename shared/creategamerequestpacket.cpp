#include "creategamerequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "CreateGameReqest";
}

CreateGameRequestPacket::CreateGameRequestPacket(
   const std::string& name,
   const std::string& level,
   int32_t rounds,
   int32_t duration,
   int32_t max_players,
   bool extra_bomb_enabled,
   bool extra_flame_enabled,
   bool extra_speedup_enabled,
   bool extra_kick_enabled,
   bool extra_skulls_enabled,
   Constants::Dimension dimension
)
    : Packet(Packet::CREATEGAMEREQUEST)
{
   _packet_name = PACKETNAME;

   _data._name = name;
   _data._level = level;
   _data._rounds = rounds;
   _data._duration = duration;
   _data._max_players = max_players;
   _data._extra_bomb_enabled = extra_bomb_enabled;
   _data._extra_flame_enabled = extra_flame_enabled;
   _data._extra_speedup_enabled = extra_speedup_enabled;
   _data._extra_kick_enabled = extra_kick_enabled;
   _data._extra_skulls_enabled = extra_skulls_enabled;
   _data._dimension = dimension;
}

CreateGameRequestPacket::CreateGameRequestPacket() : Packet(Packet::CREATEGAMEREQUEST)
{
   _packet_name = PACKETNAME;

   _data._rounds = 1;
   _data._duration = 180;
   _data._max_players = 5;
   _data._extra_bomb_enabled = false;
   _data._extra_flame_enabled = false;
   _data._extra_speedup_enabled = false;
   _data._extra_kick_enabled = false;
   _data._extra_skulls_enabled = false;
}

std::string CreateGameRequestPacket::getName() const
{
   return _data._name;
}

CreateGameData CreateGameRequestPacket::getData() const
{
   return _data;
}

void CreateGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _data._name << _data._level << _data._rounds << _data._duration << _data._max_players << _data._extra_bomb_enabled
       << _data._extra_flame_enabled << _data._extra_speedup_enabled << _data._extra_kick_enabled << _data._extra_skulls_enabled
       << static_cast<int32_t>(_data._dimension);
}

void CreateGameRequestPacket::dequeue(BinaryReader& in)
{
   int32_t dimension = 0;

   in >> _data._name >> _data._level >> _data._rounds >> _data._duration >> _data._max_players >> _data._extra_bomb_enabled >>
      _data._extra_flame_enabled >> _data._extra_speedup_enabled >> _data._extra_kick_enabled >> _data._extra_skulls_enabled >> dimension;

   _data._dimension = static_cast<Constants::Dimension>(dimension);
}

void CreateGameRequestPacket::debug()
{
   qDebug(
      "CreateGameRequestPacket:debug:\n"
      "- name: %s\n"
      "- level: %s\n"
      "- rounds: %d\n"
      "- duration: %d\n"
      "- max players: %d\n"
      "- extra bomb enabled: %d\n"
      "- extra flame enabled: %d\n"
      "- extra speedup enabled: %d\n"
      "- extra kick enabled: %d"
      "- extra skulls enabled: %d"
      "- dimension: %d",
      _data._name.c_str(),
      _data._level.c_str(),
      _data._rounds,
      _data._duration,
      _data._max_players,
      _data._extra_bomb_enabled,
      _data._extra_flame_enabled,
      _data._extra_speedup_enabled,
      _data._extra_kick_enabled,
      _data._extra_skulls_enabled,
      _data._dimension
   );
}
