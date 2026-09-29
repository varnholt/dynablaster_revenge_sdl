#pragma once

#include <cstdint>

class BinaryWriter;
class BinaryReader;

class PlayerStats
{
public:
   PlayerStats() = default;

   [[nodiscard]] uint32_t getWins() const;

   [[nodiscard]] uint32_t getKills() const;

   [[nodiscard]] uint32_t getDeaths() const;

   [[nodiscard]] uint32_t getSurvivalTime() const;

   [[nodiscard]] uint32_t getExtrasCollected() const;

   void setWins(uint32_t wins);

   void setKills(uint32_t kills);

   void setDeaths(uint32_t deaths);

   void setSurvivalTime(uint32_t time);

   void setExtrasCollected(uint32_t extras_collected);

   void increaseSurvivalTime(uint32_t survival_time);

   void increaseWins();

   void increaseKills();

   void increaseDeaths();

   void increaseExtrasCollected();

   void reset();

protected:
   uint32_t _wins = 0;
   uint32_t _kills = 0;
   uint32_t _deaths = 0;
   uint32_t _survival_time = 0;
   uint32_t _extras_collected = 0;
};

BinaryWriter& operator<<(BinaryWriter& out, const PlayerStats& stats);
BinaryReader& operator>>(BinaryReader& in, PlayerStats& stats);
