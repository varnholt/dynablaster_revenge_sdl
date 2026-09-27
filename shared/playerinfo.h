#pragma once

#include <cstdint>
#include <memory>
#include <string>

// game
#include "constants.h"
#include "playerstats.h"

// forward declarations
class PlayerDisease;

class PlayerInfo
{
   public:

      PlayerInfo();

      //! out-of-line since mDisease's unique_ptr needs PlayerDisease's complete type
      ~PlayerInfo();

      void setId(int32_t);

      void setColor(Constants::Color);

      void setNick(const std::string&);

      [[nodiscard]] int32_t getId() const;

      [[nodiscard]] Constants::Color getColor() const;

      [[nodiscard]] std::string getNick() const;

      void setPosition(float x, float y, float angle);

      void setPositionDelta(float dx, float dy, float dAngle);

      [[nodiscard]] float getX() const;

      [[nodiscard]] float getY() const;

      [[nodiscard]] float getAngle() const;

      //! setter for delta x
      void setDeltaX(float val);

      //! setter for delta y
      void setDeltaY(float val);

      //! setter for delta angle
      void setDeltaAngle(float val);

      //! getter for player's x delta
      [[nodiscard]] float getDeltaX() const;

      //! getter for player's y delta
      [[nodiscard]] float getDeltaY() const;

      //! getter for player's y delta
      [[nodiscard]] float getAngleDelta() const;

      //! setter for player stats
      void setOverallStats(const PlayerStats&);

      //! getter for player overall stats
      [[nodiscard]] PlayerStats& getOverallStats();

      //! setter for player round stats
      void setRoundStats(const PlayerStats&);

      //! getter for player stats
      [[nodiscard]] PlayerStats& getRoundStats();

      //! setter for killed flag
      void setKilled(bool killed);

      //! getter for killed flag
      [[nodiscard]] bool isKilled() const;

      //! infect player
      void infect(std::unique_ptr<PlayerDisease> disease);

      //! check if player is infected
      [[nodiscard]] bool isInfected() const;

      //! getter for disease
      [[nodiscard]] PlayerDisease* getDisease() const;

      //! getter for directions
      [[nodiscard]] int8_t getDirections() const;

      //! setter for directions
      void setDirections(int8_t directions);


   protected:

      int32_t mId;

      Constants::Color mColor;

      //! overall stats
      PlayerStats mOverallStats;

      //! round stats
      PlayerStats mRoundStats;

      std::string mNick;

      float mX;

      float mY;

      float mAngle;

      //! player direction x
      float mDeltaX;

      //! player direction y
      float mDeltaY;

      //! player angle direction
      float mDeltaAngle;

      //! killed flag
      bool mKilled;

      //! player can be infected
      std::unique_ptr<PlayerDisease> mDisease;

      //! player's directions
      int8_t mDirections;
};
