#include "gameeventpacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "GameEvent";
}

GameEventPacket::GameEventPacket(GameEvent event, float intensity, int32_t x, int32_t y)
    : Packet(Packet::GAMEEVENT), _event(event), _intensity(intensity), _x(x), _y(y)
{
   _packet_name = PACKETNAME;
}

GameEventPacket::GameEventPacket() : Packet(Packet::GAMEEVENT)
{
   _packet_name = PACKETNAME;
}

float GameEventPacket::getIntensity() const
{
   return _intensity;
}

GameEventPacket::GameEvent GameEventPacket::getGameEvent() const
{
   return _event;
}

void GameEventPacket::enqueue(BinaryWriter& out)
{
   out << static_cast<int32_t>(_event) << _intensity << _player_id << static_cast<int32_t>(_extra_type) << _x << _y;
}

void GameEventPacket::dequeue(BinaryReader& in)
{
   int32_t event = 0;
   int32_t extra = 0;

   in >> event >> _intensity >> _player_id >> extra >> _x >> _y;

   _event = static_cast<GameEvent>(event);
   _extra_type = static_cast<Constants::ExtraType>(extra);
}

void GameEventPacket::debug()
{
   qDebug("GameEventPacket: type: %d, intensity: %f", _event, _intensity);
}

void GameEventPacket::setPlayerId(int32_t player_id)
{
   _player_id = player_id;
}

void GameEventPacket::setExtraType(Constants::ExtraType extra_type)
{
   _extra_type = extra_type;
}

int32_t GameEventPacket::getPlayerId() const
{
   return _player_id;
}

Constants::ExtraType GameEventPacket::getExtraType() const
{
   return _extra_type;
}

int32_t GameEventPacket::getX() const
{
   return _x;
}

int32_t GameEventPacket::getY() const
{
   return _y;
}
