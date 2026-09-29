#include "gameinformation.h"

#include "binaryreader.h"
#include "binarywriter.h"

GameInformation::GameInformation(
   int32_t id,
   int32_t player_count,
   int32_t player_maximum_count,
   const std::string& game_name,
   const std::string& level_name,
   int32_t creator_id,
   Constants::Dimension dimensions,
   int32_t extras,
   int32_t duration,
   int32_t rounds_played,
   int32_t current_round,
   int32_t round_count,
   bool spawn_extras
)
    : _id(id),
      _player_count(player_count),
      _maximum_player_count(player_maximum_count),
      _game_name(game_name),
      _level_name(level_name),
      _creator_id(creator_id),
      _dimensions(dimensions),
      _extras(extras),
      _duration(duration),
      _games_played(rounds_played),
      _current_round(current_round),
      _round_count(round_count),
      _spawn_extras(spawn_extras)
{
}

int32_t GameInformation::getId() const
{
   return _id;
}

int32_t GameInformation::getPlayerCount() const
{
   return _player_count;
}

int32_t GameInformation::getPlayerMaximumCount() const
{
   return _maximum_player_count;
}

std::string GameInformation::getGameName() const
{
   return _game_name;
}

std::string GameInformation::getLevelName() const
{
   return _level_name;
}

int32_t GameInformation::getCreatorId() const
{
   return _creator_id;
}

Constants::Dimension GameInformation::getMapDimensions() const
{
   return _dimensions;
}

void GameInformation::setExtras(int32_t extras)
{
   _extras = extras;
}

int32_t GameInformation::getExtras() const
{
   return _extras;
}

void GameInformation::setDuration(int32_t duration)
{
   _duration = duration;
}

int32_t GameInformation::getDuration() const
{
   return _duration;
}

void GameInformation::setGamesPlayed(int32_t games_played)
{
   _games_played = games_played;
}

int32_t GameInformation::getGamesPlayed() const
{
   return _games_played;
}

float GameInformation::getMapScaleX() const
{
   switch (getMapDimensions())
   {
      case Constants::Dimension19x17:
         return 19.0f / 13.0f;

      case Constants::Dimension25x21:
         return 25.0f / 13.0f;

      default:
         return 1.0f;
   }
}

float GameInformation::getMapScaleY() const
{
   switch (getMapDimensions())
   {
      case Constants::Dimension19x17:
         return 17.0f / 11.0f;

      case Constants::Dimension25x21:
         return 21.0f / 11.0f;

      default:
         return 1.0f;
   }
}

int32_t GameInformation::getCurrentRound() const
{
   return _current_round;
}

int32_t GameInformation::getRoundCount() const
{
   return _round_count;
}

bool GameInformation::isSpawnExtrasEnabled() const
{
   return _spawn_extras;
}

void GameInformation::setSpawnExtrasEnabled(bool value)
{
   _spawn_extras = value;
}

BinaryWriter& operator<<(BinaryWriter& out, const GameInformation& info)
{
   out << info._id;
   out << info._player_count;
   out << info._maximum_player_count;
   out << info._game_name;
   out << info._level_name;
   out << info._creator_id;
   out << static_cast<int32_t>(info._dimensions);
   out << info._extras;
   out << info._duration;
   out << info._games_played;
   out << info._current_round;
   out << info._round_count;

   return out;
}

BinaryReader& operator>>(BinaryReader& in, GameInformation& info)
{
   int32_t dimensions = -1;

   in >> info._id;
   in >> info._player_count;
   in >> info._maximum_player_count;
   in >> info._game_name;
   in >> info._level_name;
   in >> info._creator_id;
   in >> dimensions;
   in >> info._extras;
   in >> info._duration;
   in >> info._games_played;
   in >> info._current_round;
   in >> info._round_count;

   info._dimensions = static_cast<Constants::Dimension>(dimensions);

   return in;
}
