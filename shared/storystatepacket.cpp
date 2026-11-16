#include "storystatepacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StoryState";
}

StoryStatePacket::StoryStatePacket() : Packet(Packet::STORYSTATE)
{
   _packet_name = PACKETNAME;
}

StoryStatePacket::StoryStatePacket(int8_t stage, int8_t lives, int32_t score, int8_t state, int8_t enemies_left, const StoryCode& code)
    : Packet(Packet::STORYSTATE),
      _stage(stage),
      _lives(lives),
      _score(score),
      _state(state),
      _enemies_left(enemies_left),
      _code_bombs(static_cast<int8_t>(code._bombs)),
      _code_flames(static_cast<int8_t>(code._flames)),
      _code_speedup(code._speedup)
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

std::string StoryStatePacket::getCode() const
{
   StoryCode code;
   code._stage_index = _stage;
   code._bombs = _code_bombs;
   code._flames = _code_flames;
   code._speedup = _code_speedup;

   // a fixed spelling per stage, so the code doesn't change while it's on screen
   code._shift = _stage & 1;
   code._invert = (_stage >> 1) & 7;
   return encodeStoryCode(code);
}

void StoryStatePacket::enqueue(BinaryWriter& out)
{
   out << _stage;
   out << _lives;
   out << _score;
   out << _state;
   out << _enemies_left;
   out << _code_bombs;
   out << _code_flames;
   out << _code_speedup;
}

void StoryStatePacket::dequeue(BinaryReader& in)
{
   in >> _stage >> _lives >> _score >> _state >> _enemies_left >> _code_bombs >> _code_flames >> _code_speedup;
}

void StoryStatePacket::debug()
{
   Log::info(std::format(
      "StoryStatePacket: stage: {}, lives: {}, score: {}, state: {}, enemies_left: {}",
      static_cast<int32_t>(_stage),
      static_cast<int32_t>(_lives),
      static_cast<int32_t>(_score),
      static_cast<int32_t>(_state),
      static_cast<int32_t>(_enemies_left)
   ));
}
