#ifndef BOTPLAYERINFO_H
#define BOTPLAYERINFO_H

#include "constants.h"
#include "playerinfo.h"

class BotPlayerInfo : public PlayerInfo
{
public:
   //! reset player data
   void reset();

   //! player got a bomb extra
   void addBomb();

   //! player got a flame extra
   void addFlame();

   //! player got a speed extra
   void addSpeed();

   //! getter for bomb count
   int getBombCount() const;

   //! getter for flame count
   int getFlameCount() const;

   //! getter for speed count
   int getSpeedCount() const;

   //! setter for kick flag
   void setKickEnabled(bool);

   //! getter for kick flag
   bool isKickEnabled() const;

protected:
   //! number of bombs
   int _bombs = SERVER_DEFAULT_BOMBCOUNT;

   //! number of flames
   int _flames = SERVER_DEFAULT_FLAMECOUNT;

   //! number of speedups
   int _speed_ups = SERVER_DEFAULT_SPEEDUPS;

   //! player can kick
   bool _kick_enabled = false;
};

#endif  // BOTPLAYERINFO_H
