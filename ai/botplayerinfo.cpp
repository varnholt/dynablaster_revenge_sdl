#include "botplayerinfo.h"

// shared
#include "playerdisease.h"

void BotPlayerInfo::addBomb()
{
   _bombs++;
}

void BotPlayerInfo::addFlame()
{
   _flames++;
}

void BotPlayerInfo::addSpeed()
{
   _speed_ups++;
}

void BotPlayerInfo::reset()
{
   _bombs = SERVER_DEFAULT_BOMBCOUNT;
   _flames = SERVER_DEFAULT_FLAMECOUNT;
   _speed_ups = SERVER_DEFAULT_SPEEDUPS;
   _kick_enabled = false;
}

/*!
   \return number of bombs
*/
int BotPlayerInfo::getBombCount() const
{
   int bomb_count = _bombs;

   if (getDisease())
   {
      Constants::SkullType skull_type = getDisease()->getType();

      if (skull_type == Constants::SkullNoBomb)
      {
         bomb_count = 0;
      }
      else if (skull_type == Constants::SkullMinimumBomb)
      {
         bomb_count = 1;
      }
      else if (skull_type == Constants::SkullMaximumBomb)
      {
         bomb_count = 10;
      }
   }

   return bomb_count;
}

/*!
   \return flame count
*/
int BotPlayerInfo::getFlameCount() const
{
   int flame_count = _flames;

   if (getDisease())
   {
      Constants::SkullType skull_type = getDisease()->getType();

      if (skull_type == Constants::SkullMinimumBomb)
      {
         flame_count = 1;
      }
      else if (skull_type == Constants::SkullMaximumBomb)
      {
         flame_count = 10;
      }
   }

   return flame_count;
}

/*!
   \return speed count
*/
int BotPlayerInfo::getSpeedCount() const
{
   return _speed_ups;
}

/*!
   \param kick kick enabled flag
*/
void BotPlayerInfo::setKickEnabled(bool kick)
{
   _kick_enabled = kick;
}

/*!
   \return kick enabled flag
*/
bool BotPlayerInfo::isKickEnabled() const
{
   return _kick_enabled;
}
