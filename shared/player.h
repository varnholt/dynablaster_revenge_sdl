#pragma once

#include <cstdint>
#include <memory>
#include <string>

// shared
#include "constants.h"
#include "playerrotation.h"
#include "playerstats.h"

class PlayerDisease;


class Player{

public:

   //! constructor
   Player(int32_t id);

   //! destructor
   ~Player();

   //! reset player
   void reset();

   //! setter for logged-in flag
   void setLoggedIn(bool);

   //! getter for logged-in flag
   [[nodiscard]] bool isLoggedIn() const;

   //! setter for keys pressed
   void setKeysPressed(int32_t);

   //! getter for keys pressed
   [[nodiscard]] int32_t getKeysPressed() const;

   //! getter for keys previously pressed
   [[nodiscard]] int32_t getKeysPressedPreviously() const;

   //! setter for x position
   void setX(float);

   //! setter for y position
   void setY(float);

   //! getter for x position
   [[nodiscard]] float getX() const;

   //! getter for y position
   [[nodiscard]] float getY() const;

   //! setter for speed
   void setSpeed(float speed);

   //! getter for speed
   [[nodiscard]] float getSpeed() const;

   //! getter for id
   [[nodiscard]] int8_t getId() const;

   //! getter for the player's rotation
   [[nodiscard]] PlayerRotation* getPlayerRotation();

   //! setter for the player's nick
   void setNick(const std::string& nick);

   //! getter for the player's nick
   [[nodiscard]] const std::string& getNick() const;

   //! setter for bombs
   void setBombCount(int8_t count);

   //! getter for bombs
   [[nodiscard]] int8_t getBombCount() const;

   //! setter for flames
   void setFlameCount(int8_t count);

   //! getter for flames
   [[nodiscard]] int8_t getFlameCount() const;

   //! setter for bombs dropped
   void setBombsDroppedCount(int8_t count);

   //! getter for bombs dropped
   [[nodiscard]] int8_t getBombsDroppedCount() const;

   //! setter for killed flag
   void setKilled(bool);

   //! getter for killed flag
   [[nodiscard]] bool isKilled() const;

   //! setter for kick flag
   void setKickEnabled(bool);

   //! getter for kick flag
   [[nodiscard]] bool isKickEnabled() const;

   //! getter for bomb count default
   [[nodiscard]] static int8_t getBombCountDefault();

   //! getter for flame count default
   [[nodiscard]] static int8_t getFlameCountDefault();

   //! getter for position skip counter
   [[nodiscard]] int32_t getPositionSkipCounter() const;

   //! setter for position skip counter
   void setPositionSkipCounter(int32_t count);

   //! getter for overall stats
   [[nodiscard]] PlayerStats* getOverallStats();

   //! getter for round stats
   [[nodiscard]] PlayerStats* getRoundStats();

   //! setter for player color
   void setColor(Constants::Color color);

   //! getter for player color
   [[nodiscard]] Constants::Color getColor() const;

   //! setter for synchronized loading flag
   void setLoadingSynchronized(bool);

   //! getter for synchronized loading flag
   [[nodiscard]] bool isLoadingSynchronized() const;

   //! setter for bot flag
   void setBot(bool bot);

   //! getter for bot flag
   [[nodiscard]] bool isBot() const;

   //! infect player
   void infect(std::unique_ptr<PlayerDisease> disease);

   //! check if player is infected
   [[nodiscard]] bool isInfected() const;

   //! check if player is invincible
   [[nodiscard]] bool isInvincible() const;

   //! getter for disease
   [[nodiscard]] PlayerDisease* getDisease() const;

   //! increase number of kills
   void increaseKills();

   //! increase number of deaths
   void increaseDeaths();

   //! increase number of wins
   void increaseWins();

   //! increase survival time
   void increaseSurvivalTime(uint32_t survivalTime);

   //! increase extra count
   void increaseExtrasCollected();

   //! reset stats
   void resetStats();

   //! increase flame count
   void increaseFlameCount();

   //! increase bomb count
   void increaseBombCount();

   //! getter for bomb processed flag
   bool isBombKeyLocked() const;

   //! setter for bomb processed flag
   void setBombKeyLocked(bool processed);


private:

   //! id
   int8_t mId;

   //! player logged in
   bool mLoggedIn;

   //! x position
   float mX;

   //! y position
   float mY;

   //! nick
   std::string mNick;

   //! number of bombs
   int32_t mBombCount;

   //! number of flames
   int32_t mFlameCount;

   //! speed
   float mSpeed;

   //! keys currently pressed
   int32_t mKeysPressed;

   //! keys previously pressed
   int32_t mKeysPreviouslyPressed;

   //! bombs dropped count
   int32_t mBombsDroppedCount;

   //! bomb processed flag
   bool mBombKeyLocked;

   //! player rotation
   PlayerRotation mPlayerRotation;

   //! player is killed
   bool mKilled;

   //! player is able to kick
   bool mKickEnabled;

   //! Number of frames when last position packet was sent
   int32_t mPositionSkippedCounter;

   //! player overall stats
   PlayerStats mOverallStats;

   //! player round stats
   PlayerStats mRoundStats;

   //! player color
   Constants::Color mColor;

   //! loading synchronized
   bool mLoadingSynchronized;

   //! player is a bot or not
   bool mBot;

   //! player can be infected
   std::unique_ptr<PlayerDisease> mDisease;
};
