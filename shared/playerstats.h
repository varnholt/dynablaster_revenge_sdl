#pragma once

#include <cstdint>

class BinaryWriter;
class BinaryReader;

class PlayerStats
{

public:

   PlayerStats();

   [[nodiscard]] uint32_t getWins() const;

   [[nodiscard]] uint32_t getKills() const;

   [[nodiscard]] uint32_t getDeaths() const;

   [[nodiscard]] uint32_t getSurvivalTime() const;

   [[nodiscard]] uint32_t getExtrasCollected() const;

   void setWins(uint32_t);

   void setKills(uint32_t);

   void setDeaths(uint32_t);

   void setSurvivalTime(uint32_t time);

   void setExtrasCollected(uint32_t extrasCollected);

   void increaseSurvivalTime(uint32_t);

   void increaseWins();

   void increaseKills();

   void increaseDeaths();

   void increaseExtrasCollected();

   void reset();


protected:

   uint32_t _wins;

   uint32_t _kills;

   uint32_t _deaths;

   uint32_t _survival_time;

   uint32_t _extras_collected;
};

BinaryWriter& operator<<(BinaryWriter& out, const PlayerStats& stats);
BinaryReader& operator>>(BinaryReader& in, PlayerStats& stats);
