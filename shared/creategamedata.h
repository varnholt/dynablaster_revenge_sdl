#pragma once

#include <cstdint>
#include <string>

// shared
#include "constants.h"

class CreateGameData
{    
   public:

      //! constructor
      CreateGameData();

      //! destructor
      virtual ~CreateGameData();

      //! game's name
      std::string mName;

      //! game level name
      std::string mLevel;

      //! number of rounds
      int32_t mRounds;

      //! game duration (s)
      int32_t mDuration;

      //! maximum player count
      int32_t mMaxPlayers;

      //! bomb extra enabled
      bool mExtraBombEnabled;

      //! flame extra enabled
      bool mExtraFlameEnabled;

      //! speedup extra enabled
      bool mExtraSpeedupEnabled;

      //! kick extra enabled
      bool mExtraKickEnabled;

      //! skulls enabled
      bool mExtraSkullsEnabled;

      //! playfield dimension
      Constants::Dimension mDimension;
};
