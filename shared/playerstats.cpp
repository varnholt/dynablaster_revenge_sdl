#include "playerstats.h"

#include "binaryreader.h"
#include "binarywriter.h"

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

void PlayerStats::setExtrasCollected(uint32_t extras_collected)
{
   _extras_collected = extras_collected;
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

void PlayerStats::increaseSurvivalTime(uint32_t survival_time)
{
   _survival_time += survival_time;
}

uint32_t PlayerStats::getSurvivalTime() const
{
   return _survival_time;
}

uint32_t PlayerStats::getExtrasCollected() const
{
   return _extras_collected;
}

BinaryWriter& operator<<(BinaryWriter& out, const PlayerStats& stats)
{
   out << stats.getWins() << stats.getKills() << stats.getDeaths() << stats.getSurvivalTime() << stats.getExtrasCollected();
   return out;
}

BinaryReader& operator>>(BinaryReader& in, PlayerStats& stats)
{
   uint32_t wins = 0;
   uint32_t kills = 0;
   uint32_t deaths = 0;
   uint32_t survival_time = 0;
   uint32_t extras_collected = 0;

   in >> wins >> kills >> deaths >> survival_time >> extras_collected;

   stats.setWins(wins);
   stats.setKills(kills);
   stats.setDeaths(deaths);
   stats.setSurvivalTime(survival_time);
   stats.setExtrasCollected(extras_collected);

   return in;
}
