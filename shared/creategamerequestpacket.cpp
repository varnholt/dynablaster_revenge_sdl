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

   _data.mName = name;
   _data.mLevel = level;
   _data.mRounds = rounds;
   _data.mDuration = duration;
   _data.mMaxPlayers = max_players;
   _data.mExtraBombEnabled = extra_bomb_enabled;
   _data.mExtraFlameEnabled = extra_flame_enabled;
   _data.mExtraSpeedupEnabled = extra_speedup_enabled;
   _data.mExtraKickEnabled = extra_kick_enabled;
   _data.mExtraSkullsEnabled = extra_skulls_enabled;
   _data.mDimension = dimension;
}

CreateGameRequestPacket::CreateGameRequestPacket() : Packet(Packet::CREATEGAMEREQUEST)
{
   _packet_name = PACKETNAME;

   _data.mRounds = 1;
   _data.mDuration = 180;
   _data.mMaxPlayers = 5;
   _data.mExtraBombEnabled = false;
   _data.mExtraFlameEnabled = false;
   _data.mExtraSpeedupEnabled = false;
   _data.mExtraKickEnabled = false;
   _data.mExtraSkullsEnabled = false;
}

std::string CreateGameRequestPacket::getName() const
{
   return _data.mName;
}

CreateGameData CreateGameRequestPacket::getData() const
{
   return _data;
}

void CreateGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _data.mName << _data.mLevel << _data.mRounds << _data.mDuration << _data.mMaxPlayers << _data.mExtraBombEnabled
       << _data.mExtraFlameEnabled << _data.mExtraSpeedupEnabled << _data.mExtraKickEnabled << _data.mExtraSkullsEnabled
       << static_cast<int32_t>(_data.mDimension);
}

void CreateGameRequestPacket::dequeue(BinaryReader& in)
{
   int32_t dimension = 0;

   in >> _data.mName >> _data.mLevel >> _data.mRounds >> _data.mDuration >> _data.mMaxPlayers >> _data.mExtraBombEnabled >>
      _data.mExtraFlameEnabled >> _data.mExtraSpeedupEnabled >> _data.mExtraKickEnabled >> _data.mExtraSkullsEnabled >> dimension;

   _data.mDimension = static_cast<Constants::Dimension>(dimension);
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
      _data.mName.c_str(),
      _data.mLevel.c_str(),
      _data.mRounds,
      _data.mDuration,
      _data.mMaxPlayers,
      _data.mExtraBombEnabled,
      _data.mExtraFlameEnabled,
      _data.mExtraSpeedupEnabled,
      _data.mExtraKickEnabled,
      _data.mExtraSkullsEnabled,
      _data.mDimension
   );
}
