#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

class BinaryWriter;
class BinaryReader;

class GameInformation
{
public:
   // constructor for reading
   GameInformation() = default;

   // constructor for writing
   GameInformation(
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
   );

   [[nodiscard]] int32_t getId() const;
   [[nodiscard]] int32_t getPlayerCount() const;
   [[nodiscard]] int32_t getPlayerMaximumCount() const;
   [[nodiscard]] std::string getGameName() const;
   [[nodiscard]] std::string getLevelName() const;
   [[nodiscard]] int32_t getCreatorId() const;
   [[nodiscard]] Constants::Dimension getMapDimensions() const;

   // extra enum combination
   void setExtras(int32_t extras);
   [[nodiscard]] int32_t getExtras() const;

   void setDuration(int32_t duration);
   [[nodiscard]] int32_t getDuration() const;

   void setGamesPlayed(int32_t games_played);
   [[nodiscard]] int32_t getGamesPlayed() const;

   [[nodiscard]] float getMapScaleX() const;
   [[nodiscard]] float getMapScaleY() const;

   [[nodiscard]] int32_t getCurrentRound() const;
   [[nodiscard]] int32_t getRoundCount() const;

   [[nodiscard]] bool isSpawnExtrasEnabled() const;
   void setSpawnExtrasEnabled(bool value);

private:
   friend BinaryWriter& operator<<(BinaryWriter& out, const GameInformation& info);
   friend BinaryReader& operator>>(BinaryReader& in, GameInformation& info);

   int32_t _id = -1;
   int32_t _player_count = 0;
   int32_t _maximum_player_count = 0;
   std::string _game_name;
   std::string _level_name;
   int32_t _creator_id = -1;
   Constants::Dimension _dimensions = Constants::DimensionInvalid;
   int32_t _extras = 0;
   int32_t _duration = 0;
   int32_t _games_played = 0;
   int32_t _current_round = 0;
   int32_t _round_count = 0;

   // not serialized
   bool _spawn_extras = false;
};

BinaryWriter& operator<<(BinaryWriter& out, const GameInformation& info);
BinaryReader& operator>>(BinaryReader& in, GameInformation& info);
