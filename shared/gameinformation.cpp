// header
#include "gameinformation.h"

// shared
#include "binaryreader.h"
#include "binarywriter.h"

//-----------------------------------------------------------------------------
/*!
 */
GameInformation::GameInformation()
    : mId(-1),
      mPlayerCount(0),
      mMaximumPlayerCount(0),
      mCreatorId(-1),
      mDimensions(Constants::DimensionInvalid),
      mExtras(0),
      mDuration(0),
      mGamesPlayed(0),
      mCurrentRound(0),
      mRoundCount(0),
      mSpawnExtras(false)
{
}

//-----------------------------------------------------------------------------
/*!
   \param id game id
   \param playerCount players in this game
   \param playerMaximum count maximum players for this game
   \param gameName this game's name
   \param levelName the level for this game
   \param creatorId id of the game creator
   \param dimensions playfield dimensions
   \param extras extra enums combined
   \param duration round duration
   \param roundsPlayer rounds played
   \param currentRound current round
   \param roundCount number of rounds to be played
   \param spawnExtras spawn extras
*/
GameInformation::GameInformation(
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
)
    : mId(id),
      mPlayerCount(playerCount),
      mMaximumPlayerCount(playerMaximumCount),
      mGameName(gameName),
      mLevelName(levelName),
      mCreatorId(creatorId),
      mDimensions(dimensions),
      mExtras(extras),
      mDuration(duration),
      mGamesPlayed(roundsPlayed),
      mCurrentRound(currentRound),
      mRoundCount(roundCount),
      mSpawnExtras(spawnExtras)
{
}

//-----------------------------------------------------------------------------
/*!
   \return game id
*/
int32_t GameInformation::getId() const
{
   return mId;
}

//-----------------------------------------------------------------------------
/*!
   \return player count
*/
int32_t GameInformation::getPlayerCount() const
{
   return mPlayerCount;
}

//-----------------------------------------------------------------------------
/*!
   \return player maximum count
*/
int32_t GameInformation::getPlayerMaximumCount() const
{
   return mMaximumPlayerCount;
}

//-----------------------------------------------------------------------------
/*!
   \return game name
*/
std::string GameInformation::getGameName() const
{
   return mGameName;
}

//-----------------------------------------------------------------------------
/*!
   \return level name
*/
std::string GameInformation::getLevelName() const
{
   return mLevelName;
}

//-----------------------------------------------------------------------------
/*!
   \return game creator id
*/
int32_t GameInformation::getCreatorId() const
{
   return mCreatorId;
}

//-----------------------------------------------------------------------------
/*!
   \return map dimension enum
*/
Constants::Dimension GameInformation::getMapDimensions() const
{
   return mDimensions;
}

//-----------------------------------------------------------------------------
/*!
   \param extras extras for this game
*/
void GameInformation::setExtras(int32_t extras)
{
   mExtras = extras;
}

//-----------------------------------------------------------------------------
/*!
   \return extras for this game
*/
int32_t GameInformation::getExtras() const
{
   return mExtras;
}

//-----------------------------------------------------------------------------
/*!
   \param duration game duration
*/
void GameInformation::setDuration(int32_t duration)
{
   mDuration = duration;
}

//-----------------------------------------------------------------------------
/*!
   \return game duration
*/
int32_t GameInformation::getDuration() const
{
   return mDuration;
}

//-----------------------------------------------------------------------------
/*!
   \param roundsPlayed number of gamesPlayed
*/
void GameInformation::setGamesPlayed(int32_t roundsPlayed)
{
   mGamesPlayed = roundsPlayed;
}

//-----------------------------------------------------------------------------
/*!
   \return games played
*/
int32_t GameInformation::getGamesPlayed() const
{
   return mGamesPlayed;
}

//-----------------------------------------------------------------------------
/*!
   \return map x scale
*/
float GameInformation::getMapScaleX() const
{
   float scale = 1.0f;

   switch (getMapDimensions())
   {
      case Constants::Dimension13x11:
         scale = 1.0f;
         break;

      case Constants::Dimension19x17:
         scale = 19.0f / 13.0f;
         break;

      case Constants::Dimension25x21:
         scale = 25.0f / 13.0f;
         break;

      default:
         break;
   }

   return scale;
}

//-----------------------------------------------------------------------------
/*!
   \return map y scale
*/
float GameInformation::getMapScaleY() const
{
   float scale = 1.0f;

   switch (getMapDimensions())
   {
      case Constants::Dimension13x11:
         scale = 1.0f;
         break;

      case Constants::Dimension19x17:
         scale = 17.0f / 11.0f;
         break;

      case Constants::Dimension25x21:
         scale = 21.0f / 11.0f;
         break;

      default:
         break;
   }

   return scale;
}

//-----------------------------------------------------------------------------
/*!
   \return current round
*/
int32_t GameInformation::getCurrentRound() const
{
   return mCurrentRound;
}

//-----------------------------------------------------------------------------
/*!
   \return round count
*/
int32_t GameInformation::getRoundCount() const
{
   return mRoundCount;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if extra spawning is enabled
*/
bool GameInformation::isSpawnExtrasEnabled() const
{
   return mSpawnExtras;
}

//-----------------------------------------------------------------------------
/*!
   \param value extra spawn enabled
*/
void GameInformation::setSpawnExtrasEnabled(bool value)
{
   mSpawnExtras = value;
}

//-----------------------------------------------------------------------------
/*!
   \param out datastream out
   \param info gameinfo object to stream
*/
BinaryWriter& operator<<(BinaryWriter& out, const GameInformation& info)
{
   out << info.mId;
   out << info.mPlayerCount;
   out << info.mMaximumPlayerCount;
   out << info.mGameName;
   out << info.mLevelName;
   out << info.mCreatorId;
   out << static_cast<int32_t>(info.mDimensions);
   out << info.mExtras;
   out << info.mDuration;
   out << info.mGamesPlayed;
   out << info.mCurrentRound;
   out << info.mRoundCount;

   return out;
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream in
   \param info gameinfo object to stream
*/
BinaryReader& operator>>(BinaryReader& in, GameInformation& info)
{
   int32_t dimensions = -1;

   in >> info.mId;
   in >> info.mPlayerCount;
   in >> info.mMaximumPlayerCount;
   in >> info.mGameName;
   in >> info.mLevelName;
   in >> info.mCreatorId;
   in >> dimensions;
   in >> info.mExtras;
   in >> info.mDuration;
   in >> info.mGamesPlayed;
   in >> info.mCurrentRound;
   in >> info.mRoundCount;

   info.mDimensions = static_cast<Constants::Dimension>(dimensions);

   return in;
}
