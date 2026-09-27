#include "playerstats.h"

PlayerStats::PlayerStats()
 : _wins(0),
   _kills(0),
   _deaths(0),
   _survival_time(0),
   _extras_collected(0)
{
}


void PlayerStats::increaseWins()
{
   _wins++;
}


void PlayerStats::increaseKills()
{
   _kills++;
}


void PlayerStats::increaseDeaths()
{
   _deaths++;
}


void PlayerStats::increaseExtrasCollected()
{
   _extras_collected++;
}


void PlayerStats::reset()
{
   _wins = 0;
   _kills = 0;
   _deaths = 0;
   _survival_time = 0;
   _extras_collected = 0;
}


void PlayerStats::setWins(uint32_t wins)
{
   _wins = wins;
}


void PlayerStats::setKills(uint32_t kills)
{
   _kills = kills;
}


void PlayerStats::setDeaths(uint32_t deaths)
{
   _deaths = deaths;
}


void PlayerStats::setSurvivalTime(uint32_t time)
{
   _survival_time = time;
}


void PlayerStats::setExtrasCollected(uint32_t extrasCollected)
{
   _extras_collected = extrasCollected;
}


uint32_t PlayerStats::getWins() const
{
   return _wins;
}


uint32_t PlayerStats::getKills() const
{
   return _kills;
}


uint32_t PlayerStats::getDeaths() const
{
   return _deaths;
}


void PlayerStats::increaseSurvivalTime(uint32_t survivalTime)
{
   _survival_time += survivalTime;
}


uint32_t PlayerStats::getSurvivalTime() const
{
   return _survival_time;
}


uint32_t PlayerStats::getExtrasCollected() const
{
   return _extras_collected;
}

