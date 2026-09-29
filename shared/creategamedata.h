#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

class CreateGameData
{
public:
   CreateGameData() = default;
   virtual ~CreateGameData() = default;

   std::string mName;
   std::string mLevel;
   int32_t mRounds = 0;

   // game duration (s)
   int32_t mDuration = 0;

   int32_t mMaxPlayers = 0;
   bool mExtraBombEnabled = false;
   bool mExtraFlameEnabled = false;
   bool mExtraSpeedupEnabled = false;
   bool mExtraKickEnabled = false;
   bool mExtraSkullsEnabled = false;
   Constants::Dimension mDimension = Constants::Dimension13x11;
};
