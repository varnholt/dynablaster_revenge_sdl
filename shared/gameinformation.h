#pragma once

#include <cstdint>
#include <string>

// Qt

// shared
#include "constants.h"

class BinaryWriter;
class BinaryReader;

class GameInformation
{
public:
   //! constructor for reading
   GameInformation();

   //! constructor for writing
   GameInformation(
      int32_t id,
      int32_t playerCount,
      int32_t playerMaximumCount,
      const std::string& gameName,
      const std::string& levelName,
      int32_t creatorId,
      Constants::Dimension dimensions,
      int32_t extras,
      int32_t duration,
      int32_t roundsPlayed,
      int32_t currentRound,
      int32_t roundCount,
      bool spawnExtras
   );

   //! getter for game id
   [[nodiscard]] int32_t getId() const;

   //! getter for player count
   [[nodiscard]] int32_t getPlayerCount() const;

   //! getter for the player maximum count
   [[nodiscard]] int32_t getPlayerMaximumCount() const;

   //! getter for the game name
   [[nodiscard]] std::string getGameName() const;

   //! getter for the level name
   [[nodiscard]] std::string getLevelName() const;

   //! getter for the game creator id
   [[nodiscard]] int32_t getCreatorId() const;

   //! getter for the map dimension enum
   [[nodiscard]] Constants::Dimension getMapDimensions() const;

   //! setter for the extra enum combination
   void setExtras(int32_t extra);

   //! getter for the extra enum combination
   [[nodiscard]] int32_t getExtras() const;

   //! setter for the game duration
   void setDuration(int32_t duration);

   //! getter for the game duration
   [[nodiscard]] int32_t getDuration() const;

   //! setter for the number of games played
   void setGamesPlayed(int32_t gamesPlayed);

   //! getter for the number of games played
   [[nodiscard]] int32_t getGamesPlayed() const;

   //! getter for x map scale
   [[nodiscard]] float getMapScaleX() const;

   //! getter for y map scale
   [[nodiscard]] float getMapScaleY() const;

   //! getter for current round count
   [[nodiscard]] int32_t getCurrentRound() const;

   //! getter for round count
   [[nodiscard]] int32_t getRoundCount() const;

   //! getter for spawn extras flag
   [[nodiscard]] bool isSpawnExtrasEnabled() const;

   //! setter for spawn extras flag
   void setSpawnExtrasEnabled(bool value);

public:
   //! game id
   int32_t mId;

   //! player count
   int32_t mPlayerCount;

   //! maximum player count
   int32_t mMaximumPlayerCount;

   //! game name
   std::string mGameName;

   //! level name
   std::string mLevelName;

   //! creator id
   int32_t mCreatorId;

   //! game dimensions
   Constants::Dimension mDimensions;

   //! extras
   int32_t mExtras;

   //! game duration
   int32_t mDuration;

   //! number of games played
   int32_t mGamesPlayed;

   //! current round
   int32_t mCurrentRound;

   //! number of rounds
   int32_t mRoundCount;

   //! spawn extra flag
   bool mSpawnExtras;
};

BinaryWriter& operator<<(BinaryWriter& out, const GameInformation& info);
BinaryReader& operator>>(BinaryReader& in, GameInformation& info);
