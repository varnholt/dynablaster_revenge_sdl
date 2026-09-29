#pragma once

#include <cstdint>
#include <vector>

#include "packet.h"
#include "playerstats.h"

// one player's slice of a GameStatsPacket, streamed as a single vector
struct PlayerGameStats
{
   int32_t player_id = 0;
   PlayerStats overall_stats;
   PlayerStats round_stats;
};

BinaryWriter& operator<<(BinaryWriter& out, const PlayerGameStats& stats);
BinaryReader& operator>>(BinaryReader& in, PlayerGameStats& stats);

class GameStatsPacket : public Packet
{
public:
   // write constructor
   GameStatsPacket(
      const std::vector<int32_t>& ids,
      const std::vector<PlayerStats>& overall_stats,
      const std::vector<PlayerStats>& round_stats
   );

   // read constructor
   GameStatsPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] std::vector<PlayerStats> getOverallStats() const;
   [[nodiscard]] std::vector<PlayerStats> getRoundStats() const;
   [[nodiscard]] std::vector<int32_t> getPlayerIds() const;

private:
   // per-player id + overall/round stats, one entry per player
   std::vector<PlayerGameStats> _player_stats;
};
