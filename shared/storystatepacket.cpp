#include "storystatepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StoryState";
}

StoryStatePacket::StoryStatePacket() : Packet(Packet::STORYSTATE)
{
   _packet_name = PACKETNAME;
}

StoryStatePacket::StoryStatePacket(int8_t stage, int8_t lives, int32_t score, int8_t state, int8_t enemies_left)
    : Packet(Packet::STORYSTATE),
      _stage(stage),
      _lives(lives),
      _score(score),
      _state(state),
      _enemies_left(enemies_left)
{
   _packet_name = PACKETNAME;
}

int8_t StoryStatePacket::getStage() const
{
   return _stage;
}

int8_t StoryStatePacket::getLives() const
{
   return _lives;
}

int32_t StoryStatePacket::getScore() const
{
   return _score;
}

int8_t StoryStatePacket::getState() const
{
   return _state;
}

int8_t StoryStatePacket::getEnemiesLeft() const
{
   return _enemies_left;
}

void StoryStatePacket::enqueue(BinaryWriter& out)
{
   out << _stage;
   out << _lives;
   out << _score;
   out << _state;
   out << _enemies_left;
}

void StoryStatePacket::dequeue(BinaryReader& in)
{
   in >> _stage >> _lives >> _score >> _state >> _enemies_left;
}

void StoryStatePacket::debug()
{
   qDebug("StoryStatePacket: stage: %d, lives: %d, score: %d, state: %d, enemies_left: %d", static_cast<int32_t>(_stage), static_cast<int32_t>(_lives), static_cast<int32_t>(_score), static_cast<int32_t>(_state), static_cast<int32_t>(_enemies_left));
}
